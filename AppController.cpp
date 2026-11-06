#include "AppController.h"

#include <chrono>
#include <exception>

#include "Logger.h"
#include "Protocol.h"

AppController::AppController()
    : m_work(asio::make_work_guard(m_io)),
      m_finderClient(m_io, Protocol::kDiscoveryPort) {
    auto net = std::make_shared<NetEvents>();
    net->onMatch = [this](const std::string& ip, const std::string& name) { onMatch(ip, name); };
    net->onPeerClosed = [this](const std::string& ip) { onPeerClosed(ip); };
    net->onMessage = [this](const std::string& ip, const std::string& text) {
        if (m_ui.onMessage) m_ui.onMessage(ip, text);
    };
    net->onTransfer = [this](const TransferInfo& info) {
        if (m_ui.onTransfer) m_ui.onTransfer(info);
    };
    net->onNotice = [this](const std::string& ip, const std::string& text) {
        if (m_ui.onNotice) m_ui.onNotice(ip, text);
    };
    m_net = std::move(net);
}

AppController::~AppController() {
    stop();
}

void AppController::setUiEvents(UiEvents events) {
    m_ui = std::move(events);
}

void AppController::start() {
    if (m_running) return;

    m_server = std::make_unique<ServerSocket>(m_io, m_net, Protocol::kMainPort, Protocol::kFilePort);
    m_server->start();

    m_finderServer = std::make_unique<FinderServer>(m_io, Protocol::kDiscoveryPort, m_finderClient, m_net);
    m_finderServer->start();
    m_finderClient.start();

    m_thread = std::thread([this] {
        try {
            m_io.run();
        } catch (const std::exception& e) {
            LOG() << "ERR -> Ağ thread'i durdu: " << e.what();
        }
    });
    m_running = true;
    LOG() << "INFO -> Ağ servisleri başlatıldı.";
}

void AppController::stop() {
    if (!m_running) return;
    m_running = false;

    m_work.reset();
    m_io.stop();
    if (m_thread.joinable()) m_thread.join();

    // io thread durduğu için nesneler artık yalnızca bu thread'den erişiliyor
    m_clients.clear();
    m_devices.clear();
    m_finderServer.reset();
    m_server.reset();
}

std::string AppController::validateIp(const std::string& ip) {
    std::error_code ec;
    const auto address = asio::ip::make_address(ip, ec);
    if (ec || !address.is_v4() || address.is_unspecified() || address.is_multicast() || address.is_loopback()) {
        return "Geçerli bir IPv4 adresi girin.";
    }
    return {};
}

void AppController::addDevice(const std::string& ip) {
    asio::post(m_io, [this, ip] {
        m_finderClient.Probe(ip);

        // Cihaz yanıt vermezse arayüzü bilgilendir
        auto timer = std::make_shared<asio::steady_timer>(m_io, std::chrono::seconds(5));
        timer->async_wait([this, ip, timer](const std::error_code& ec) {
            if (!ec && !m_devices.contains(ip) && m_ui.onNotice) {
                m_ui.onNotice(ip, "Cihaz yanıt vermedi: " + ip);
            }
        });
    });
}

void AppController::sendMessage(const std::string& ip, const std::string& text) {
    asio::post(m_io, [this, ip, text] {
        const auto found = m_clients.find(ip);
        if (found == m_clients.end()) {
            if (m_ui.onNotice) m_ui.onNotice(ip, "Mesaj gönderilemedi: cihaz bağlı değil.");
            return;
        }
        found->second->sendMessage(text);
    });
}

void AppController::sendFiles(const std::string& ip, std::vector<std::filesystem::path> files) {
    asio::post(m_io, [this, ip, files = std::move(files)] {
        const auto found = m_clients.find(ip);
        if (found == m_clients.end()) {
            reportTransferFailure(ip, "Cihaz bağlı değil.");
            return;
        }
        found->second->sendFiles(files);
    });
}

void AppController::onMatch(const std::string& ip, const std::string& name) {
    auto& known = m_devices[ip];
    if (known != name) {
        known = name;
        if (m_ui.onDeviceFound) m_ui.onDeviceFound(ip, name);
    }
    connectTo(ip);
}

void AppController::onPeerClosed(const std::string& ip) {
    if (const auto found = m_clients.find(ip); found != m_clients.end()) {
        found->second->close();
        m_clients.erase(found);
    }
    if (m_devices.erase(ip) > 0 && m_ui.onDeviceLost) {
        m_ui.onDeviceLost(ip);
    }
}

// Cihaz zaten bağlıysa yeniden bağlanılmaz; bağlantı kurulamadıysa bir sonraki keşifte tekrar denenir
void AppController::connectTo(const std::string& ip) {
    if (const auto found = m_clients.find(ip); found != m_clients.end()) {
        if (found->second->isConnected()) return;
        found->second->close();
    }

    auto client = ClientSocket::create(m_io, m_net, ip);
    m_clients[ip] = client;
    client->connect(Protocol::kMainPort, Protocol::kFilePort, [ip](bool ok) {
        if (ok) {
            LOG() << "INFO -> " << ip << " ile bağlantı kuruldu.";
        } else {
            LOG() << "ERR -> " << ip << " ile bağlantı kurulamadı.";
        }
    });
}

void AppController::reportTransferFailure(const std::string& ip, const std::string& reason) {
    if (!m_ui.onTransfer) return;

    TransferInfo info;
    info.ip = ip;
    info.stage = TransferInfo::Stage::Failed;
    info.batchEnd = true;
    info.detail = reason;
    m_ui.onTransfer(info);
}
