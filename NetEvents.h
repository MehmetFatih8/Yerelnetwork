#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

// Bir dosya transferinin (gönderim veya alım) durumu. Arayüze bu yapı olduğu gibi iletilir.
struct TransferInfo {
    enum class Stage { Progress, Done, Failed };

    std::string ip;
    std::string fileName;             // UTF-8
    bool incoming{false};
    double percent{0.0};
    Stage stage{Stage::Progress};
    std::size_t index{1};             // Toplu işlemdeki sıra (1'den başlar)
    std::size_t total{1};             // Toplu işlemdeki dosya sayısı
    bool batchEnd{false};             // Bu olaydan sonra bekleyen dosya kalmadıysa true
    std::string detail;               // Tamamlandıysa kayıt yolu, hata varsa açıklama
};

// Ağ katmanının (io thread) denetleyiciye bildirdiği olaylar.
struct NetEvents {
    std::function<void(const std::string& ip, const std::string& name)> onMatch;
    std::function<void(const std::string& ip, const std::string& text)> onMessage;
    std::function<void(const std::string& ip)> onPeerClosed;
    std::function<void(const TransferInfo& info)> onTransfer;
    std::function<void(const std::string& ip, const std::string& text)> onNotice;
};

using NetEventsPtr = std::shared_ptr<const NetEvents>;
