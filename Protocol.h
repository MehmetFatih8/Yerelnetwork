#pragma once

#include <cstddef>
#include <string_view>

// Ağ protokolü sabitleri. Portlar ve paket ön ekleri yalnızca burada tanımlanır.
namespace Protocol {

    // Portlar
    inline constexpr unsigned short kDiscoveryPort = 8888;  // UDP: cihaz keşfi
    inline constexpr unsigned short kMainPort = 9999;       // TCP: mesajlar
    inline constexpr unsigned short kFilePort = 9998;       // TCP: dosya transferi

    // UDP keşif paketleri: "<ön ek>:<kullanıcı adı>"
    inline constexpr std::string_view kDiscoverPrefix = "P2P_DISCOVER_HELLO";
    inline constexpr std::string_view kMatchPrefix = "P2P_MATCH_HELLO";

    // Ana TCP soketi: "MSG:<bayt sayısı>:<metin>"
    inline constexpr std::string_view kMsgPrefix = "MSG:";

    // Dosya TCP soketi: "FILE_REQ:<ad>:<bayt>:" isteğine "FILE_ACK" (kabul) veya "FILE_ERR" (red) yanıtı,
    // kabulden sonra gönderen ham veriyi akıtır.
    inline constexpr std::string_view kFileReqPrefix = "FILE_REQ:";
    inline constexpr std::string_view kFileAck = "FILE_ACK";
    inline constexpr std::string_view kFileErr = "FILE_ERR";

    inline constexpr std::size_t kChunkSize = 64 * 1024;
    inline constexpr std::size_t kMaxMessageSize = 1024 * 1024;  // 1 MB
    inline constexpr std::size_t kMaxHeaderSize = 1024;          // Çerçeve başlığı üst sınırı
}
