#pragma once
#include <windows.h>
// All offsets are relative to the beginning of the immutable .cnro section.
// The original game's UTF-16 ABI is retained; editable/storage text is UTF-8.
struct CNHeader {
    char magic[8];
    DWORD version, total, count, records, text, textBytes, font, fontBytes, face;
    LONG height, width;
    DWORD charset, scale, maxWidth, minScale, textRva, reserved;
};
// reserved flags: 1 subset, 2 external, 4 omit DXA, 8 font.dxa, 16 per-row scale.
// Flag 16 stores scale percent in budget bits 16..31, pixel width in bits 0..15.
struct CNRecord { DWORD source, target, hash, budget; };
// Version 2 is a compressed container around the unchanged version 1 payload.
struct CNCompressedHeader {char magic[8];DWORD version,total,rawBytes,algorithm,packedBytes,checksum;};
inline DWORD payloadChecksum(const BYTE* bytes,SIZE_T count){DWORD h=2166136261u;for(SIZE_T i=0;i<count;++i){h^=bytes[i];h*=16777619u;}return h;}
