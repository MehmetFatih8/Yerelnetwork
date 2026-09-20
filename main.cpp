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

//constexpr std::string ip = "127.0.0.1";
constexpr unsigned int tcp_port = 9999;
constexpr unsigned int udp_port = 8888;
std::unordered_map<std::string, ClientSocket::Pointer> clientPool;

void connect(asio::io_context& ioContext, const std::string& ip) {

    std::cout << ip << "'e bağlanılıyor." << std::endl;

    auto newClient = ClientSocket::create(ioContext);
    newClient->connect(ip, tcp_port, [](bool connected) {

                if (connected) {
                    std::cout << "Bağlantı başarılı." << std::endl;
                }
    });

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

        std::setlocale(LC_ALL, "tr_TR.UTF-8");

        asio::io_context ioContext;

        auto workGuard = asio::make_work_guard(ioContext);

        ServerSocket server(ioContext, tcp_port);
        server.start();


        FinderClient finder1 = FinderClient(ioContext, udp_port);

        FinderServer finder0(ioContext, udp_port, finder1);

        finder0.setOnDeviceDiscovered([&ioContext, &finder0](const std::string& ip) {
            std::cout << ip << " " << finder0.devices.find(ip)->second << " ile bağlantı kuruluyor." << std::endl;
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


            if (komut == "exit") {
                break;
            }


            // if (baglanti == true && komut.starts_with('C')) {
            //
            //     const std::string& filePath = komut;
            //
            //     std::string peerKey = "127.0.0.1:" + std::to_string(tcp_port);
            //     sendfile(peerKey, filePath);
            //
            //
            // }

            bool success{false};

            size_t colonPos = komut.find(':');

            if (colonPos != std::string::npos || colonPos != 0) {

                std::string input_username = komut.substr(0, colonPos);
                std::string message = komut.substr(colonPos + 1);

                for (const auto& [ip, username] : finder0.devices) {
                    if (username == input_username) {
                        std::cout << message << " mesajı gönderiliyor" << std::endl;
                        success = true;

                        if (message.starts_with('C')) {
                            sendfile(ip, message);
                        } else {
                            sendmessage(ip, message);
                        }

                    }
                }

            }
            if (!success) {
                std::cout << "Mesaj gönderme sırasında bir hata oldu." << std::endl;
            }

        }


        std::cout << "Bitiyor." << std::endl;

        workGuard.reset();
        ioContext.stop();

        if (workerThread.joinable()) {
            workerThread.join();
        }

    } catch (std::exception& e) {

        std::cerr << "Main thread hatasi: " << e.what() << std::endl;

    }

    return 0;
}








