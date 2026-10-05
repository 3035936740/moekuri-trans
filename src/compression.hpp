#pragma once
#include "format.h"
#include <compressapi.h>
#include <vector>
#include <stdexcept>
namespace cn {
inline std::vector<BYTE> compressPayload(const std::vector<BYTE>& raw){
    COMPRESSOR_HANDLE compressor=0;if(!CreateCompressor(COMPRESS_ALGORITHM_XPRESS_HUFF,0,&compressor))throw std::runtime_error("Cannot create payload compressor");
    SIZE_T count=0;BOOL query=Compress(compressor,raw.data(),raw.size(),0,0,&count);DWORD error=GetLastError();
    if((!query&&error!=ERROR_INSUFFICIENT_BUFFER)||count>0xffffffffu-sizeof(CNCompressedHeader)){CloseCompressor(compressor);throw std::runtime_error("Compression size query failed");}
    std::vector<BYTE> result(sizeof(CNCompressedHeader)+count);BOOL ok=Compress(compressor,raw.data(),raw.size(),result.data()+sizeof(CNCompressedHeader),count,&count);CloseCompressor(compressor);
    if(!ok)throw std::runtime_error("Payload compression failed");result.resize(sizeof(CNCompressedHeader)+count);
    CNCompressedHeader h{};memcpy(h.magic,"MOECNZ11",8);h.version=2;h.total=(DWORD)result.size();h.rawBytes=(DWORD)raw.size();h.algorithm=COMPRESS_ALGORITHM_XPRESS_HUFF;h.packedBytes=(DWORD)count;h.checksum=payloadChecksum(raw.data(),raw.size());memcpy(result.data(),&h,sizeof(h));return result;
}
inline std::vector<BYTE> decompressPayload(const BYTE* bytes,SIZE_T available){
    if(available<sizeof(CNCompressedHeader))throw std::runtime_error("Truncated compressed payload");CNCompressedHeader h;memcpy(&h,bytes,sizeof(h));
    if(memcmp(h.magic,"MOECNZ11",8)||h.version!=2||h.total>available||h.total<sizeof(h)||h.packedBytes!=h.total-sizeof(h)||h.rawBytes<sizeof(CNHeader)||h.rawBytes>256u*1024u*1024u||h.algorithm!=COMPRESS_ALGORITHM_XPRESS_HUFF)throw std::runtime_error("Invalid compressed payload bounds");
    DECOMPRESSOR_HANDLE decompressor=0;if(!CreateDecompressor(h.algorithm,0,&decompressor))throw std::runtime_error("Cannot create payload decompressor");std::vector<BYTE> raw(h.rawBytes);SIZE_T count=0;BOOL ok=Decompress(decompressor,bytes+sizeof(h),h.packedBytes,raw.data(),raw.size(),&count);CloseDecompressor(decompressor);
    if(!ok||count!=raw.size()||payloadChecksum(raw.data(),raw.size())!=h.checksum)throw std::runtime_error("Compressed payload checksum mismatch");return raw;
}
}
