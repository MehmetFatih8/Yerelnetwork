#include "NativeFiles.h"

#include <objbase.h>
#include <commdlg.h>
#include <shlwapi.h>

#include "../Text.h"

namespace {

std::string FileRecordJson(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA data{};
    unsigned long long size = 0;
    if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) {
        size = (static_cast<unsigned long long>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
    }
    const std::wstring name = PathFindFileNameW(path.c_str());

    return "{\"name\":" + JsonQuote(Utf8FromWide(name)) +
           ",\"size\":" + std::to_string(size) +
           ",\"path\":" + JsonQuote(Utf8FromWide(path)) + "}";
}

}  // namespace

std::string PickFilesJson(HWND owner) {
    std::vector<wchar_t> buffer(1 << 16, L'\0');
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = buffer.data();
    ofn.nMaxFile = static_cast<DWORD>(buffer.size());
    ofn.lpstrFilter = L"Tüm dosyalar (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"Gönderilecek dosyaları seçin";
    ofn.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetOpenFileNameW(&ofn)) return "[]";

    // Çoklu seçim: klasör\0dosya1\0dosya2\0\0 ; tek seçim: tam yol\0\0
    std::vector<std::wstring> paths;
    const wchar_t* cursor = buffer.data();
    const std::wstring first = cursor;
    cursor += first.size() + 1;
    if (*cursor == L'\0') {
        paths.push_back(first);
    } else {
        while (*cursor) {
            const std::wstring name = cursor;
            cursor += name.size() + 1;
            paths.push_back(first + L"\\" + name);
        }
    }
    return FilesJson(paths);
}

std::string FilesJson(const std::vector<std::wstring>& paths) {
    std::string json = "[";
    for (std::size_t i = 0; i < paths.size(); ++i) {
        if (i > 0) json += ",";
        json += FileRecordJson(paths[i]);
    }
    return json + "]";
}

HRESULT STDMETHODCALLTYPE FileDropHandler::QueryInterface(REFIID riid, void** ppv) {
    if (riid == IID_IUnknown || riid == IID_ICoreWebView2WebMessageReceivedEventHandler) {
        *ppv = static_cast<ICoreWebView2WebMessageReceivedEventHandler*>(this);
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE FileDropHandler::Invoke(ICoreWebView2* /*sender*/,
                                                  ICoreWebView2WebMessageReceivedEventArgs* args) {
    ICoreWebView2WebMessageReceivedEventArgs2* args2 = nullptr;
    if (FAILED(args->QueryInterface(IID_ICoreWebView2WebMessageReceivedEventArgs2,
                                    reinterpret_cast<void**>(&args2)))) {
        return S_OK;
    }

    ICoreWebView2ObjectCollectionView* objects = nullptr;
    if (SUCCEEDED(args2->get_AdditionalObjects(&objects)) && objects) {
        UINT count = 0;
        objects->get_Count(&count);

        std::vector<std::wstring> paths;
        for (UINT i = 0; i < count; ++i) {
            IUnknown* item = nullptr;
            if (FAILED(objects->GetValueAtIndex(i, &item)) || !item) continue;

            ICoreWebView2File* file = nullptr;
            if (SUCCEEDED(item->QueryInterface(IID_ICoreWebView2File, reinterpret_cast<void**>(&file)))) {
                LPWSTR path = nullptr;
                if (SUCCEEDED(file->get_Path(&path)) && path) {
                    paths.emplace_back(path);
                    CoTaskMemFree(path);
                }
                file->Release();
            }
            item->Release();
        }
        objects->Release();

        if (!paths.empty() && onDrop) onDrop(paths);
    }

    args2->Release();
    return S_OK;
}
