#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// Windows API'leri UTF-16, ağ ve arayüz ise UTF-8 kullanır. Dönüşümler burada toplanır.
std::string Utf8FromWide(std::wstring_view wide);
std::wstring WideFromUtf8(std::string_view utf8);
std::filesystem::path PathFromUtf8(std::string_view utf8);

// Metni çift tırnaklı bir JSON string'ine çevirir. Aynı zamanda geçerli bir JavaScript literalidir.
std::string JsonQuote(std::string_view utf8);

// ["a", "b\"c"] biçimindeki JSON dizisini string listesine çevirir. Yalnızca string elemanları okur.
std::vector<std::string> ParseJsonStringArray(std::string_view json);
