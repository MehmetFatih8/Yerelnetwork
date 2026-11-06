#include "FinderClient.h"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string_view>

#include <windows.h>

#include "Logger.h"
#include "Protocol.h"
#include "Text.h"

FinderClient::FinderClient(asio::io_context& io, unsigned short port)
    : m_socket(io), m_broadcast(asio::ip::address_v4::broadcast(), port), m_timer(io) {
    std::error_code ec;
    m_socket.open(asio::ip::udp::v4(), ec);
    if (!ec) m_socket.set_option(asio::socket_base::broadcast(true), ec);
    if (ec) LOG() << "ERR -> FinderClient soketi açılamadı: " << ec.message();
}

void FinderClient::start() {
    m_username = getSystemUsername();
    Find();
}

void FinderClient::Find() {
    send(std::string(Protocol::kDiscoverPrefix) + ":" + m_username, m_broadcast);

    m_timer.expires_after(std::chrono::seconds(3));
    m_timer.async_wait([this](const std::error_code& ec) {
        if (!ec) Find();
    });
}

void FinderClient::Match(const asio::ip::udp::endpoint& requester) {
    // Yanıt her zaman keşif portuna gider; istekteki kaynak port geçici olabilir
    const asio::ip::udp::endpoint reply(requester.address(), m_broadcast.port());
    send(std::string(Protocol::kMatchPrefix) + ":" + m_username, reply);
}

void FinderClient::Probe(const std::string& ip) {
    // io thread'inde istisna dönmemeli: geçersiz adres sessizce reddedilir
    std::error_code ec;
    const auto address = asio::ip::make_address(ip, ec);
    if (ec) {
        LOG() << "ERR -> Geçersiz keşif adresi: " << ip;
        return;
    }
    send(std::string(Protocol::kDiscoverPrefix) + ":" + m_username,
         asio::ip::udp::endpoint(address, m_broadcast.port()));
}

void FinderClient::send(const std::string& text, const asio::ip::udp::endpoint& to) {
    auto packet = std::make_shared<const std::string>(text);
    m_socket.async_send_to(asio::buffer(*packet), to,
        [packet](const std::error_code& ec, std::size_t) {
            if (ec) LOG() << "ERR -> Keşif paketi gönderilemedi: " << ec.message();
        });
}

std::string FinderClient::getSystemUsername() {
    // GetUserNameW, oturum açmış kullanıcının adını UTF-16 olarak verir (Türkçe karakterler dahil)
    constexpr DWORD kBufferSize = 257;
    wchar_t buffer[kBufferSize];
    DWORD size = kBufferSize;
    if (GetUserNameW(buffer, &size) && size > 1) {
        return Utf8FromWide(std::wstring_view(buffer, size - 1));
    }

    if (const char* envUser = std::getenv("USERNAME"); envUser && *envUser) {
        return envUser;
    }
    return "UnknownUser";
}
