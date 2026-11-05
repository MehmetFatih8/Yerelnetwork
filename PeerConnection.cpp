//
// Created by MehmetFatih on 11.09.2026.
//

#include "PeerConnection.h"

#include <iomanip>
#include <iostream>

#include "FinderServer.h"
#include "Logger.h"

PeerConnection::PeerConnection(asio::io_context& io_context):
    m_ioContext(io_context), m_mainsocket(io_context), m_filesocket(io_context), m_mainbuffer(64 * 1024), m_filebuffer(64 * 1024) {
}

void PeerConnection::startListening() {
    m_mainsocket.async_read_some(asio::buffer(m_mainbuffer),
        [this, self = shared_from_this()](const std::error_code ec, std::size_t bytesTransferred) {
            Handle_mainRead(ec, bytesTransferred);
        });

    m_filesocket.async_read_some(asio::buffer(m_filebuffer),
        [this, self = shared_from_this()](const std::error_code ec, std::size_t bytesTransferred) {
            Handle_fileRead(ec, bytesTransferred);
        });
}


void PeerConnection::Handle_mainRead(const std::error_code& error, std::size_t bytesTransferred) {


    // SOCKETTEN MESAJ ALAMAYINCA
    if (error) {
        LOG() << "ERR -> PeerConnection tarafında mesaj alınamadı: " << error.message();
        {
            std::lock_guard<std::mutex> lock(FinderServer::devicesMutex);
            FinderServer::devices.erase(m_mainsocket.remote_endpoint().address().to_string());
        }
        return;
    }

    std::string payload(m_mainbuffer.data(), bytesTransferred);

    if (payload.starts_with("MSG:")) {
        std::stringstream ss(payload);
        std::string prefix, sizeStr, message, message_username;

        std::getline(ss, prefix, ':');
        std::getline(ss, sizeStr, ':');
        std::getline(ss, message);


        { // Kilidin başladığı yer
            std::lock_guard<std::mutex> lock(FinderServer::devicesMutex); // myMutex.lock() çalıştı

            message_username = FinderServer::devices.find(m_mainsocket.remote_endpoint().address().to_string())->second;

            // Buradaki işlemler bitene kadar diğer tüm threadler bekler:
        }

        std::cout << message_username << ":" << message << std::endl;

    }



    startListening();
}

void PeerConnection::Handle_fileRead(const std::error_code& error, std::size_t bytesTransferred) {


    if (error) {
        if (m_isReceivingFile) {
            cleanupIncompleteFile();
        }
        LOG() << "ERR -> Dosya socketinden okuma hatası: " << error.message();
        {
            std::lock_guard<std::mutex> lock(FinderServer::devicesMutex);
            FinderServer::devices.erase(m_filesocket.remote_endpoint().address().to_string());
        }
        return;
    }

    if (m_isReceivingFile) {
        std::size_t writeSize = std::min(bytesTransferred, m_remainingBytes);
        m_outputFile.write(m_filebuffer.data(), writeSize);
        m_remainingBytes -= writeSize;

        std::size_t receivedBytes = m_totalFileBytes - m_remainingBytes;
        double percent = (static_cast<double>(receivedBytes) / m_totalFileBytes) * 100.0;

        std::cout << "\r[Alınıyor] %" << std::fixed << std::setprecision(1) << percent
                  << " (" << receivedBytes << " / " << m_totalFileBytes << " bayt)" << std::flush;

        if (m_remainingBytes == 0) {
            std::cout << std::endl << "Dosya alımı tamamlandı: " << m_currentFileName << std::endl;
            m_outputFile.close();
            m_isReceivingFile = false;
            m_currentFileName.clear();
        }

    } else {


        std::string payload(m_filebuffer.data(), bytesTransferred);

        if (payload.starts_with("FILE_REQ:")) {

            std::stringstream ss(payload);
            std::string prefix, fileName, sizeStr;

            std::getline(ss, prefix, ':');
            std::getline(ss, fileName, ':');
            std::getline(ss, sizeStr, ':');

            m_totalFileBytes = std::stoull(sizeStr);
            m_remainingBytes = m_totalFileBytes;
            m_isReceivingFile = true;
            m_currentFileName = "received_" + fileName;

            m_outputFile.open(m_currentFileName, std::ios::binary);
            std::cout << "Dosya alımı başlatıldı: " << m_currentFileName << " (" << static_cast<int>(m_totalFileBytes / (1024 * 1024)) << " megabayt)" << std::endl;

            auto ackMsg = std::make_shared<std::string>("FILE_ACK");
            asio::async_write(m_filesocket, asio::buffer(*ackMsg),
                [](const std::error_code ec, std::size_t) {

                    if (ec) {
                        LOG() << "ERR -> Dosya onayı gönderilemedi: " << ec.message();
                    } else {
                        LOG() << "INFO -> Dosya onayı gönderildi.";
                    }
                });

        }

    }

    m_filesocket.async_read_some(asio::buffer(m_filebuffer),
        [this, self = shared_from_this()](const std::error_code ec, std::size_t bytesTransferred) {
            Handle_fileRead(ec, bytesTransferred);
        });

}

void PeerConnection::cleanupIncompleteFile() {

    if (m_outputFile.is_open()) {
        m_outputFile.close();
    }

    if (m_isReceivingFile) {
        m_isReceivingFile = false;
        m_remainingBytes = 0;
        if (!m_currentFileName.empty()) {
            std::remove(m_currentFileName.c_str());
            LOG() << m_currentFileName << " yarım kaldığı için sistemden temizlendi.";
            m_currentFileName.clear();
        }
    }

}
