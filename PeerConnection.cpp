#include "PeerConnection.h"

#include <algorithm>
#include <charconv>

#include <windows.h>
#include <objbase.h>
#include <shlobj.h>

#include "Logger.h"
#include "Protocol.h"
#include "Text.h"

namespace {

// Alınan dosyalar Downloads\Yerelkesif altına yazılır
std::filesystem::path ReceiveDirectory() {
    std::filesystem::path base = std::filesystem::temp_directory_path();
    PWSTR folder = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &folder))) {
        base = folder;
        CoTaskMemFree(folder);
    }

    std::filesystem::path dir = base / L"Yerelkesif";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

// Karşı taraftan gelen ad yol bileşenlerini içerebilir; yalnızca dosya adı kullanılır
std::filesystem::path SafeFileName(const std::string& utf8Name) {
    auto name = PathFromUtf8(utf8Name).filename();
    if (name.empty() || name == L"." || name == L"..") name = L"dosya";
    return name;
}

// Aynı ada sahip dosya varsa "ad (1).uzanti" biçiminde yeni bir ad üretir
std::filesystem::path UniquePath(const std::filesystem::path& dir, const std::filesystem::path& name) {
    std::filesystem::path target = dir / name;
    std::error_code ec;
    for (int i = 1; std::filesystem::exists(target, ec); ++i) {
        target = dir / (name.stem().native() + L" (" + std::to_wstring(i) + L")" + name.extension().native());
    }
    return target;
}

