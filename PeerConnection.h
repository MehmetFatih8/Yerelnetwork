#pragma once

#include <asio.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "NetEvents.h"

// Bir cihazdan gelen iki TCP bağlantısını (mesaj ve dosya) birleştirir ve gelen çerçeveleri çözer.
class PeerConnection : public std::enable_shared_from_this<PeerConnection> {
public:
    using Pointer = std::shared_ptr<PeerConnection>;

    static Pointer create(asio::io_context& io, NetEventsPtr events, std::string ip) {
        return Pointer(new PeerConnection(io, std::move(events), std::move(ip)));
    }

    void setMainSocket(asio::ip::tcp::socket&& socket) { m_main = std::move(socket); }
    void setFileSocket(asio::ip::tcp::socket&& socket) { m_file = std::move(socket); }
    bool isReady() const { return m_main.is_open() && m_file.is_open(); }

    void startListening();

private:
    PeerConnection(asio::io_context& io, NetEventsPtr events, std::string ip);

    void readMain();
    void readFile();
    void parseMain();
    void parseFile();
    void beginReceive(const std::string& utf8Name, std::uint64_t size);
    void finishReceive();
    void abortReceive(const std::string& reason);
    void notifyIncoming(TransferInfo::Stage stage, const std::string& detail);
    void sendFileReply(std::string_view reply);
    void handleLost(const std::error_code& ec);

    NetEventsPtr m_events;
    std::string m_ip;
    asio::ip::tcp::socket m_main;
    asio::ip::tcp::socket m_file;
    std::vector<char> m_mainChunk;
    std::vector<char> m_fileChunk;
    std::string m_mainBuffer;   // Parçalı gelebilen mesaj çerçeveleri için birikim tamponu
    std::string m_fileBuffer;

    // Gelen dosyanın durumu
    std::ofstream m_out;
    std::filesystem::path m_outPath;
    std::string m_outName;
    std::uint64_t m_expected{0};
    std::uint64_t m_received{0};
    bool m_receiving{false};
    bool m_lost{false};
};
