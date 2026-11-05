//
// Created by MehmetFatih on 11.09.2026.
//

#include "ServerSocket.h"
#include "PeerConnection.h"
#include <iostream>

#include "Logger.h"


ServerSocket::ServerSocket(asio::io_context& ioContext, int main_port, int file_port):
    m_ioContext(ioContext),
    m_mainacceptor(m_ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), main_port)),
    m_fileacceptor(m_ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), file_port)){}

ServerSocket::~ServerSocket() {
    std::error_code ec;
    m_mainacceptor.close(ec);
    if (ec) {
        LOG() << "ERR -> ServerSocket kapatılırken hata oluştu: " << ec.message();
    }
    m_fileacceptor.close(ec);
    if (ec) {
        LOG() << "ERR -> ServerSocket kapatılırken hata oluştu: " << ec.message();
    }
}

void ServerSocket::start() {
    mainAccept();
    fileAccept();
}

void ServerSocket::mainAccept() {

    auto sock = std::make_shared<asio::ip::tcp::socket>(m_ioContext);
    m_mainacceptor.async_accept(*sock, [this, sock](const asio::error_code& ec) {
        if (!ec) {
            std::string ip = sock->remote_endpoint().address().to_string();
            auto& peer = m_pending[ip];
            if (!peer) {
                peer = PeerConnection::create(m_ioContext);
            }

            peer->set_mainsocket(std::move(*sock));
            if (peer->isReady()) {
                peer->startListening();
                m_pending.erase(ip);
            }

        }

        mainAccept();
    });
}

void ServerSocket::fileAccept() {

    auto sock = std::make_shared<asio::ip::tcp::socket>(m_ioContext);
    m_fileacceptor.async_accept(*sock, [this, sock](const asio::error_code& ec) {
        if (!ec) {
            std::string ip = sock->remote_endpoint().address().to_string();
            auto& peer = m_pending[ip];
            if (!peer) {
                peer = PeerConnection::create(m_ioContext);
            }

            peer->set_filesocket(std::move(*sock));
            if (peer->isReady()) {
                peer->startListening();
                m_pending.erase(ip);
            }

        }

        fileAccept();
    });
}


/*auto newConnection = PeerConnection::create(m_ioContext);

    m_mainacceptor.async_accept(
        newConnection->get_mainSocket(),
        [this, newConnection](const std::error_code& error) {

            if (!error) {

                newConnection->startListening();
                //std::cout << "Server tarafında yeni bağlantı geldi ve sınıfa devredildi." << std::endl;

            }

            startAccept();

        });

    m_fileacceptor.async_accept(
        newConnection->get_fileSocket(),
        [this, newConnection](const std::error_code& error) {

            if (!error) {

                newConnection->startListening();
                //std::cout << "Server tarafında yeni dosya bağlantısı geldi ve sınıfa devredildi." << std::endl;

            }

            startAccept();

        });*/
