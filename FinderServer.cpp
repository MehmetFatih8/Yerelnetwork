#include "FinderServer.h"

#include "Logger.h"
#include "Protocol.h"

namespace {

std::set<std::string> CollectLocalIps(asio::ip::tcp::resolver& resolver) {
    std::set<std::string> ips;
    std::error_code ec;
    const auto results = resolver.resolve(asio::ip::host_name(), "", ec);
    if (ec) {
        LOG() << "ERR -> Yerel IP listesi alınamadı: " << ec.message();
        return ips;
    }
    for (const auto& entry : results) {
        const auto address = entry.endpoint().address();
        if (address.is_v4()) ips.insert(address.to_string());
    }
    return ips;
}

}  // namespace

FinderServer::FinderServer(asio::io_context& io, unsigned short port, FinderClient& finderClient, NetEventsPtr events)
    : m_socket(io, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)),
      m_buffer(1024),
      m_finderClient(finderClient),
      m_events(std::move(events)) {}

void FinderServer::start() {
    asio::ip::tcp::resolver resolver(m_socket.get_executor());
    m_localIps = CollectLocalIps(resolver);
    startListening();
}

void FinderServer::startListening() {
    m_socket.async_receive_from(asio::buffer(m_buffer), m_sender,
        [this](const std::error_code& ec, std::size_t bytes) {
            if (ec) {
                // Gönderilen bir paket için ICMP hatası gelebilir; yalnızca soket kapandıysa dinlemeyi bırak
                LOG() << "ERR -> FinderServer dinleme hatası: " << ec.message();
                if (!m_socket.is_open()) return;
            } else {
                handleDatagram(std::string_view(m_buffer.data(), bytes), m_sender);
            }
            startListening();
        });
}

void FinderServer::handleDatagram(std::string_view text, const asio::ip::udp::endpoint& sender) {
    const std::string ip = sender.address().to_string();
    if (sender.address().is_loopback() || m_localIps.contains(ip)) return;

    if (text.starts_with(Protocol::kDiscoverPrefix)) {
        m_finderClient.Match(sender);
        return;
    }

    if (text.starts_with(Protocol::kMatchPrefix)) {
        // "P2P_MATCH_HELLO:<kullanıcı adı>"
        auto name = text.substr(Protocol::kMatchPrefix.size());
        if (name.starts_with(':')) name.remove_prefix(1);
        m_events->onMatch(ip, name.empty() ? ip : std::string(name));
    }
}
