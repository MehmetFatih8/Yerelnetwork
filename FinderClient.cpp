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
    Find();
}

void FinderClient::Find() {

    m_socket.async_send_to(asio::buffer(discoveryMsg), m_endpoint,
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

    m_socket.async_send_to(asio::buffer(matchMsg), endpoint,
        [](const std::error_code& ec, std::size_t bytes_transferred) {

        });

}
