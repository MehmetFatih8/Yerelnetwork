//
// Created by MehmetFatih on 11.09.2026.
//

#include "ClientSocket.h"

#include <iostream>

ClientSocket::ClientSocket(asio::io_context& ioContext):
    m_ioContext(ioContext), m_socket(m_ioContext){}

ClientSocket::~ClientSocket() {
    if (m_socket.is_open()) {
        std::error_code ec;
        m_socket.close(ec);
    }
}

void ClientSocket::connect(const std::string& host, unsigned short port, std::function<void(bool)> onConnected) {
    auto endpoint = asio::ip::tcp::endpoint(asio::ip::make_address(host), port);

    m_socket.async_connect(endpoint, [this, self = shared_from_this(), onConnected](std::error_code ec) {

        if (!ec) {
            m_isConnected = true;
            std::cout << "Client tarafından bağlanıldı." << std::endl;
            if (onConnected) onConnected(true);
        } else {
            m_isConnected = false;
            std::cout << "Client tarafından bağlanılamadı: " << ec.message() << std::endl;
            if (onConnected) onConnected(false);
        }


    });
}

void ClientSocket::sendMessage(const std::string &message) {
    if (!m_isConnected) {
        std::cout << "Bağlantı yok mesaj gönderilemedi." << std::endl;
    }

    auto msgBuffer = std::make_shared<std::string>(message);

    asio::async_write(m_socket, asio::buffer(*msgBuffer),
        [this, self = shared_from_this(), msgBuffer](const std::error_code ec, std::size_t ) {
            if (ec) {
                std::cout << "Mesaj gönderim hatası: " << ec.message() << std::endl;
                m_isConnected = false;
            }
        });
}
