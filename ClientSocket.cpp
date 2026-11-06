#include "ClientSocket.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <string_view>

#include "Logger.h"
#include "Protocol.h"
#include "Text.h"

struct ClientSocket::OutgoingFile {
    std::filesystem::path path;
    std::string name;                 // UTF-8 dosya adı
    std::uint64_t size{0};
    std::uint64_t sent{0};
    std::size_t index{1};
    std::size_t total{1};
    std::ifstream in;
};

ClientSocket::ClientSocket(asio::io_context& io, NetEventsPtr events, std::string ip)
    : m_events(std::move(events)), m_ip(std::move(ip)), m_main(io), m_file(io) {}

void ClientSocket::connect(unsigned short mainPort, unsigned short filePort, std::function<void(bool)> onReady) {
    const auto address = asio::ip::make_address(m_ip);

    struct State {
        int pending{2};
        bool failed{false};
    };
    auto state = std::make_shared<State>();

    auto onOne = [this, self = shared_from_this(), state, onReady](const std::error_code& ec) {
        if (state->failed) return;
        if (ec) {
            state->failed = true;
            LOG() << "ERR -> " << m_ip << " bağlantısı kurulamadı: " << ec.message();
            close();
            onReady(false);
            return;
        }
        if (--state->pending == 0) {
            m_connected = true;
            onReady(true);
        }
    };

    m_main.async_connect(asio::ip::tcp::endpoint(address, mainPort), onOne);
    m_file.async_connect(asio::ip::tcp::endpoint(address, filePort), onOne);
}

void ClientSocket::close() {
    if (m_current) {
        m_files.clear();
        report(*m_current, TransferInfo::Stage::Failed, "Bağlantı kapatıldı.");
        m_current.reset();
    }
    m_files.clear();
    m_messages.clear();
    m_connected = false;

    std::error_code ec;
    m_main.close(ec);
    m_file.close(ec);
}

void ClientSocket::sendMessage(const std::string& text) {
    if (!m_connected) {
        m_events->onNotice(m_ip, "Mesaj gönderilemedi: bağlantı yok.");
        return;
    }
    if (text.size() > Protocol::kMaxMessageSize) {
        m_events->onNotice(m_ip, "Mesaj çok uzun, gönderilemedi.");
        return;
    }

    const bool idle = m_messages.empty();
    m_messages.push_back(std::make_shared<const std::string>(
        std::string(Protocol::kMsgPrefix) + std::to_string(text.size()) + ":" + text));
    if (idle) writeNextMessage();
}

// Mesajlar tek tek yazılır; böylece aynı soket üzerinde iki yazma işlemi çakışmaz
void ClientSocket::writeNextMessage() {
    if (m_messages.empty()) return;

    auto packet = m_messages.front();
    asio::async_write(m_main, asio::buffer(*packet),
        [this, self = shared_from_this(), packet](const std::error_code& ec, std::size_t) {
            if (m_messages.empty() || m_messages.front() != packet) return;   // Bu arada kapatıldı
            if (ec) {
                m_events->onNotice(m_ip, "Mesaj gönderilemedi: " + ec.message());
                close();
                return;
            }
            m_messages.pop_front();
            writeNextMessage();
        });
}

void ClientSocket::sendFiles(const std::vector<std::filesystem::path>& files) {
    if (files.empty()) return;

    if (!m_connected) {
        TransferInfo info;
        info.ip = m_ip;
        info.fileName = Utf8FromWide(files.front().filename().native());
        info.stage = TransferInfo::Stage::Failed;
        info.total = files.size();
        info.batchEnd = true;
        info.detail = "Cihazla bağlantı yok.";
        m_events->onTransfer(info);
        return;
    }

    for (std::size_t i = 0; i < files.size(); ++i) {
        m_files.push_back({files[i], i + 1, files.size()});
    }
    if (!m_current) startNextFile();
}

