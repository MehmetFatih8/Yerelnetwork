#include "MainWindow.h"

#include <stdexcept>

#include <dwmapi.h>

namespace {

constexpr wchar_t kClassName[] = L"YerelKesifPencere";
constexpr int kMinWidth = 880;    // Mantıksal piksel (96 DPI)
constexpr int kMinHeight = 600;

// Sol/sağ/alt boyutlandırma kenarlarının kalınlığı. Büyütülmüşken 0.
int ResizeBorder(HWND hwnd) {
    if (IsZoomed(hwnd)) return 0;
    return GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
}

// Gövde alanı pencerenin tamamını kaplar, üst kenar her zaman 0: başlık çubuğu en tepeye oturur.
// webview, dış pencere verildiğinde WebView2 alanını boyutlandırmaz; alanı biz yerleştiriyoruz.
void FitChild(HWND hwnd) {
    HWND child = FindWindowExW(hwnd, nullptr, L"webview_widget", nullptr);
    RECT rc{};
    if (!child || !GetClientRect(hwnd, &rc)) return;

    const int border = ResizeBorder(hwnd);
    SetWindowPos(child, nullptr, border, 0, rc.right - 2 * border, rc.bottom - border,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

// Büyütülmüşken çerçeve olmaz (kenarda beyaz çizgi kalmasın)
void ApplyShadow(HWND hwnd) {
    const MARGINS shadow = IsZoomed(hwnd) ? MARGINS{0, 0, 0, 0} : MARGINS{0, 0, 1, 1};
    DwmExtendFrameIntoClientArea(hwnd, &shadow);
}

// Windows 11'in aktif pencere etrafına çizdiği beyaz vurgu çizgisini kapatır
void RemoveBorderColor(HWND hwnd) {
    const COLORREF noColor = 0xFFFFFFFE;   // DWMWA_COLOR_NONE
    const DWORD dwmaBorderColor = 34;      // DWMWA_BORDER_COLOR
    DwmSetWindowAttribute(hwnd, dwmaBorderColor, &noColor, sizeof(noColor));
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lp);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    auto* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_NCCALCSIZE:
        // Başlık çubuğu yok: tüm pencere istemci alanı. WS_CAPTION stili kalır, böylece
        // Windows'un animasyonları ve gölgesi çalışır. Büyütülmüşken pencere ekran dışına taşar, içeri alınır.
        if (wp == TRUE && IsZoomed(hwnd)) {
            auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lp);
            const int pay = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
            params->rgrc[0].left += pay;
            params->rgrc[0].top += pay;
            params->rgrc[0].right -= pay;
            params->rgrc[0].bottom -= pay;
        }
        return 0;

    case WM_NCACTIVATE:
        // Aktiflik değişince Windows çerçeveyi yeniden çizip beyaz vurgu çizgisi göstermesin
        return DefWindowProcW(hwnd, msg, wp, -1);

    case WM_NCHITTEST: {
        if (IsZoomed(hwnd)) return HTCLIENT;

        const int x = static_cast<short>(LOWORD(lp));
        const int y = static_cast<short>(HIWORD(lp));
        RECT rc{};
        GetWindowRect(hwnd, &rc);
        const int border = ResizeBorder(hwnd);
        const bool left = x < rc.left + border;
        const bool right = x >= rc.right - border;
        const bool top = y < rc.top + border;
        const bool bottom = y >= rc.bottom - border;

        if (top && left) return HTTOPLEFT;
        if (top && right) return HTTOPRIGHT;
        if (bottom && left) return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
        if (top) return HTTOP;
        if (bottom) return HTBOTTOM;
        return HTCLIENT;
    }

    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lp);
        const UINT dpi = GetDpiForWindow(hwnd);
        info->ptMinTrackSize.x = MulDiv(kMinWidth, dpi, 96);
        info->ptMinTrackSize.y = MulDiv(kMinHeight, dpi, 96);

        // Başlık çubuğu gizliyken büyütülmüş boyut görev çubuğunu kapatabilir; çalışma alanına sabitle
        MONITORINFO monitor{};
        monitor.cbSize = sizeof(monitor);
        if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor)) {
            info->ptMaxPosition.x = monitor.rcWork.left - monitor.rcMonitor.left;
            info->ptMaxPosition.y = monitor.rcWork.top - monitor.rcMonitor.top;
            info->ptMaxSize.x = monitor.rcWork.right - monitor.rcWork.left;
            info->ptMaxSize.y = monitor.rcWork.bottom - monitor.rcWork.top;
        }
        return 0;
    }

    case WM_SIZE:
        ApplyShadow(hwnd);
        FitChild(hwnd);
        if (self && self->onMaximizeChanged && (wp == SIZE_MAXIMIZED || wp == SIZE_RESTORED)) {
            self->onMaximizeChanged(wp == SIZE_MAXIMIZED);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace

MainWindow::MainWindow(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(12, 16, 24));   // Arayüzün zemin rengiyle aynı
    wc.lpszClassName = kClassName;
    RegisterClassExW(&wc);

    // Pencere gizli oluşturulur; arayüz ilk kareyi çizince gösterilir
    const UINT dpi = GetDpiForSystem();
    const int width = MulDiv(960, dpi, 96);
    const int height = MulDiv(640, dpi, 96);

    RECT work{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    const int x = work.left + (work.right - work.left - width) / 2;
    const int y = work.top + (work.bottom - work.top - height) / 2;

    m_hwnd = CreateWindowExW(0, kClassName, L"Yerelkeşif", WS_OVERLAPPEDWINDOW,
                             x, y, width, height, nullptr, nullptr, instance, this);
    if (!m_hwnd) throw std::runtime_error("Pencere oluşturulamadı.");

    RemoveBorderColor(m_hwnd);
    ApplyShadow(m_hwnd);
}

MainWindow::~MainWindow() {
    if (m_hwnd && IsWindow(m_hwnd)) DestroyWindow(m_hwnd);
}

void MainWindow::show() {
    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
}

void MainWindow::minimize() {
    ShowWindow(m_hwnd, SW_MINIMIZE);
}

void MainWindow::toggleMaximize() {
    ShowWindow(m_hwnd, IsZoomed(m_hwnd) ? SW_RESTORE : SW_MAXIMIZE);
}

void MainWindow::close() {
    PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
}

void MainWindow::dragWindow() {
    // Fare WebView2 alanının üstündeyken sürükleme Windows'a asenkron bildirilir
    ReleaseCapture();
    PostMessageW(m_hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
}

void MainWindow::resizeFromTop() {
    ReleaseCapture();
    PostMessageW(m_hwnd, WM_NCLBUTTONDOWN, HTTOP, 0);
}

void MainWindow::fitWebView() {
    FitChild(m_hwnd);
}
