/*#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>*/
#include <iostream>
#include <asio.hpp>
#include "ServerSocket.h"
#include "ClientSocket.h"
#include "FinderClient.h"
#include "FinderServer.h"
#include "Logger.h"

//constexpr std::string ip = "127.0.0.1";
constexpr unsigned int tcp_mainport = 9999;
constexpr unsigned int tcp_fileport = 9998;
constexpr unsigned int udp_port = 8888;
std::unordered_map<std::string, ClientSocket::Pointer> clientPool;

void connect(asio::io_context& ioContext, const std::string& ip) {

    LOG() << ip << "'e bağlanılıyor.";


    auto newClient = ClientSocket::create(ioContext);
    newClient->connect(ip, tcp_mainport, tcp_fileport, [](bool mainsocket_connected) {

        if (mainsocket_connected) {
            LOG() << "Main bağlantı başarılı.";

        }
    },

    [](bool filesocket_connected) {

        if (filesocket_connected) {
            LOG() << "Dosya socket bağlantısı başarılı.";

        }
    }

    );

    clientPool[ip] = newClient;

}

void sendmessage(const std::string& ip, const std::string& message) {

    auto it = clientPool.find(ip);

    it->second->sendMessage(message);

}

void sendfile(const std::string& ip, const std::string& filePath) {

    auto it = clientPool.find(ip);

    it->second->sendFile(filePath);

}

int main() {

    try {

        Logger::init();
        std::setlocale(LC_ALL, "tr_TR.UTF-8");

        #if defined(_WIN32)
                // Konsol girdi ve çıktılarını UTF-8 standardına çek
                SetConsoleCP(CP_UTF8);
                SetConsoleOutputCP(CP_UTF8);
        #endif

        asio::io_context ioContext;

        auto workGuard = asio::make_work_guard(ioContext);

        ServerSocket server(ioContext, tcp_mainport, tcp_fileport);
        server.start();


        FinderClient finder1 = FinderClient(ioContext, udp_port);

        FinderServer finder0(ioContext, udp_port, finder1);

        finder0.setOnDeviceDiscovered([&ioContext](const std::string& ip) {

            {
                std::lock_guard<std::mutex> lock(FinderServer::devicesMutex);
                LOG() << ip << " " << FinderServer::devices.find(ip)->second << " ile bağlantı kuruluyor.";

            }

            connect(ioContext, ip);
        });

        finder0.start();



        std::thread workerThread([&ioContext]() {
            ioContext.run();
        });

        std::cout << "Başladı." << std::endl;

        finder1.start();

        while (true) {

            std::string komut;
            std::getline(std::cin, komut);

            bool success{false};


            if (komut == "exit") {
                break;
            }

            else if (komut == "/list") {
                std::cout << "Bulunan cihazlar:" << std::endl;
                {
                    std::lock_guard<std::mutex> lock(FinderServer::devicesMutex);
                    for (const auto& [ip, username] : FinderServer::devices) {
                        std::cout << ip << " - " << username << std::endl;
                }
                success = true;
                }

            }

            else if (komut.starts_with("/file")) {

                std::stringstream ss(komut);
                std::string command, input_username, filePath;
                std::getline(ss, command, ' ');
                std::getline(ss, input_username, ':');
                std::getline(ss, filePath);

                {
                    std::lock_guard<std::mutex> lock(FinderServer::devicesMutex);
                    for (const auto& [ip, username] : FinderServer::devices) {
                    if (username == input_username) {
                        LOG() << filePath << " dosyası gönderiliyor.";
                        sendfile(ip, filePath);
                        success = true;
                    }
                    }
                }


            }

            else {
                std::stringstream ss(komut);
                std::string input_username, mesaj;
                std::getline(ss, input_username, ':');
                std::getline(ss, mesaj);


                {
                    std::lock_guard<std::mutex> lock(FinderServer::devicesMutex);
                    for (const auto& [ip, username] : FinderServer::devices) {
                    if (username == input_username) {
                        sendmessage(ip, mesaj);
                        success = true;
                    }
                    }
                }


            }

            if (!success) {
                std::cout << "Mesaj gönderme sırasında bir hata oldu." << std::endl;
            }

        }

        workGuard.reset();
        ioContext.stop();

        if (workerThread.joinable()) {
            workerThread.join();
        }

    } catch (std::exception& e) {

        LOG() << "ERR -> Main thread hatasi: " << e.what();

    }

    return 0;
}
