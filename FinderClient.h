//
// Created by MehmetFatih on 13.09.2026.
//

#ifndef YERELKESIF_FINDERCLIENT_H
#define YERELKESIF_FINDERCLIENT_H
#include "asio.hpp"


class FinderClient {
    public:
        FinderClient(asio::io_context& ioContext, int port);
        ~FinderClient();
        void start();
        void Match(const asio::ip::udp::endpoint& matchEndpoint);

    private:
        asio::ip::udp::socket m_socket;
        asio::ip::udp::endpoint m_endpoint;
        asio::steady_timer m_timer;
        void Find();

        std::string discoveryMsg = "P2P_DISCOVER_HELLO";
        std::string matchMsg = "P2P_MATCH_HELLO";
};


#endif //YERELKESIF_FINDERCLIENT_H
