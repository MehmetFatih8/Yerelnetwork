//
// Created by MehmetFatih on 11.09.2026.
//

#ifndef YERELKESIF_CLIENTSOCKET_H
#define YERELKESIF_CLIENTSOCKET_H
#include <memory>
#include <asio.hpp>


class ClientSocket : public std::enable_shared_from_this<ClientSocket> {
    public:
        using Pointer = std::shared_ptr<ClientSocket>;

        static Pointer create(asio::io_context& ioContext) {
            return Pointer(new ClientSocket(ioContext));
        }

        void connect(const std::string& host, unsigned short port, std::function<void(bool)> onConnected = nullptr);

        void sendMessage(const std::string& message);

        bool isConnected() const {return m_isConnected;}

        ~ClientSocket();

    private:
        explicit ClientSocket(asio::io_context& ioContext);

        asio::io_context& m_ioContext;
        asio::ip::tcp::socket m_socket;
        bool m_isConnected{false};
};


#endif //YERELKESIF_CLIENTSOCKET_H