// Kuyruğun başındaki dosya için: istek -> onay -> veri. Dosya açılamazsa yalnızca o dosya atlanır.
void ClientSocket::startNextFile() {
    if (m_files.empty()) return;

    const PendingFile item = m_files.front();
    m_files.pop_front();

    auto file = std::make_shared<OutgoingFile>();
    file->path = item.path;
    file->name = Utf8FromWide(item.path.filename().native());
    file->index = item.index;
    file->total = item.total;

    std::error_code ec;
    file->size = std::filesystem::file_size(item.path, ec);
    file->in.open(item.path, std::ios::binary);
    if (ec || !file->in.is_open()) {
        LOG() << "ERR -> Dosya açılamadı: " << file->name;
        report(*file, TransferInfo::Stage::Failed, "Dosya açılamadı.");
        startNextFile();
        return;
    }

    m_current = file;
    sendRequest(file);
}

void ClientSocket::sendRequest(const std::shared_ptr<OutgoingFile>& file) {
    auto request = std::make_shared<const std::string>(
        std::string(Protocol::kFileReqPrefix) + file->name + ":" + std::to_string(file->size) + ":");

    asio::async_write(m_file, asio::buffer(*request),
        [this, self = shared_from_this(), file, request](const std::error_code& ec, std::size_t) {
            if (m_current != file) return;
            if (ec) {
                failBatch(file, "İstek gönderilemedi: " + ec.message());
                return;
            }
            waitForAck(file);
        });
}

void ClientSocket::waitForAck(const std::shared_ptr<OutgoingFile>& file) {
    auto reply = std::make_shared<std::array<char, 64>>();
    m_file.async_read_some(asio::buffer(*reply),
        [this, self = shared_from_this(), file, reply](const std::error_code& ec, std::size_t bytes) {
            if (m_current != file) return;
            if (ec) {
                failBatch(file, "Onay alınamadı: " + ec.message());
                return;
            }
            if (std::string_view(reply->data(), bytes).starts_with(Protocol::kFileAck)) {
                sendChunk(file);
                return;
            }
            finishFile(file, false, "Karşı taraf dosyayı reddetti.");
        });
}

void ClientSocket::sendChunk(const std::shared_ptr<OutgoingFile>& file) {
    auto chunk = std::make_shared<std::vector<char>>(Protocol::kChunkSize);
    file->in.read(chunk->data(), static_cast<std::streamsize>(chunk->size()));
    const auto bytes = static_cast<std::size_t>(file->in.gcount());
    if (bytes == 0) {
        finishFile(file, true, "");
        return;
    }
    chunk->resize(bytes);

    asio::async_write(m_file, asio::buffer(*chunk),
        [this, self = shared_from_this(), file, chunk](const std::error_code& ec, std::size_t written) {
            if (m_current != file) return;
            if (ec) {
                failBatch(file, "Veri gönderilemedi: " + ec.message());
                return;
            }
            file->sent += written;
            report(*file, TransferInfo::Stage::Progress, "");
            sendChunk(file);
        });
}

// Tek dosyanın sonucu. Başarısız olsa da kuyruktaki sonraki dosyalara geçilir.
void ClientSocket::finishFile(std::shared_ptr<OutgoingFile> file, bool ok, const std::string& reason) {
    report(*file, ok ? TransferInfo::Stage::Done : TransferInfo::Stage::Failed, ok ? file->name : reason);
    file->in.close();
    m_current.reset();
    startNextFile();
}

// Bağlantı düzeyinde hata: kuyruk iptal edilir ve soketler kapatılır (sonraki keşifte yeniden bağlanılır)
void ClientSocket::failBatch(std::shared_ptr<OutgoingFile> file, const std::string& reason) {
    LOG() << "ERR -> Dosya gönderimi başarısız (" << file->name << "): " << reason;
    m_files.clear();
    report(*file, TransferInfo::Stage::Failed, reason);
    m_current.reset();
    close();
}

void ClientSocket::report(const OutgoingFile& file, TransferInfo::Stage stage, const std::string& detail) {
    TransferInfo info;
    info.ip = m_ip;
    info.fileName = file.name;
    info.incoming = false;
    info.stage = stage;
    info.index = file.index;
    info.total = file.total;
    info.batchEnd = stage != TransferInfo::Stage::Progress && m_files.empty();
    info.detail = detail;

    if (stage == TransferInfo::Stage::Done) {
        info.percent = 100.0;
    } else if (file.size > 0) {
        info.percent = 100.0 * static_cast<double>(file.sent) / static_cast<double>(file.size);
    }
    m_events->onTransfer(info);
}