/*void ApplyCustomStyle() {
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.WindowPadding = ImVec2(12, 12);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]      = ImVec4(0.04f, 0.05f, 0.08f, 1.00f);
    colors[ImGuiCol_ChildBg]       = ImVec4(0.06f, 0.08f, 0.13f, 1.00f);
    colors[ImGuiCol_Border]        = ImVec4(0.12f, 0.18f, 0.30f, 1.00f);
    colors[ImGuiCol_Button]        = ImVec4(0.08f, 0.14f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.12f, 0.22f, 0.38f, 1.00f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.00f, 0.50f, 0.80f, 1.00f);
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1100, 580, "Yerelkeşif", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Font ve Türkçe Karakter Ayarı
    ImFontConfig font_config;
    font_config.PixelSnapH = true;

    ImWchar ranges[] = {
        0x0020, 0x00FF, // Temel Latin
        0x0100, 0x017F, // Latin Extended-A (Türkçe: Ş, Ğ, İ, Ö, Ü, Ç, ş, ğ, ı, ö, ü, ç)
        0,
    };

#ifdef _WIN32
    ImFont* custom_font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeuib.ttf", 19.0f, &font_config, ranges);
    if (!custom_font) {
        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 19.0f, &font_config, ranges);
    }
#elif defined(__linux__)
    ImFont* custom_font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 19.0f, &font_config, ranges);
    if (!custom_font) {
        io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf", 19.0f, &font_config, ranges);
    }
#endif

    ApplyCustomStyle();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Dinamik IP Listesi (Başlangıçta boş)
    std::vector<std::string> discoveredIps;
    int nextIpSuffix = 10; // İlk eklenecek IP 192.168.1.10 olacak

    int display_w = 0, display_h = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));

        ImGui::Begin("MainDashboard", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse);

        float windowWidth = ImGui::GetContentRegionAvail().x;
        float leftWidth = windowWidth * 0.33f;

        // --- SOL PANEL: Discovered Devices ---
        ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, 0), true, ImGuiWindowFlags_NoScrollbar);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
        ImGui::TextColored(ImVec4(0.90f, 0.92f, 0.95f, 1.00f), "  Discovered Devices");

        ImGui::SameLine(leftWidth - 95);
        if (ImGui::Button("+ IP Ekle", ImVec2(85, 30))) {
            discoveredIps.push_back("192.168.1." + std::to_string(nextIpSuffix));
            nextIpSuffix++;
        }

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        // Listeye eklenen dinamik IP'leri ekrana kart olarak basıyoruz
        for (int i = 0; i < (int)discoveredIps.size(); i++) {
            ImGui::PushID(i);
            ImGui::BeginChild(discoveredIps[i].c_str(), ImVec2(0, 52), true, ImGuiWindowFlags_NoScrollbar);

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
            ImGui::TextColored(ImVec4(0.00f, 0.83f, 1.00f, 1.00f), " %s", discoveredIps[i].c_str());

            ImGui::SameLine(145);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
            ImGui::TextColored(ImVec4(0.12f, 0.80f, 0.35f, 1.00f), "Aktif");

            ImGui::SameLine(ImGui::GetWindowWidth() - 85);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
            if (ImGui::Button("Bağlan", ImVec2(75, 28))) {}

            ImGui::EndChild();
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // --- SAĞ PANEL: Dosya Gönderim Alanı ---
        float rightWidth = ImGui::GetContentRegionAvail().x;
        ImGui::BeginChild("RightPanel", ImVec2(rightWidth, 0), true, ImGuiWindowFlags_NoScrollbar);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
        ImGui::TextColored(ImVec4(0.00f, 0.83f, 1.00f, 1.00f), "  192.168.1.10 - Dosya Gönderimi");

        ImGui::SameLine(rightWidth - 45);
        if (ImGui::Button("X", ImVec2(32, 30))) {}

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        float innerHeight = ImGui::GetContentRegionAvail().y - 60;

        // Sürükle Bırak Kutusu
        ImGui::BeginChild("DropZone", ImVec2(240, innerHeight), true, ImGuiWindowFlags_NoScrollbar);
        {
            float zoneW = ImGui::GetWindowWidth();
            float zoneH = ImGui::GetWindowHeight();
            ImGui::SetCursorPos(ImVec2(zoneW / 2 - 50, zoneH / 2 - 40));
            ImGui::TextColored(ImVec4(0.00f, 0.83f, 1.00f, 1.00f), "       +");
            ImGui::Spacing();
            ImGui::SetCursorPosX(zoneW / 2 - 75);
            ImGui::TextDisabled("Dosya Sürükle veya Tıkla");
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Dosya Listesi Kutusu
        float fileListWidth = ImGui::GetContentRegionAvail().x;
        ImGui::BeginChild("FileListPanel", ImVec2(fileListWidth, innerHeight), true, ImGuiWindowFlags_NoScrollbar);
        {
            if (ImGui::Button("+ Dosya Ekle", ImVec2(fileListWidth - 20, 36))) {}
            float listH = ImGui::GetWindowHeight();
            ImGui::SetCursorPos(ImVec2(fileListWidth / 2 - 85, listH / 2));
            ImGui::TextDisabled("Henüz dosya seçilmedi");
        }
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::SetCursorPosX(rightWidth - 155);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.75f, 0.42f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.88f, 0.50f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.08f, 0.60f, 0.34f, 1.00f));

        if (ImGui::Button("Transferi Başlat", ImVec2(145, 38))) {}

        ImGui::PopStyleColor(3);

        ImGui::EndChild();
        ImGui::End();

        // Render İşlemi
        ImGui::Render();
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.04f, 0.05f, 0.08f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}*/

