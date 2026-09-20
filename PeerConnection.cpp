//
// Created by MehmetFatih on 11.09.2026.
//

#include "PeerConnection.h"

#include <iomanip>
#include <iostream>

PeerConnection::PeerConnection(asio::io_context& io_context):
    m_socket(io_context), m_buffer(64 * 1024){
}

void PeerConnection::startListening() {
    m_socket.async_read_some(asio::buffer(m_buffer),
        [this, self = shared_from_this()](const std::error_code ec, std::size_t bytesTransferred) {
            HandleRead(ec, bytesTransferred);
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
            std::cout << m_currentFileName << " dosyası silindi." << std::endl;
            m_currentFileName.clear();

        }
    }
}

void PeerConnection::HandleRead(const std::error_code& error, std::size_t bytesTransferred) {



    // DOSYA AKTARIMI YARIDA KESİLİRSE
    if (error && m_isReceivingFile) {
        cleanupIncompleteFile();
        return;
    }

    // SOCKETTEN MESAJ ALAMAYINCA
    if (error) {
        std::cerr << error.message() << std::endl;
        std::cout << ip_port << "'tan mesaj alınamadı. Clienttan mesaj bekleme döngüsünü öldürüyorum." << std::endl;
        return;
    }

    ip_port = (m_socket.remote_endpoint().address().to_string() + std::to_string(m_socket.remote_endpoint().port()));


    // DOSYA AKTARIMI SIRASINDA
    if (m_isReceivingFile) {

        std::size_t writeSize = std::min(m_remainingBytes, bytesTransferred);
        m_outputFile.write(m_buffer.data(), writeSize);
        m_remainingBytes -= writeSize;

        if (m_remainingBytes == 0) {
            std::cout << m_currentFileName <<" adlı dosyanın alımı tamamlandı." << std::endl;
            m_outputFile.close();
            m_isReceivingFile = false;
            m_currentFileName.clear();
        }


    }

    // MESAJ GELDİĞİNDE
    else if (bytesTransferred >= MSG_PREFIX.size() && std::string(m_buffer.data(), MSG_PREFIX.size()) == MSG_PREFIX) {

        std::string payload(m_buffer.data() + MSG_PREFIX.size(), bytesTransferred - MSG_PREFIX.size());
        std::size_t colonPos = payload.find(':');
        if (colonPos != std::string::npos) {
            std::string actualMessage = payload.substr(colonPos + 1);
            std::cout << ip_port << "'tan gelen mesaj:" << actualMessage << std::endl;
        }

    }


    // DOSYA AKTARIMI BAŞLATILDIĞINDA
    else if (bytesTransferred >= FILE_PREFIX.size() && std::string(m_buffer.data(), FILE_PREFIX.size()) == std::string(FILE_PREFIX)) {

        std::string payload(m_buffer.data() + FILE_PREFIX.size(), bytesTransferred - FILE_PREFIX.size());
        std::stringstream ss(payload);

        std::string fileName, sizeStr;

        if (std::getline(ss, fileName, ':')&& std::getline(ss, sizeStr, ':')) {

            m_remainingBytes = std::stoull(sizeStr);
            m_isReceivingFile = true;
            m_currentFileName = "received_ " + fileName;

            m_outputFile.open(m_currentFileName, std::ios::binary);
            std::cout << fileName << " adlı dosyanın transferi başladı. Dosya boyutu: " << m_remainingBytes << " bayt." << std::endl;


            std::string extraData;
            std::size_t headerSize = FILE_PREFIX.size() + fileName.size() + sizeStr.size() + 2; // +2 iki tane ':' için

            if (bytesTransferred > headerSize) {
                std::size_t initalDataSize = bytesTransferred - headerSize;
                std::size_t writeSize = std::min(initalDataSize, m_remainingBytes);

                m_outputFile.write(m_buffer.data() + headerSize, writeSize);

                m_remainingBytes -= writeSize;

                if (m_remainingBytes == 0) {
                    std::cout << "Dosya alımı server tarafında 1 pakette tamamlandı." << std::endl;
                    m_outputFile.close();
                    m_isReceivingFile = false;
                    m_currentFileName.clear();
                }

            }


        }

    }

    startListening();

}