#pragma once

#include <asio.hpp>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "NetEvents.h"

// Bir cihaza giden TCP bağlantısı: mesajlar ana soket, dosyalar dosya soketi üzerinden gider.
// Tüm çağrılar io thread'inde yapılmalıdır (AppController bunu sağlar).
class ClientSocket : public std::enable_shared_from_this<ClientSocket> {
public:
    using Pointer = std::shared_ptr<ClientSocket>;

    static Pointer create(asio::io_context& io, NetEventsPtr events, std::string ip) {
        return Pointer(new ClientSocket(io, std::move(events), std::move(ip)));
    }

    // onReady her durumda tam bir kez çağrılır: iki soket de bağlandıysa true
    void connect(unsigned short mainPort, unsigned short filePort, std::function<void(bool)> onReady);
    bool isConnected() const { return m_connected; }

    void sendMessage(const std::string& text);
    void sendFiles(const std::vector<std::filesystem::path>& files);
    void close();

private:
    struct PendingFile {
        std::filesystem::path path;
        std::size_t index;
        std::size_t total;
    };
    struct OutgoingFile;

    ClientSocket(asio::io_context& io, NetEventsPtr events, std::string ip);

    void writeNextMessage();
    void startNextFile();
    void sendRequest(const std::shared_ptr<OutgoingFile>& file);
    void waitForAck(const std::shared_ptr<OutgoingFile>& file);
    void sendChunk(const std::shared_ptr<OutgoingFile>& file);
    void finishFile(std::shared_ptr<OutgoingFile> file, bool ok, const std::string& reason);
    void failBatch(std::shared_ptr<OutgoingFile> file, const std::string& reason);
    void report(const OutgoingFile& file, TransferInfo::Stage stage, const std::string& detail);

    NetEventsPtr m_events;
    std::string m_ip;
    asio::ip::tcp::socket m_main;
    asio::ip::tcp::socket m_file;
    bool m_connected{false};
    std::deque<std::shared_ptr<const std::string>> m_messages;   // Sıradaki ilk öğe yazılmakta olan paket
    std::deque<PendingFile> m_files;
    std::shared_ptr<OutgoingFile> m_current;
};
