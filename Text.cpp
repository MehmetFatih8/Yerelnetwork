#include "Text.h"

#include <charconv>
#include <cstdio>
#include <system_error>

#include <windows.h>

std::string Utf8FromWide(std::wstring_view wide) {
    if (wide.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                                         nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                        utf8.data(), size, nullptr, nullptr);
    return utf8;
}

std::wstring WideFromUtf8(std::string_view utf8) {
    if (utf8.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), size);
    return wide;
}

std::filesystem::path PathFromUtf8(std::string_view utf8) {
    // C++20: char8_t dizisi UTF-8 olarak yorumlanır, Windows'ta geniş yola dönüşür
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(utf8.data()), utf8.size()));
}

std::string JsonQuote(std::string_view utf8) {
    std::string out = "\"";
    for (unsigned char c : utf8) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20) {
                char escaped[8];
                std::snprintf(escaped, sizeof(escaped), "\\u%04x", c);
                out += escaped;
            } else {
                out += static_cast<char>(c);
            }
        }
    }
    out += '"';
    return out;
}

namespace {

bool IsJsonSpace(char c) {
    return c == ' ' || c == ',' || c == '\t' || c == '\r' || c == '\n';
}

// pos, açılış tırnağında olmalı. Okunan string out'a eklenir ve pos kapanış tırnağının sonrasına ilerler.
bool ReadJsonString(std::string_view s, std::size_t& pos, std::string& out) {
    if (pos >= s.size() || s[pos] != '"') return false;
    ++pos;

    while (pos < s.size()) {
        const char c = s[pos++];
        if (c == '"') return true;
        if (c != '\\') {
            out += c;
            continue;
        }
        if (pos >= s.size()) return false;

        const char escape = s[pos++];
        switch (escape) {
        case '"':  out += '"'; break;
        case '\\': out += '\\'; break;
        case '/':  out += '/'; break;
        case 'b':  out += '\b'; break;
        case 'f':  out += '\f'; break;
        case 'n':  out += '\n'; break;
        case 'r':  out += '\r'; break;
        case 't':  out += '\t'; break;
        case 'u': {
            if (pos + 4 > s.size()) return false;
            unsigned int code = 0;
            const auto result = std::from_chars(s.data() + pos, s.data() + pos + 4, code, 16);
            if (result.ec != std::errc{}) return false;
            pos += 4;
            // JSON.stringify yalnızca kontrol karakterlerini \u ile kaçışlar; BMP aralığı yeterli
            if (code < 0x80) {
                out += static_cast<char>(code);
            } else if (code < 0x800) {
                out += static_cast<char>(0xC0 | (code >> 6));
                out += static_cast<char>(0x80 | (code & 0x3F));
            } else {
                out += static_cast<char>(0xE0 | (code >> 12));
                out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (code & 0x3F));
            }
            break;
        }
        default:
            return false;
        }
    }
    return false;
}

}  // namespace

std::vector<std::string> ParseJsonStringArray(std::string_view json) {
    std::vector<std::string> result;
    std::size_t pos = json.find('[');
    if (pos == std::string_view::npos) return result;
    ++pos;

    while (pos < json.size()) {
        while (pos < json.size() && IsJsonSpace(json[pos])) ++pos;
        if (pos >= json.size() || json[pos] == ']') break;

        std::string item;
        if (!ReadJsonString(json, pos, item)) break;
        result.push_back(std::move(item));
    }
    return result;
}
