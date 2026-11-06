#include "ServerSocket.h"

#include "Logger.h"

ServerSocket::ServerSocket(asio::io_context& io, NetEventsPtr events, unsigned short mainPort, unsigned short filePort)
    : m_io(io),
      m_events(std::move(events)),
      m_mainAcceptor(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), mainPort)),
      m_fileAcceptor(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), filePort)) {}

void ServerSocket::start() {
    acceptLoop(m_mainAcceptor, true);
    acceptLoop(m_fileAcceptor, false);
}

void ServerSocket::acceptLoop(asio::ip::tcp::acceptor& acceptor, bool isMain) {
    auto sock = std::make_shared<asio::ip::tcp::socket>(m_io);
    acceptor.async_accept(*sock, [this, &acceptor, isMain, sock](const std::error_code& ec) {
        if (ec) {
            LOG() << "ERR -> Bağlantı kabul edilemedi: " << ec.message();
            if (acceptor.is_open()) acceptLoop(acceptor, isMain);
            return;
        }

        std::error_code endpointEc;
        const auto endpoint = sock->remote_endpoint(endpointEc);
        if (!endpointEc) {
            const std::string ip = endpoint.address().to_string();
            auto& peer = m_pending[ip];
            if (!peer) peer = PeerConnection::create(m_io, m_events, ip);

            if (isMain) {
                peer->setMainSocket(std::move(*sock));
            } else {
                peer->setFileSocket(std::move(*sock));
            }

            if (peer->isReady()) {
                peer->startListening();
                m_pending.erase(ip);
            }
        }

        acceptLoop(acceptor, isMain);
    });
}
