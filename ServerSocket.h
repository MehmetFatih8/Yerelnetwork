//
// Created by MehmetFatih on 11.09.2026.
//

#ifndef YERELKESIF_SERVERSOCKET_H
#define YERELKESIF_SERVERSOCKET_H
#include "asio.hpp"
#include "PeerConnection.h"


class ServerSocket {
    public:
        ServerSocket(asio::io_context& ioContext, int main_port, int file_port);
        ~ServerSocket();
        void start();

    private:
        void mainAccept();
        void fileAccept();
        std::unordered_map<std::string, PeerConnection::Pointer> m_pending;
        asio::io_context& m_ioContext;
        asio::ip::tcp::acceptor m_mainacceptor;
        asio::ip::tcp::acceptor m_fileacceptor;
};


#endif //YERELKESIF_SERVERSOCKET_H
