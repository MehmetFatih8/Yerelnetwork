//
// Created by MehmetFatih on 13.09.2026.
//

#ifndef YERELKESIF_FINDERSERVER_H
#define YERELKESIF_FINDERSERVER_H
#include <string>
#include <unordered_map>
#include <asio.hpp>

#include "FinderClient.h"


class FinderServer {
    private:
        asio::ip::udp::socket m_socket;
        std::vector<char> m_buffer;
        asio::ip::udp::endpoint m_endpoint;
        std::string ip_port;
        asio::ip::tcp::resolver m_resolver;
        void startListening();
        std::string getLocalIP();
        FinderClient& m_finderClient;
        std::function<void(const std::string& ip)> m_onDeviceDiscovered;


        std::string localip;
        std::string discoveryMsg = "P2P_DISCOVER_HELLO";
        std::string matchMsg = "P2P_MATCH_HELLO";

    public:
        FinderServer(asio::io_context& ioContext, int port, FinderClient& finderClient);
        ~FinderServer();

        void setOnDeviceDiscovered(std::function<void(const std::string& ip)> callback) {
            m_onDeviceDiscovered = std::move(callback);
        }
        void start();
        std::unordered_map<std::string, std::string> devices;

};


#endif //YERELKESIF_FINDERSERVER_H
