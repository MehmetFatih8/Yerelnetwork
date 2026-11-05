//
// Created by MehmetFatih on 11.09.2026.
//

#include "ClientSocket.h"
#include <fstream>
#include <iomanip>
#include <iostream>

#include "Logger.h"
#include "PeerConnection.h"

ClientSocket::ClientSocket(asio::io_context& ioContext):
    m_ioContext(ioContext), m_mainsocket(m_ioContext), m_filesocket(m_ioContext){}

ClientSocket::~ClientSocket() {
    if (m_mainsocket.is_open()) {
        std::error_code ec;
        m_mainsocket.close(ec);
        if (ec) {
            LOG() << "ERR -> ClientSocket kapatılırken hata oluştu: " << ec.message();
        }
    }
    if (m_filesocket.is_open()) {
        std::error_code ec;
        m_filesocket.close(ec);
        if (ec) {
            LOG() << "ERR -> ClientSocket kapatılırken hata oluştu: " << ec.message();
        }
    }
}

void ClientSocket::connect(const std::string& host, unsigned short main_port, unsigned short file_port, std::function<void(bool)> on_main_Connected, std::function<void(bool)> on_file_Connected) {
    auto main_endpoint = asio::ip::tcp::endpoint(asio::ip::make_address(host), main_port);
    auto file_endpoint = asio::ip::tcp::endpoint(asio::ip::make_address(host), file_port);

    m_mainsocket.async_connect(main_endpoint, [this, self = shared_from_this(), on_main_Connected](std::error_code ec) {

        if (!ec) {
            m_mainisConnected = true;
            //std::cout << "Client tarafında bağlantı başarılı." << std::endl;
            if (on_main_Connected) on_main_Connected(true);
        } else {
            m_mainisConnected = false;
            LOG() << "ERR -> Client tarafında bağlantı başarısız: " << ec.message();
            if (on_main_Connected) on_main_Connected(false);
        }


    });
    m_filesocket.async_connect(file_endpoint, [this, self = shared_from_this(), on_file_Connected](std::error_code ec) {

        if (!ec) {
            //std::cout << "Client tarafında file socket bağlantı başarılı." << std::endl;
            m_fileisConnected = true;
            if (on_file_Connected) on_file_Connected(true);
        } else {
            m_fileisConnected = false;
            LOG() << "ERR -> Client tarafında file socket bağlantı başarısız: " << ec.message();
            if (on_file_Connected) on_file_Connected(false);
        }

    });


}

void ClientSocket::sendMessage(const std::string &message) {
    if (!m_mainisConnected) {
        LOG() << "ERR -> Bağlantı yok mesaj gönderilemedi.";
        return;
    }

    std::string packet = "MSG:" + std::to_string(message.size()) + ":" + message;
    auto msgBuffer = std::make_shared<std::string>(packet);

    asio::async_write(m_mainsocket, asio::buffer(*msgBuffer),
        [this, self = shared_from_this()](const std::error_code ec, std::size_t) {
            if (ec) {
                LOG() << "ERR -> Mesaj gönderilemedi: " << ec.message();
            }
        });

}

void ClientSocket::sendFile(const std::string &filePath) {

    auto file = std::make_shared<std::ifstream>(filePath, std::ios::binary | std::ios::ate);
    if (!file->is_open()) {
        LOG() << "ERR -> Dosya açılamadı: " << filePath;
        return;
    }

    std::size_t fileSize = file->tellg();
    file->seekg(0, std::ios::beg);

    std::string fileName = filePath.substr(filePath.find_last_of("/\\") + 1);

    std::string reqMsg = "FILE_REQ:" + fileName + ":" + std::to_string(fileSize) + ":";
    auto reqBuffer = std::make_shared<std::string>(reqMsg);

    asio::async_write(m_filesocket, asio::buffer(*reqBuffer),
        [this, self = shared_from_this(), file, fileSize](const std::error_code ec, std::size_t) {

            if (ec) {
                LOG() << "ERR -> Dosya gönderim isteği gönderilemedi: " << ec.message();
                return;
            }

            LOG() << "INFO -> Karşı tarafın onay vermesi bekleniyor...";
            m_fileAckBuffer.resize(1024);
            m_filesocket.async_read_some(asio::buffer(m_fileAckBuffer),
                [this, self, file, fileSize](const std::error_code ec, std::size_t len) {


                    if (!ec) {
                        std::string ack(m_fileAckBuffer.data(), len);
                        if (ack.starts_with("FILE_ACK")) {
                            LOG() << "INFO -> Karşı taraf dosya gönderimini onayladı. Dosya gönderimi başlatılıyor...";

                            sendNextChunk(file, fileSize, 0);

                        } else {
                            LOG() << "ERR -> Karşı taraf dosya gönderimini reddetti.";
                        }

                    } else {
                        LOG() << "ERR -> Dosya onayı alınamadı: " << ec.message();
                    }
                });

        }

        );

}

void ClientSocket::sendNextChunk(std::shared_ptr<std::ifstream> file, std::size_t fileSize, std::size_t sentBytes) {
    auto buffer = std::make_shared<std::vector<char>>(64 * 1024);
    file->read(buffer->data(), buffer->size());
    std::streamsize bytesRead = file->gcount();

    if (bytesRead > 0) {

        asio::async_write(m_filesocket, asio::buffer(*buffer, bytesRead),
            [this, self = shared_from_this(), file, fileSize, sentBytes, bytesRead](const std::error_code ec, std::size_t) {

                if (!ec) {
                    std::size_t totalSent = sentBytes + bytesRead;
                    double percent = (static_cast<double>(totalSent) / fileSize) * 100.0;

                    std::cout << "\r[Gönderiliyor] %" << std::fixed << std::setprecision(1) << percent
                              << " (" << totalSent << " / " << fileSize << " byte)" << std::flush;

                    sendNextChunk(file, fileSize, totalSent);


                } else {
                    LOG() << "ERR -> Dosya gönderim hatası: " << ec.message();
                }

            });
    } else {
        std::cout << "\nDosya gönderimi tamamlandı." << std::endl;
    }

}

