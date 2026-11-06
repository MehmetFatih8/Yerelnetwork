#pragma once

#include <windows.h>

#include <functional>
#include <string>
#include <vector>

#include "WebView2.h"

// Windows dosya seçme penceresi (çoklu seçim). Sonuç [{"name","size","path"}] JSON dizisidir; iptalde "[]".
std::string PickFilesJson(HWND owner);

// Tam yol listesini [{"name","size","path"}] JSON dizisine çevirir.
std::string FilesJson(const std::vector<std::wstring>& paths);

// WebView2 mesaj olayı. Arayüz sürükle-bırakta dosya nesnelerini postMessageWithAdditionalObjects ile yollar;
// bu sınıf nesnelerin tam yollarını alıp onDrop'a iletir.
class FileDropHandler : public ICoreWebView2WebMessageReceivedEventHandler {
public:
    std::function<void(const std::vector<std::wstring>& paths)> onDrop;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }   // Uygulama ömrü boyunca yaşar
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender,
                                     ICoreWebView2WebMessageReceivedEventArgs* args) override;
};
