#pragma once

#include <string>

#include "webview/webview.h"

#include "../AppController.h"
#include "MainWindow.h"
#include "NativeFiles.h"

// Arayüz köprüsü: WebView2 örneğini, JavaScript'ten C++'a çağrıları ve C++'tan JavaScript'e
// gönderilen olayları yönetir. Ağ olayları burada UI thread'ine taşınır.
class UiBridge {
public:
    UiBridge(MainWindow& window, AppController& app);
    ~UiBridge();

    UiBridge(const UiBridge&) = delete;
    UiBridge& operator=(const UiBridge&) = delete;

    void run();

private:
    void configureBrowser();
    void bindFunctions();
    void bindEvents();
    void post(std::string js);   // Herhangi bir thread'den JavaScript çalıştırır
    void showError(const std::string& text);

    MainWindow& m_window;
    AppController& m_app;
    FileDropHandler m_drop;      // m_web'den önce yıkılmamalı; m_web'in sırasını koru
    webview::webview m_web;
    bool m_started{false};
};
