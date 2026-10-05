#pragma once
#include "json.hpp"

namespace moe {
// Read the family name from an unmodified TTF/OTF sfnt name table. Bound every
// offset before access, so an invalid optional font cannot break translation.
inline std::wstring fontFamily(const std::string& bytes) {
    auto u16 = [&](size_t p) -> unsigned {
        if (p > bytes.size() || bytes.size() - p < 2) throw std::runtime_error("Truncated font table");
        return (unsigned)(unsigned char)bytes[p] * 256 + (unsigned char)bytes[p + 1];
    };
    auto u32 = [&](size_t p) -> uint32_t { return ((uint32_t)u16(p) << 16) | u16(p + 2); };
    uint32_t signature = u32(0);
    if (signature != 0x00010000 && signature != 0x4f54544f && signature != 0x74727565)
        throw std::runtime_error("Expected a single TTF/OTF font");
    unsigned tables = u16(4);
    if (!tables || tables > 256) throw std::runtime_error("Invalid font table count");
    for (unsigned t = 0; t < tables; ++t) {
        size_t p = 12 + t * 16;
        if (u32(p) != 0x6e616d65) continue;
        size_t start = u32(p + 8), length = u32(p + 12);
        if (start > bytes.size() || length > bytes.size() - start || length < 6)
            throw std::runtime_error("Invalid font name table bounds");
        unsigned count = u16(start + 2), strings = u16(start + 4);
        if (count > (length - 6) / 12 || strings > length) throw std::runtime_error("Invalid font name records");
        int best = -1; std::wstring family;
        for (unsigned i = 0; i < count; ++i) {
            p = start + 6 + i * 12;
            unsigned platform = u16(p), language = u16(p + 4), name = u16(p + 6);
            size_t n = u16(p + 8), offset = u16(p + 10);
            if (name != 1 || (platform != 0 && platform != 3)) continue;
            if (n > 1024 || n % 2 || offset > length - strings || n > length - strings - offset)
                throw std::runtime_error("Invalid font family string");
            std::wstring value;
            for (size_t j = 0; j < n; j += 2) value += (wchar_t)u16(start + strings + offset + j);
            if (value.empty() || value.size() >= LF_FACESIZE || value.find(L'\0') != std::wstring::npos) continue;
            // GDI uses the legacy family (name ID 1); choose its English name
            // first to avoid dependence on the user's Windows language.
            int score = (platform == 3 ? 10 : 0) + (language == 0x409 ? 5 : language == 0x804 ? 4 : 0);
            if (score > best) { utf8(value); best = score; family = std::move(value); }
        }
        if (!family.empty()) return family;
        throw std::runtime_error("Font has no usable Unicode family name");
    }
    throw std::runtime_error("Font name table missing");
}
}
