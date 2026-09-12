//
// Created by MehmetFatih on 11.09.2026.
//

#ifndef YERELKESIF_PEERCONNECTION_H
#define YERELKESIF_PEERCONNECTION_H

#include <asio.hpp>
#include <fstream>
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
        std::string ip_port;

    private:
        explicit PeerConnection(asio::io_context& io_context);

        void HandleRead(const std::error_code& error, std::size_t bytesTransferred);
        void cleanupIncompleteFile();

        asio::ip::tcp::socket m_socket;
        std::vector<char> m_buffer;

        std::ofstream m_outputFile;
        std::string m_currentFileName;
        std::size_t m_remainingBytes{0};
        bool m_isReceivingFile{false};

        constexpr static std::string_view FILE_PREFIX_RAW = std::string_view("\x00\xFF__P2P_FILE__\x1F", 17);
        inline static const std::string FILE_PREFIX{FILE_PREFIX_RAW.data(), FILE_PREFIX_RAW.size()};

        constexpr static std::string_view MSG_PREFIX_RAW = std::string_view("\x00\xFF__P2P_MSG__\x1F", 16);
        inline static const std::string MSG_PREFIX{MSG_PREFIX_RAW.data(), MSG_PREFIX_RAW.size()};

};


#endif //YERELKESIF_PEERCONNECTION_H
