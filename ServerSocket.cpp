//
// Created by MehmetFatih on 11.09.2026.
//

#include "ServerSocket.h"
#include "PeerConnection.h"
#include <iostream>


ServerSocket::ServerSocket(asio::io_context& ioContext, int port):
    m_ioContext(ioContext),
    m_acceptor(m_ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)){}

ServerSocket::~ServerSocket() {
    std::error_code ec;
    m_acceptor.close(ec);
}

void ServerSocket::start() {
    startAccept();
}

void ServerSocket::startAccept() {
    auto newConnection = PeerConnection::create(m_ioContext);

    m_acceptor.async_accept(
        newConnection->getSocket(),
        [this, newConnection](const std::error_code& error) {

            if (!error) {

                newConnection->startListening();
                std::cout << "Server tarafından yeni bağlantı geldi ve sınıfa devredildi." << std::endl;

            }

            startAccept();

        });
}
