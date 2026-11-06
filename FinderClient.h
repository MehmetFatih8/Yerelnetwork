#pragma once

#include <asio.hpp>
#include <string>

// UDP keşif yayını (3 saniyede bir) ve keşif isteklerine verilen eşleşme yanıtları.
class FinderClient {
public:
    FinderClient(asio::io_context& io, unsigned short port);

    void start();                                           // Kullanıcı adını alır ve yayını başlatır
    void Match(const asio::ip::udp::endpoint& requester);   // Keşif isteğine eşleşme yanıtı verir
    void Probe(const std::string& ip);                      // Belirli bir IP'ye keşif isteği (manuel ekleme)

private:
    void Find();
    void send(const std::string& text, const asio::ip::udp::endpoint& to);
    static std::string getSystemUsername();

    asio::ip::udp::socket m_socket;
    asio::ip::udp::endpoint m_broadcast;
    asio::steady_timer m_timer;
    std::string m_username;   // UTF-8
};
