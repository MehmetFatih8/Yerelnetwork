//
// Created by MehmetFatih on 13.09.2026.
//

#include "FinderServer.h"

#include <iostream>

#include "FinderClient.h"


FinderServer::FinderServer(asio::io_context& ioContext, int port, FinderClient& finderClient):
    m_socket(ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)),
    m_buffer(1024),
    m_resolver(ioContext),
    m_finderClient(finderClient){}


void FinderServer::start() {
    localip = getLocalIP();
    startListening();
}

FinderServer::~FinderServer() {
    if (m_socket.is_open()) {
        m_socket.cancel();
        m_socket.close();
    }
}


void FinderServer::startListening() {

    m_socket.async_receive_from(asio::buffer(m_buffer),m_endpoint,
        [this](const std::error_code& ec , std::size_t bytesTransferred) {

            if (!ec) {

                const std::string ip = m_endpoint.address().to_string();


                if (ip == localip || ip == "127.0.0.1") {

                    //std::cout << "Bizim ipden mesaj geldi." << std::endl;
                    startListening();
                    return;

                }

                // if (ip == "127.0.0.1") {
                //
                //     startListening();
                //     return;
                //
                // }

                //std::cout << ip_port << "'tan mesaj: " << std::string_view(m_buffer.data(), bytesTransferred) << std::endl;

                if (std::string_view(m_buffer.data(), bytesTransferred).starts_with(discoveryMsg)) {
                    m_finderClient.Match(m_endpoint);

                }

                if (std::string_view(m_buffer.data(), bytesTransferred).starts_with(matchMsg)) {

                    if (!devices.contains(ip)) {

                        std::string text = std::string(m_buffer.data(), bytesTransferred);
                        size_t colonPos = text.find(':');

                        std::string username = text.substr(colonPos + 1);
                        //std::cout << username << std::endl;
                        devices[ip] = username;
                        //std::cout << ip << " -> " << devices.find(ip)->second << std::endl;

                        if (m_onDeviceDiscovered) {
                            m_onDeviceDiscovered(ip);
                        }

                    }

                }


            } else {
                std::cerr << "FinderServer tarafında hata: " << ec.message() << std::endl;
            }

            startListening();

        });

}


std::string FinderServer::getLocalIP() {
    try {

        auto endpoints = m_resolver.resolve(asio::ip::host_name(), "");

        for (const auto& entry : endpoints) {
            auto addr = entry.endpoint().address();

            if (addr.is_v4() && !addr.is_loopback()) {

                std::string ipStr = addr.to_string();

                //192.168.56 VM'lerin default ipsi
                if (ipStr.rfind("192.168", 0) == 0 && ipStr.rfind("192.168.56.", 0) != 0) {
                    return ipStr;
                }

                if (ipStr.rfind("10.", 0) == 0) {
                    return ipStr;
                }

                std::cout << "Garip ip: " << ipStr << std::endl;


            }

        }

    } catch (const std::exception& e) {

        std::cerr << "Local IP bulunamadı: " << e.what() << std::endl;

    }

    return "Bulunamadı.";
}

