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

                    std::cout << "Bizim ipden mesaj geldi." << std::endl;
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

                if (std::string_view(m_buffer.data(), bytesTransferred) == discoveryMsg) {

                    if (!devices.contains(ip)) {
                        m_finderClient.Match(m_endpoint);
                    }

                }

                if (std::string_view(m_buffer.data(), bytesTransferred) == matchMsg) {

                    devices[ip] = getSystemUsername();
                    //std::cout << ip << " -> " << devices.find(ip)->second << std::endl;

                    if (m_onDeviceDiscovered) {
                        m_onDeviceDiscovered(ip);
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


#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

std::string FinderServer::getSystemUsername() {
#if defined(_WIN32)
    // 1. Windows API (GetUserNameA) ile oturum açmış kullanıcı adını al
    char username[256];
    DWORD size = sizeof(username);
    if (GetUserNameA(username, &size)) {
        return std::string(username);
    }

    // 2. Başarısız olursa USERNAME ortam değişkenini kontrol et
    const char* envUser = std::getenv("USERNAME");
    if (envUser != nullptr && envUser[0] != '\0') {
        return std::string(envUser);
    }

#else
    // 1. Linux POSIX API (getpwuid_r) ile kullanıcı adını al
    uid_t uid = geteuid();
    struct passwd pwd;
    struct passwd* result = nullptr;
    char buffer[1024];

    if (getpwuid_r(uid, &pwd, buffer, sizeof(buffer), &result) == 0 && result != nullptr) {
        if (result->pw_name != nullptr && result->pw_name[0] != '\0') {
            return std::string(result->pw_name);
        }
    }

    // 2. Başarısız olursa getlogin() veya ortam değişkenlerini kontrol et
    char* loginName = getlogin();
    if (loginName != nullptr && loginName[0] != '\0') {
        return std::string(loginName);
    }

    const char* envUser = std::getenv("USER");
    if (envUser == nullptr || envUser[0] == '\0') {
        envUser = std::getenv("LOGNAME");
    }

    if (envUser != nullptr && envUser[0] != '\0') {
        return std::string(envUser);
    }
#endif

    return "UnknownUser";
}