#pragma once
#include <winsock2.h>
#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace moe {
using StringMap = std::map<std::wstring, std::wstring>;
inline std::wstring wide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), nullptr, 0);
    if (!n) throw std::runtime_error("Invalid UTF-8");
    std::wstring r(n, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), &r[0], n);
    return r;
}
inline std::string utf8(const std::wstring& s) {
    if (s.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    if (!n) throw std::runtime_error("Invalid UTF-16");
    std::string r(n, '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s.data(), (int)s.size(), &r[0], n, nullptr, nullptr);
    return r;
}
class JsonParser {
    const std::string& s;
    size_t p = 0;
    [[noreturn]] void fail(const char* why) const {
        throw std::runtime_error(std::string(why) + " at byte " + std::to_string(p));
    }
    void ws() { while (p < s.size() && (s[p] == ' ' || s[p] == '\t' || s[p] == '\n' || s[p] == '\r')) ++p; }
    bool take(char c) { ws(); if (p < s.size() && s[p] == c) { ++p; return true; } return false; }
    unsigned hex4() {
        unsigned u = 0;
        for (int i = 0; i < 4; ++i) {
            if (p == s.size()) fail("Incomplete Unicode escape");
            unsigned char c = s[p++];
            unsigned v = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 16;
            if (v == 16) fail("Invalid Unicode escape");
            u = (u << 4) | v;
        }
        return u;
    }
    std::wstring string() {
        if (!take('"')) fail("Expected a JSON string");
        std::wstring r;
        std::string raw;
        auto flush = [&]() { r += wide(raw); raw.clear(); };
        while (p < s.size()) {
            unsigned char c = s[p++];
            if (c == '"') { flush(); return r; }
            if (c < 0x20) fail("Unescaped control character");
            if (c != '\\') { raw += (char)c; continue; }
            flush();
            if (p == s.size()) fail("Incomplete escape");
            switch (s[p++]) {
                case '"': r += L'"'; break;
                case '\\': r += L'\\'; break;
                case '/': r += L'/'; break;
                case 'b': r += L'\b'; break;
                case 'f': r += L'\f'; break;
                case 'n': r += L'\n'; break;
                case 'r': r += L'\r'; break;
                case 't': r += L'\t'; break;
                case 'u': {
                    unsigned u = hex4();
                    if (u >= 0xd800 && u <= 0xdbff) {
                        if (p + 2 > s.size() || s[p] != '\\' || s[p + 1] != 'u') fail("Missing low surrogate");
                        p += 2;
                        unsigned low = hex4();
                        if (low < 0xdc00 || low > 0xdfff) fail("Invalid low surrogate");
                        r += (wchar_t)u; r += (wchar_t)low;
                    } else {
                        if (u >= 0xdc00 && u <= 0xdfff) fail("Unpaired low surrogate");
                        r += (wchar_t)u;
                    }
                    break;
                }
                default: fail("Invalid escape");
            }
        }
        fail("Unterminated string");
    }
public:
    explicit JsonParser(const std::string& input) : s(input) {}
    StringMap parse() {
        if (s.compare(0, 3, "\xef\xbb\xbf") == 0) p = 3;
        StringMap r;
        if (!take('{')) fail("Expected a flat JSON object");
        if (!take('}')) for (;;) {
            std::wstring k = string();
            if (!take(':')) fail("Expected colon");
            std::wstring v = string();
            if (!r.emplace(std::move(k), std::move(v)).second) fail("Duplicate key");
            if (take('}')) break;
            if (!take(',')) fail("Expected comma");
        }
        ws();
        if (p != s.size()) fail("Trailing data");
        return r;
    }
};
inline std::string quote(const std::wstring& s) {
    std::string r = "\"";
    for (unsigned char c : utf8(s)) {
        switch (c) {
            case '"': r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n"; break;
            case '\r': r += "\\r"; break;
            case '\t': r += "\\t"; break;
            default:
                if (c < 0x20) {
                    const char* h = "0123456789abcdef";
                    r += "\\u00"; r += h[c >> 4]; r += h[c & 15];
                } else r += (char)c;
        }
    }
    return r + '"';
}
inline std::string serialize(const StringMap& m) {
    std::string r = "{\n";
    bool first = true;
    for (const auto& e : m) {
        if (!first) r += ",\n";
        first = false;
        r += "  " + quote(e.first) + ": " + quote(e.second);
    }
    return r + "\n}\n";
}
inline std::wstring winFilePath(const std::wstring& path){
    if(path.rfind(L"\\\\?\\",0)==0)return path;
    auto count=GetFullPathNameW(path.c_str(),0,nullptr,nullptr);if(!count || count>32760)throw std::runtime_error("Invalid Windows file path");
    std::wstring full(count,L'\0');auto written=GetFullPathNameW(path.c_str(),count,&full[0],nullptr);if(!written || written>=count)throw std::runtime_error("Windows file path changed");full.resize(written);
    if(full.size()<240)return full;
    return full.rfind(L"\\\\",0)==0?L"\\\\?\\UNC\\"+full.substr(2):L"\\\\?\\"+full;
}
inline std::string readFile(const std::wstring& path) {
    auto native=winFilePath(path);HANDLE h = CreateFileW(native.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open file (Win32 " + std::to_string(GetLastError()) + ")");
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(h, &size) || size.QuadPart < 0 || size.QuadPart > 128 * 1024 * 1024) { CloseHandle(h); throw std::runtime_error("File exceeds 128 MiB"); }
    std::string r((size_t)size.QuadPart, '\0');
    DWORD count = 0;
    BOOL ok = ReadFile(h, r.empty() ? nullptr : &r[0], (DWORD)r.size(), &count, nullptr);
    CloseHandle(h);
    if (!ok || count != r.size()) throw std::runtime_error("Cannot read complete file");
    return r;
}
inline bool writeAtomic(const std::wstring& path, const std::string& bytes) {
    if(path.empty())return false;
    std::wstring temp = path + L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
    std::wstring nativeTemp,native;try{nativeTemp=winFilePath(temp);native=winFilePath(path);}catch(const std::exception&){return false;}
    HANDLE h = CreateFileW(nativeTemp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD n = 0;
    bool ok = WriteFile(h, bytes.data(), (DWORD)bytes.size(), &n, nullptr) && n == bytes.size() && FlushFileBuffers(h);
    CloseHandle(h);
    if (ok) ok = !!MoveFileExW(nativeTemp.c_str(), native.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!ok) DeleteFileW(nativeTemp.c_str());
    return ok;
}
}
