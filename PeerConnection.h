//
// Created by MehmetFatih on 11.09.2026.
//

#ifndef YERELKESIF_PEERCONNECTION_H
#define YERELKESIF_PEERCONNECTION_H

#include <asio.hpp>
#include <memory>
#include <vector>



class PeerConnection : public std::enable_shared_from_this<PeerConnection> {
    public:
        using Pointer =std::shared_ptr<PeerConnection>;

        static Pointer create(asio::io_context& io_context) {
            return Pointer(new PeerConnection(io_context));
        }

        asio::ip::tcp::socket& getSocket() {
            return m_socket;
        }
        void startListening();

    private:
        explicit PeerConnection(asio::io_context& io_context);

        void HandleRead(const std::error_code& error, std::size_t bytesTransferred);

        asio::ip::tcp::socket m_socket;
        std::vector<char> m_buffer;
};


#endif //YERELKESIF_PEERCONNECTION_H
