#pragma once
#include <windows.h>
// MoeKuri uses the legacy DXArchive v4 stream, not the newer v7/v8 container.
inline const BYTE* moeDxaKey(){static const BYTE key[12]={0x92,0xf6,0xe4,0x68,0x9e,0xc1,0x90,0x32,0xcb,0x1e,0,0xa3};return key;}
inline DWORD moeDxa32(const BYTE* b){return (DWORD)b[0]|((DWORD)b[1]<<8)|((DWORD)b[2]<<16)|((DWORD)b[3]<<24);}
inline bool moeDxaDecode(const BYTE* b,DWORD bytes,BYTE* out,DWORD expected){
    if(bytes<9||moeDxa32(b)!=expected||moeDxa32(b+4)!=bytes)return false;
    DWORD p=9,o=0;BYTE escape=b[8];
    while(p<bytes){BYTE v=b[p++];if(v!=escape){if(o>=expected)return false;out[o++]=v;continue;}
        if(p>=bytes)return false;DWORD code=b[p++];if(code==escape){if(o>=expected)return false;out[o++]=escape;continue;}
        if(code>escape)--code;DWORD count=code>>3;if(code&4){if(p>=bytes)return false;count|=(DWORD)b[p++]<<5;}count+=4;
        DWORD width=(code&3)+1;if(width>3||width>bytes-p)return false;DWORD distance=0;
        for(DWORD i=0;i<width;++i)distance|=(DWORD)b[p++]<<(i*8);++distance;
        if(distance>o||count>expected-o)return false;while(count--){out[o]=out[o-distance];++o;}
    }return o==expected;
}
