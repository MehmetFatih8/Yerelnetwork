//
// Created by MehmetFatih on 13.09.2026.
//

#include "FinderServer.h"

#include <iostream>

#include "FinderClient.h"
#include "Logger.h"


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


                //std::cout << ip_port << "'tan mesaj: " << std::string_view(m_buffer.data(), bytesTransferred) << std::endl;

                if (std::string_view(m_buffer.data(), bytesTransferred).starts_with(discoveryMsg)) {
                    m_finderClient.Match(m_endpoint);

                }

                if (std::string_view(m_buffer.data(), bytesTransferred).starts_with(matchMsg)) {

                    bool is_newdevice = false;

                    {
                        std::lock_guard<std::mutex> lock(devicesMutex);
                        if (!devices.contains(ip)) {

                            is_newdevice = true;

                            std::string text = std::string(m_buffer.data(), bytesTransferred);
                            size_t colonPos = text.find(':');

                            std::string username = text.substr(colonPos + 1);
                            //std::cout << username << std::endl;
                            devices[ip] = username;
                            //std::cout << ip << " -> " << devices.find(ip)->second << std::endl;

                        }
                    }

                    if (m_onDeviceDiscovered && is_newdevice) {
                        m_onDeviceDiscovered(ip);
                    }

                }


            } else {
                LOG() << "ERR -> FinderServer tarafında hata: " << ec.message();
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

                LOG() << "Garip ip: " << ipStr;

            }

        }

    } catch (const std::exception& e) {
        LOG() << "ERR -> Local IP bulunamadı: " << e.what();
    }

    return "Bulunamadı.";
}

