//
// Created by MehmetFatih on 11.09.2026.
//

#ifndef YERELKESIF_SERVERSOCKET_H
#define YERELKESIF_SERVERSOCKET_H
#include "asio.hpp"


class ServerSocket {
    public:
        ServerSocket(asio::io_context& ioContext, int port);
        ~ServerSocket();
        void start();

    private:
        void startAccept();
        asio::io_context& m_ioContext;
        asio::ip::tcp::acceptor m_acceptor;
};


#endif //YERELKESIF_SERVERSOCKET_H
