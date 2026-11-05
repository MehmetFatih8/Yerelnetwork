//
// Created by MehmetFatih on 11.09.2026.
//

#ifndef YERELKESIF_PEERCONNECTION_H
#define YERELKESIF_PEERCONNECTION_H

#include <asio.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>



class PeerConnection : public std::enable_shared_from_this<PeerConnection> {
    public:
        using Pointer =std::shared_ptr<PeerConnection>;

        static Pointer create(asio::io_context& io_context) {
            return Pointer(new PeerConnection(io_context));
        }

        void startListening();

        void set_mainsocket(asio::ip::tcp::socket&& socket) {
            m_mainsocket = std::move(socket);
        }

        void set_filesocket(asio::ip::tcp::socket&& socket) {
            m_filesocket = std::move(socket);
        }

        bool isReady() const {
            return m_mainsocket.is_open() && m_filesocket.is_open();
        }



    private:
        explicit PeerConnection(asio::io_context& io_context);

        void Handle_mainRead(const std::error_code& error, std::size_t bytesTransferred);
        void Handle_fileRead(const std::error_code& error, std::size_t bytesTransferred);
        void cleanupIncompleteFile();

        asio::io_context& m_ioContext;
        asio::ip::tcp::socket m_mainsocket;
        asio::ip::tcp::socket m_filesocket;
        std::vector<char> m_mainbuffer;
        std::vector<char> m_filebuffer;

        std::ofstream m_outputFile;
        std::string m_currentFileName;
        std::size_t m_remainingBytes{0};
        std::size_t m_totalFileBytes{0};
        bool m_isReceivingFile{false};

        //constexpr static std::string_view FILE_PREFIX_RAW = std::string_view("\x00\xFF__P2P_FILE__\x1F", 17);
        //inline static const std::string FILE_PREFIX{FILE_PREFIX_RAW.data(), FILE_PREFIX_RAW.size()};

        //constexpr static std::string_view MSG_PREFIX_RAW = std::string_view("\x00\xFF__P2P_MSG__\x1F", 16);
        //inline static const std::string MSG_PREFIX{MSG_PREFIX_RAW.data(), MSG_PREFIX_RAW.size()};

};


#endif //YERELKESIF_PEERCONNECTION_H
