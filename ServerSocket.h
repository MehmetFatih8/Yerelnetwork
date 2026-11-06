#pragma once

#include <asio.hpp>
#include <string>
#include <unordered_map>

#include "NetEvents.h"
#include "PeerConnection.h"

// Gelen TCP bağlantılarını kabul eder. Aynı IP'den gelen mesaj ve dosya soketlerini eşleştirip
// bir PeerConnection altında birleştirir.
class ServerSocket {
public:
    ServerSocket(asio::io_context& io, NetEventsPtr events, unsigned short mainPort, unsigned short filePort);

    void start();

private:
    void acceptLoop(asio::ip::tcp::acceptor& acceptor, bool isMain);

    asio::io_context& m_io;
    NetEventsPtr m_events;
    asio::ip::tcp::acceptor m_mainAcceptor;
    asio::ip::tcp::acceptor m_fileAcceptor;
    std::unordered_map<std::string, PeerConnection::Pointer> m_pending;
};
