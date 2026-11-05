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

        void connect(const std::string& host, unsigned short main_port, unsigned short file_port, std::function<void(bool)> on_main_Connected = nullptr, std::function<void(bool)> on_file_Connected = nullptr);

        void sendMessage(const std::string& message);
        void sendFile(const std::string& filePath);

        //bool isConnected() const {return m_isConnected;}

        ~ClientSocket();

    private:
        explicit ClientSocket(asio::io_context& ioContext);
        void sendNextChunk(std::shared_ptr<std::ifstream> file, std::size_t fileSize, std::size_t sentBytes);


        asio::io_context& m_ioContext;
        asio::ip::tcp::socket m_mainsocket;
        asio::ip::tcp::socket m_filesocket;
        bool m_mainisConnected{false};
        bool m_fileisConnected{false};
        std::vector<char> m_fileAckBuffer;


        //constexpr static std::string_view FILE_PREFIX_RAW = std::string_view("\x00\xFF__P2P_FILE__\x1F", 17);
        //inline static const std::string FILE_PREFIX{FILE_PREFIX_RAW.data(), FILE_PREFIX_RAW.size()};

        //constexpr static std::string_view MSG_PREFIX_RAW = std::string_view("\x00\xFF__P2P_MSG__\x1F", 16);
        //inline static const std::string MSG_PREFIX{MSG_PREFIX_RAW.data(), MSG_PREFIX_RAW.size()};
};


#endif //YERELKESIF_CLIENTSOCKET_H
