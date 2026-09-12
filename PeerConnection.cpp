//
// Created by MehmetFatih on 11.09.2026.
//

#include "PeerConnection.h"

#include <iostream>

PeerConnection::PeerConnection(asio::io_context& io_context):
    m_socket(io_context), m_buffer(4096) {

}

void PeerConnection::startListening() {
    m_socket.async_read_some(asio::buffer(m_buffer),
        std::bind(&PeerConnection::HandleRead, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void PeerConnection::HandleRead(const std::error_code& error, std::size_t bytesTransferred) {
    
    if (!error) {
        std::string remote_user_ip = m_socket.remote_endpoint().address().to_string();
        std::string remote_user_port = std::to_string(m_socket.remote_endpoint().port());
        std::cout << remote_user_ip << ":" << remote_user_port << "'den gelen bayt sayısı: " << bytesTransferred << std::endl;
        std::cout.write(m_buffer.data(), bytesTransferred);
        std::cout << std::endl;

        startListening();

    } else {
        std::cerr << "Hata oldu: " << error.message() << std::endl;
    }
}