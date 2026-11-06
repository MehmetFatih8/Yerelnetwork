#pragma once

#include <asio.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "ClientSocket.h"
#include "FinderClient.h"
#include "FinderServer.h"
#include "NetEvents.h"
#include "ServerSocket.h"

// Arayüzün aldığı olaylar. Bunlar ağ thread'inde çağrılır; arayüz UI thread'ine geçirmelidir.
struct UiEvents {
    std::function<void(const std::string& ip, const std::string& name)> onDeviceFound;
    std::function<void(const std::string& ip)> onDeviceLost;
    std::function<void(const std::string& ip, const std::string& text)> onMessage;
    std::function<void(const TransferInfo& info)> onTransfer;
    std::function<void(const std::string& ip, const std::string& text)> onNotice;
};

// Ağ bileşenlerinin sahibi: keşif, sunucu, giden bağlantılar ve bulunan cihaz listesi.
// Genel metodlar UI thread'inden çağrılabilir; asıl işler io thread'inde yürür.
class AppController {
public:
    AppController();
    ~AppController();

    AppController(const AppController&) = delete;
    AppController& operator=(const AppController&) = delete;

    void setUiEvents(UiEvents events);

    // Portları açar ve keşfi başlatır. Port kullanımdaysa std::system_error fırlatır.
    void start();
    void stop();

    // IP doğrulaması. Hata metni döner; geçerliyse boş string.
    static std::string validateIp(const std::string& ip);

    void addDevice(const std::string& ip);
    void sendMessage(const std::string& ip, const std::string& text);
    void sendFiles(const std::string& ip, std::vector<std::filesystem::path> files);

private:
    void onMatch(const std::string& ip, const std::string& name);
    void onPeerClosed(const std::string& ip);
    void connectTo(const std::string& ip);
    void reportTransferFailure(const std::string& ip, const std::string& reason);

    asio::io_context m_io;
    asio::executor_work_guard<asio::io_context::executor_type> m_work;
    std::thread m_thread;
    bool m_running{false};

    NetEventsPtr m_net;
    UiEvents m_ui;

    FinderClient m_finderClient;
    std::unique_ptr<ServerSocket> m_server;
    std::unique_ptr<FinderServer> m_finderServer;

    std::unordered_map<std::string, std::string> m_devices;           // ip -> kullanıcı adı
    std::unordered_map<std::string, ClientSocket::Pointer> m_clients; // ip -> giden bağlantı
};
