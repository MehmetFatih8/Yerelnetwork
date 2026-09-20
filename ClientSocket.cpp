//
// Created by MehmetFatih on 11.09.2026.
//

#include "ClientSocket.h"
#include <fstream>
#include <iostream>

ClientSocket::ClientSocket(asio::io_context& ioContext):
    m_ioContext(ioContext), m_socket(m_ioContext){}

ClientSocket::~ClientSocket() {
    if (m_socket.is_open()) {
        std::error_code ec;
        m_socket.close(ec);
        if (ec) {
            std::cout << "Client tarafında socket kapatılamadı." << std::endl;
        }
    }
}

void ClientSocket::connect(const std::string& host, unsigned short port, std::function<void(bool)> onConnected) {
    auto endpoint = asio::ip::tcp::endpoint(asio::ip::make_address(host), port);

    m_socket.async_connect(endpoint, [this, self = shared_from_this(), onConnected](std::error_code ec) {

        if (!ec) {
            m_isConnected = true;
            //std::cout << "Client tarafında bağlantı başarılı." << std::endl;
            if (onConnected) onConnected(true);
        } else {
            m_isConnected = false;
            std::cerr << "Client tarafında bağlantı başarısız: " << ec.message() << std::endl;
            if (onConnected) onConnected(false);
        }


    });
}

void ClientSocket::sendMessage(const std::string &message) {
    if (!m_isConnected) {
        std::cerr << "Bağlantı yok mesaj gönderilemedi." << std::endl;
        return;
    }


    std::string packet = std::string(MSG_PREFIX) + std::to_string(message.size()) + ":" + message;

    auto msgBuffer = std::make_shared<std::string>(packet);

    asio::async_write(m_socket, asio::buffer(*msgBuffer),
        [this, self = shared_from_this(), msgBuffer](const std::error_code ec, std::size_t ) {
            if (ec) {
                std::cerr << "Mesaj gönderim hatası: " << ec.message() << std::endl;
                m_isConnected = false;
            }
        });
}

void ClientSocket::sendFile(const std::string &filePath) {

    if (!m_isConnected) {
        std::cerr << "Bağlantı yok dosya gönderilemedi." << std::endl;
        return;
    }

    auto file = std::make_shared<std::ifstream>(filePath, std::ios::binary | std::ios::ate);
    if (!file->is_open()) {
        std::cerr << "Dosya açılamadı." << std::endl;
        return;
    }

    std::streamsize fileSize = file->tellg();
    file->seekg(0, std::ios::beg);


    std::string fileName = filePath.substr(filePath.find_last_of("/\\") + 1);
    std::string headerStr = std::string(FILE_PREFIX) + fileName + ":" + std::to_string(fileSize) + ":";

    auto packet = std::make_shared<std::vector<char>>(headerStr.begin(), headerStr.end());

    std::size_t headerLen = packet->size();
    packet->resize(headerLen + 64 * 1024);

    file->read(packet->data() + headerLen, 64 * 1024);

    std::streamsize bytesRead = file->gcount();
    packet->resize(headerLen + bytesRead);

    int packetcounter = 1;

    asio::async_write(m_socket, asio::buffer(*packet),
        [this, self = shared_from_this(), file, packet, packetcounter](const std::error_code ec, std::size_t) {

            if (file->good()) {
                sendNextChunk(file, packetcounter);
            } else {
                std::cout << "Dosya gönderimi client tarafında 1 pakette tamamlandı." << std::endl;
            }
        }
        );

}


void ClientSocket::sendNextChunk(std::shared_ptr<std::ifstream> file, int packetcounter) {

    packetcounter++;

    auto buffer = std::make_shared<std::vector<char>>(64 * 1024);
    file->read(buffer->data(), buffer->size());
    std::streamsize bytesRead = file->gcount();

    if (bytesRead > 0) {
        asio::async_write(m_socket, asio::buffer(*buffer, bytesRead),
            [this, self = shared_from_this(), file, buffer, packetcounter](const std::error_code ec, std::size_t ) {
                if (!ec) {
                    sendNextChunk(file, packetcounter);
                }
            }
        );
    } else {
        std::cout << "Dosya gönderimi client tarafında "<< packetcounter << " pakette tamamlandı." << std::endl;
    }



}
