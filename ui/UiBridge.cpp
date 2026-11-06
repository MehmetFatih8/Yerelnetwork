#include "UiBridge.h"

#include <exception>
#include <filesystem>

#include "../Logger.h"
#include "../Text.h"
#include "ArayuzHtml.h"

namespace {

constexpr wchar_t kTitle[] = L"Yerelkeşif";

// Arayüzden gelen "[ip, yol1, yol2, ...]" dizisinin elemanlarını dosya yollarına çevirir
std::vector<std::filesystem::path> ToPaths(const std::vector<std::string>& values, std::size_t from) {
    std::vector<std::filesystem::path> paths;
    for (std::size_t i = from; i < values.size(); ++i) {
        paths.push_back(PathFromUtf8(values[i]));
    }
    return paths;
}

}  // namespace

UiBridge::UiBridge(MainWindow& window, AppController& app)
    : m_window(window), m_app(app), m_web(false, window.handle()) {
    m_window.onMaximizeChanged = [this](bool maximized) {
        post(std::string("window.pencereDurumu && window.pencereDurumu(") + (maximized ? "true" : "false") + ")");
    };
    m_drop.onDrop = [this](const std::vector<std::wstring>& paths) {
        post("window.dosyalarEklendi(" + FilesJson(paths) + ")");
    };

    configureBrowser();
    bindFunctions();
    bindEvents();

    m_web.set_html(kArayuzHtml);
    m_window.fitWebView();
}

UiBridge::~UiBridge() {
    // Ağ thread'i durmadan webview yıkılmamalı; olaylar artık post edilmeyecek
    m_app.stop();
    m_window.onMaximizeChanged = nullptr;
}

void UiBridge::run() {
    m_web.run();
}

void UiBridge::configureBrowser() {
    // Tarayıcı gibi görünmesin: sağ tık menüsü, yakınlaştırma, DevTools ve tarayıcı kısayolları kapalı
    auto controllerResult = m_web.browser_controller();
    if (!controllerResult.ok()) return;

    auto* controller = static_cast<ICoreWebView2Controller*>(controllerResult.value());
    ICoreWebView2* core = nullptr;
    ICoreWebView2Settings* settings = nullptr;
    if (SUCCEEDED(controller->get_CoreWebView2(&core)) && SUCCEEDED(core->get_Settings(&settings))) {
        settings->put_AreDefaultContextMenusEnabled(FALSE);
        settings->put_IsZoomControlEnabled(FALSE);
        settings->put_AreDevToolsEnabled(FALSE);

        ICoreWebView2Settings3* settings3 = nullptr;
        if (SUCCEEDED(settings->QueryInterface(IID_ICoreWebView2Settings3, reinterpret_cast<void**>(&settings3)))) {
            settings3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
            settings3->Release();
        }
    }

    if (core) {
        EventRegistrationToken token{};
        core->add_WebMessageReceived(&m_drop, &token);
    }
    if (settings) settings->Release();
    if (core) core->Release();
}

void UiBridge::bindFunctions() {
    // Arayüz ilk kareyi çizince çağırır: pencereyi göster ve ağ servislerini başlat
    m_web.bind("uygulamaHazir", [this](std::string) -> std::string {
        m_window.show();
        if (!m_started) {
            m_started = true;
            try {
                m_app.start();
            } catch (const std::exception& e) {
                LOG() << "ERR -> Ağ servisi başlatılamadı: " << e.what();
                showError(std::string("Ağ servisi başlatılamadı: ") + e.what() +
                          "\nAynı anda başka bir Yerelkeşif çalışıyor olabilir.");
                m_window.close();
            }
        }
        return "null";
    });

    // Özel başlık çubuğu
    m_web.bind("pencereKucult", [this](std::string) -> std::string {
        m_window.minimize();
        return "null";
    });
    m_web.bind("pencereBuyult", [this](std::string) -> std::string {
        m_window.toggleMaximize();
        return "null";
    });
    m_web.bind("pencereKapat", [this](std::string) -> std::string {
        m_window.close();
        return "null";
    });
    m_web.bind("pencereSurukle", [this](std::string) -> std::string {
        m_window.dragWindow();
        return "null";
    });
    m_web.bind("pencereBoyutlandir", [this](std::string req) -> std::string {
        const auto args = ParseJsonStringArray(req);
        if (!args.empty() && args[0] == "ust") m_window.resizeFromTop();
        return "null";
    });

    // Dosya seçme: [{"name","size","path"}] JSON'u döner
    m_web.bind("cppDosyaSec", [this](std::string) -> std::string {
        return PickFilesJson(m_window.handle());
    });

    // Manuel IP ekleme: hata metni ya da null döner
    m_web.bind("cppIpEkle", [this](std::string req) -> std::string {
        const auto args = ParseJsonStringArray(req);
        const std::string ip = args.empty() ? std::string() : args[0];
        const std::string error = AppController::validateIp(ip);
        if (!error.empty()) return JsonQuote(error);
        m_app.addDevice(ip);
        return "null";
    });

    // args: [ip, metin]
    m_web.bind("cppMesajGonder", [this](std::string req) -> std::string {
        const auto args = ParseJsonStringArray(req);
        if (args.size() >= 2) m_app.sendMessage(args[0], args[1]);
        return "null";
    });

    // args: [ip, yol1, yol2, ...]
    m_web.bind("cppTransferBaslat", [this](std::string req) -> std::string {
        const auto args = ParseJsonStringArray(req);
        if (args.size() >= 2) m_app.sendFiles(args[0], ToPaths(args, 1));
        return "null";
    });
}

// Ağ olayları UI thread'ine post edilip arayüzdeki fonksiyonlara iletilir
void UiBridge::bindEvents() {
    UiEvents events;

    events.onDeviceFound = [this](const std::string& ip, const std::string& name) {
        post("window.cihazEkle(" + JsonQuote(ip) + "," + JsonQuote(name) + ")");
    };
    events.onDeviceLost = [this](const std::string& ip) {
        post("window.cihazKayboldu(" + JsonQuote(ip) + ")");
    };
    events.onMessage = [this](const std::string& ip, const std::string& text) {
        post("window.mesajAl(" + JsonQuote(ip) + "," + JsonQuote(text) + ")");
    };
    events.onNotice = [this](const std::string& ip, const std::string& text) {
        post("window.bildirim(" + JsonQuote(ip) + "," + JsonQuote(text) + ")");
    };
    events.onTransfer = [this](const TransferInfo& info) {
        const char* stage = info.stage == TransferInfo::Stage::Progress ? "ilerleme"
                          : info.stage == TransferInfo::Stage::Done     ? "tamam"
                                                                        : "hata";
        post("window.transferDurumu(" + JsonQuote(info.ip) + "," + (info.incoming ? "true" : "false") + "," +
             JsonQuote(info.fileName) + "," + std::to_string(info.percent) + "," + JsonQuote(stage) + "," +
             std::to_string(info.index) + "," + std::to_string(info.total) + "," + JsonQuote(info.detail) + "," +
             (info.batchEnd ? "true" : "false") + ")");
    };

    m_app.setUiEvents(std::move(events));
}

void UiBridge::post(std::string js) {
    m_web.dispatch([this, js = std::move(js)] { m_web.eval(js); });
}

void UiBridge::showError(const std::string& text) {
    MessageBoxW(m_window.handle(), WideFromUtf8(text).c_str(), kTitle, MB_ICONERROR);
}