bool ParseNumber(std::string_view digits, std::uint64_t& value) {
    if (digits.empty()) return false;
    const char* end = digits.data() + digits.size();
    const auto result = std::from_chars(digits.data(), end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

}  // namespace

PeerConnection::PeerConnection(asio::io_context& io, NetEventsPtr events, std::string ip)
    : m_events(std::move(events)),
      m_ip(std::move(ip)),
      m_main(io),
      m_file(io),
      m_mainChunk(Protocol::kChunkSize),
      m_fileChunk(Protocol::kChunkSize) {}

void PeerConnection::startListening() {
    readMain();
    readFile();
}

void PeerConnection::readMain() {
    m_main.async_read_some(asio::buffer(m_mainChunk),
        [this, self = shared_from_this()](const std::error_code& ec, std::size_t bytes) {
            if (ec) {
                handleLost(ec);
                return;
            }
            m_mainBuffer.append(m_mainChunk.data(), bytes);
            parseMain();
            readMain();
        });
}

void PeerConnection::readFile() {
    m_file.async_read_some(asio::buffer(m_fileChunk),
        [this, self = shared_from_this()](const std::error_code& ec, std::size_t bytes) {
            if (ec) {
                handleLost(ec);
                return;
            }
            m_fileBuffer.append(m_fileChunk.data(), bytes);
            parseFile();
            readFile();
        });
}

// "MSG:<bayt>:<metin>" çerçevelerini tamponda çözer; tam gelmemiş çerçeve bekler
void PeerConnection::parseMain() {
    while (true) {
        const std::string_view view(m_mainBuffer);
        const std::string_view prefix = Protocol::kMsgPrefix;

        if (!view.starts_with(prefix)) {
            if (prefix.starts_with(view)) return;   // Ön ek henüz tamamlanmadı
            LOG() << "ERR -> Geçersiz mesaj çerçevesi (" << m_ip << "), tampon temizlendi.";
            m_mainBuffer.clear();
            return;
        }

        const auto colon = view.find(':', prefix.size());
        if (colon == std::string_view::npos) {
            if (view.size() > Protocol::kMaxHeaderSize) m_mainBuffer.clear();
            return;
        }

        std::uint64_t length = 0;
        if (!ParseNumber(view.substr(prefix.size(), colon - prefix.size()), length) ||
            length > Protocol::kMaxMessageSize) {
            LOG() << "ERR -> Geçersiz mesaj uzunluğu (" << m_ip << "), tampon temizlendi.";
            m_mainBuffer.clear();
            return;
        }

        const std::size_t frameSize = colon + 1 + static_cast<std::size_t>(length);
        if (view.size() < frameSize) return;   // Gövde henüz tamamlanmadı

        std::string text(view.substr(colon + 1, static_cast<std::size_t>(length)));
        m_mainBuffer.erase(0, frameSize);
        m_events->onMessage(m_ip, text);
    }
}

// Önce "FILE_REQ:<ad>:<bayt>:" başlığını çözer, ardından gelen veriyi dosyaya yazar
void PeerConnection::parseFile() {
    while (!m_fileBuffer.empty()) {
        if (m_receiving) {
            const auto remaining = m_expected - m_received;
            const auto take = static_cast<std::size_t>(std::min<std::uint64_t>(m_fileBuffer.size(), remaining));
            m_out.write(m_fileBuffer.data(), static_cast<std::streamsize>(take));
            m_fileBuffer.erase(0, take);
            m_received += take;

            if (!m_out) {
                abortReceive("Dosya diske yazılamadı.");
                return;
            }
            notifyIncoming(TransferInfo::Stage::Progress, "");
            if (m_received == m_expected) finishReceive();
            continue;
        }

        const std::string_view view(m_fileBuffer);
        const std::string_view prefix = Protocol::kFileReqPrefix;
        if (!view.starts_with(prefix)) {
            if (prefix.starts_with(view)) return;   // Başlık henüz tamamlanmadı
            LOG() << "ERR -> Geçersiz dosya çerçevesi (" << m_ip << "), tampon temizlendi.";
            m_fileBuffer.clear();
            return;
        }

        const auto nameEnd = view.find(':', prefix.size());
        const auto sizeEnd = nameEnd == std::string_view::npos ? std::string_view::npos
                                                               : view.find(':', nameEnd + 1);
        if (sizeEnd == std::string_view::npos) {
            if (view.size() > Protocol::kMaxHeaderSize) m_fileBuffer.clear();
            return;
        }

        const std::string name(view.substr(prefix.size(), nameEnd - prefix.size()));
        std::uint64_t size = 0;
        if (name.empty() || !ParseNumber(view.substr(nameEnd + 1, sizeEnd - nameEnd - 1), size)) {
            LOG() << "ERR -> Geçersiz dosya isteği (" << m_ip << ").";
            m_fileBuffer.clear();
            return;
        }

        m_fileBuffer.erase(0, sizeEnd + 1);
        beginReceive(name, size);
    }
}

void PeerConnection::beginReceive(const std::string& utf8Name, std::uint64_t size) {
    const auto name = SafeFileName(utf8Name);
    m_outPath = UniquePath(ReceiveDirectory(), name);
    m_outName = Utf8FromWide(name.native());
    m_expected = size;
    m_received = 0;

    m_out.open(m_outPath, std::ios::binary);
    if (!m_out.is_open()) {
        LOG() << "ERR -> Dosya oluşturulamadı: " << Utf8FromWide(m_outPath.native());
        sendFileReply(Protocol::kFileErr);
        notifyIncoming(TransferInfo::Stage::Failed, "Dosya oluşturulamadı.");
        return;
    }

    m_receiving = true;
    LOG() << "INFO -> Dosya alımı başladı: " << m_outName << " (" << m_expected << " bayt)";
    sendFileReply(Protocol::kFileAck);

    if (m_expected == 0) finishReceive();
}

void PeerConnection::finishReceive() {
    m_out.close();
    m_receiving = false;
    LOG() << "INFO -> Dosya alındı: " << Utf8FromWide(m_outPath.native());
    notifyIncoming(TransferInfo::Stage::Done, Utf8FromWide(m_outPath.native()));
}

void PeerConnection::abortReceive(const std::string& reason) {
    m_out.close();
    std::error_code ec;
    std::filesystem::remove(m_outPath, ec);   // Yarım kalan dosyayı bırakma
    m_receiving = false;
    LOG() << "ERR -> Dosya alımı iptal edildi (" << m_outName << "): " << reason;
    notifyIncoming(TransferInfo::Stage::Failed, reason);
}

void PeerConnection::notifyIncoming(TransferInfo::Stage stage, const std::string& detail) {
    TransferInfo info;
    info.ip = m_ip;
    info.fileName = m_outName;
    info.incoming = true;
    info.stage = stage;
    info.batchEnd = stage != TransferInfo::Stage::Progress;
    info.detail = detail;
    if (stage == TransferInfo::Stage::Done) {
        info.percent = 100.0;
    } else if (m_expected > 0) {
        info.percent = 100.0 * static_cast<double>(m_received) / static_cast<double>(m_expected);
    }
    m_events->onTransfer(info);
}

void PeerConnection::sendFileReply(std::string_view reply) {
    auto packet = std::make_shared<std::string>(reply);
    asio::async_write(m_file, asio::buffer(*packet),
        [packet](const std::error_code& ec, std::size_t) {
            if (ec) LOG() << "ERR -> Dosya yanıtı gönderilemedi: " << ec.message();
        });
}

void PeerConnection::handleLost(const std::error_code& ec) {
    if (m_lost) return;
    m_lost = true;

    LOG() << "INFO -> " << m_ip << " bağlantısı kapandı: " << ec.message();
    std::error_code ignored;
    m_main.close(ignored);
    m_file.close(ignored);

    if (m_receiving) abortReceive("Bağlantı koptu.");
    m_events->onPeerClosed(m_ip);
}
