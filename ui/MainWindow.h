#pragma once

#include <windows.h>
#include <functional>

// Çerçevesiz görünümlü ana pencere. Başlık çubuğu, boyutlandırma ve gölge davranışını yönetir.
// İçerik alanı WebView2 tarafından doldurulur ve bu sınıf onu pencereye yerleştirir.
class MainWindow {
public:
    explicit MainWindow(HINSTANCE instance);
    ~MainWindow();

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

    HWND handle() const { return m_hwnd; }

    void show();
    void minimize();
    void toggleMaximize();
    void close();
    void dragWindow();      // Başlık çubuğundan sürükleme
    void resizeFromTop();   // Üst kenardan boyutlandırma
    void fitWebView();      // WebView2 alanını pencere içine yerleştirir

    std::function<void(bool maximized)> onMaximizeChanged;

private:
    HWND m_hwnd{nullptr};
};
