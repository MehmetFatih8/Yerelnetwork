#pragma once

#include <asio.hpp>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "FinderClient.h"
#include "NetEvents.h"

// UDP keşif portunu dinler. Keşif isteğine FinderClient ile yanıt verir,
// eşleşme yanıtlarını NetEvents::onMatch ile denetleyiciye bildirir.
class FinderServer {
public:
    FinderServer(asio::io_context& io, unsigned short port, FinderClient& finderClient, NetEventsPtr events);

    void start();

private:
    void startListening();
    void handleDatagram(std::string_view text, const asio::ip::udp::endpoint& sender);

    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_sender;
    std::vector<char> m_buffer;
    std::set<std::string> m_localIps;   // Kendi paketlerimizi ayıklamak için yerel IPv4 adresleri
    FinderClient& m_finderClient;
    NetEventsPtr m_events;
};
