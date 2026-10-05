#pragma once
#include <fontsub.h>
#include <cstdlib>
#include <string>
#include <set>
#include <vector>
namespace cn {
inline std::string reduceFont(const std::string& font,const std::wstring& chars,bool& reduced){
    reduced=false;if(font.size()<12||font.compare(0,4,"OTTO")==0)return font;
    std::set<USHORT> keep;for(unsigned c=0;c<256;++c)keep.insert((USHORT)c);for(unsigned c=0x3000;c<0x3100;++c)keep.insert((USHORT)c);
    for(auto c:chars){unsigned code=(USHORT)c;
        // Retain the complete font for supplementary code points or complex shaping scripts.
        if(code>=0xd800&&code<=0xdfff)return font;keep.insert((USHORT)c);}
    if(keep.size()>65535)return font;std::vector<USHORT> codes(keep.begin(),keep.end());BYTE* output=0;ULONG capacity=0,written=0;
    ULONG status=CreateFontPackage((const BYTE*)font.data(),(ULONG)font.size(),&output,&capacity,&written,TTFCFP_FLAGS_SUBSET,0,TTFCFP_SUBSET,TTFCFP_LANG_KEEP_ALL,TTFCFP_MS_PLATFORMID,TTFCFP_UNICODE_CHAR_SET,codes.data(),(USHORT)codes.size(),std::malloc,std::realloc,std::free,0);
    if(status||!output||!written||written>=font.size()){if(output)std::free(output);return font;}
    std::string result((char*)output,written);std::free(output);reduced=true;return result;
}
}
