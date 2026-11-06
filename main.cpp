#include <windows.h>
#include <objbase.h>

#include <exception>

#include "AppController.h"
#include "Logger.h"
#include "ui/MainWindow.h"
#include "ui/UiBridge.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    Logger::init();

    // Pencere ve WebView2 fiziksel piksellerle çizilsin; Windows bulanık ölçeklemesin
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // WebView2, dış pencereyle kullanılırken COM'un STA olarak başlatılmasını ister
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    try {
        AppController app;
        MainWindow window(instance);
        UiBridge ui(window, app);
        ui.run();
    } catch (const std::exception& e) {
        LOG() << "ERR -> Yerelkeşif başlatılamadı: " << e.what();
        MessageBoxA(nullptr, e.what(), "Yerelkeşif", MB_ICONERROR);
        return 1;
    }

    return 0;
}
