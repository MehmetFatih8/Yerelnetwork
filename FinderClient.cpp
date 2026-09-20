//
// Created by MehmetFatih on 13.09.2026.
//

#include "FinderClient.h"

#include <iostream>


FinderClient::FinderClient(asio::io_context& ioContext, int port):
    m_socket(ioContext),
    m_endpoint(asio::ip::address_v4::broadcast(), port),
    m_timer(ioContext){

        try {
            m_socket.open(asio::ip::udp::v4());
            m_socket.set_option(asio::socket_base::broadcast(true));
        }   catch (std::exception& e) {
            std::cerr << "FinderClient tarafında tanım kısmında hata: " << e.what() << std::endl;
        }
}

FinderClient::~FinderClient() {
    if (m_socket.is_open()) {
        m_socket.close();
    }
}

void FinderClient::start() {
    username = getSystemUsername();
    Find();
}

void FinderClient::Find() {

    std::string text = discoveryMsg + ":" + username;

    m_socket.async_send_to(asio::buffer(text), m_endpoint,
        [this](const std::error_code& ec, std::size_t bytes_transferred){

            if (!ec) {

                //std::cout << "FinderClient tarafında keşif mesajı gönderildi." << std::endl;

                m_timer.expires_after(std::chrono::seconds(3));
                m_timer.async_wait([this](const std::error_code& timerec) {

                    if (!timerec) {
                        Find();
                    }

                });


            } else {
                std::cerr << "FinderClient tarafında keşif anında hata: " << ec.message() << std::endl;
            }
        }

    );

}

void FinderClient::Match(const asio::ip::udp::endpoint& matchEndpoint) {

    asio::ip::udp::endpoint endpoint(matchEndpoint.address(), m_endpoint.port());

    std::string text = matchMsg + ":" + username;

    m_socket.async_send_to(asio::buffer(text), endpoint,
        [](const std::error_code& ec, std::size_t bytes_transferred) {

        });

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

std::string FinderClient::getSystemUsername() {
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
