#pragma once
#include "archive.hpp"
namespace cn {
inline std::vector<DWORD> relocations(const PE& p){std::vector<DWORD> r;auto d=p.nt().OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];DWORD used=0;while(used<d.Size){if(d.Size-used<8)throw std::runtime_error("Bad relocation header");size_t off=p.offset(d.VirtualAddress+used,8);DWORD page=p.dword(off),size=p.dword(off+4);if(size<8||size>d.Size-used||size%2)throw std::runtime_error("Bad relocation block");off=p.offset(d.VirtualAddress+used,size);for(DWORD n=8;n<size;n+=2){WORD v=p.word(off+n);if((v>>12)==3)r.push_back(page+(v&4095));else if(v>>12)throw std::runtime_error("Unsupported relocation type");}used+=size;}return r;}
inline Bytes relocationBytes(std::vector<DWORD> rs){std::sort(rs.begin(),rs.end());rs.erase(std::unique(rs.begin(),rs.end()),rs.end());Bytes out;for(size_t i=0;i<rs.size();){DWORD page=rs[i]&~4095u;std::vector<WORD> words;while(i<rs.size()&&(rs[i]&~4095u)==page)words.push_back((WORD)(0x3000|(rs[i++]&4095)));if(words.size()%2)words.push_back(0);DWORD size=8+(DWORD)words.size()*2;size_t start=out.size();out.resize(start+size);memcpy(out.data()+start,&page,4);memcpy(out.data()+start+4,&size,4);memcpy(out.data()+start+8,words.data(),words.size()*2);}return out;}
inline std::vector<IMAGE_IMPORT_DESCRIPTOR> imports(const PE& p){std::vector<IMAGE_IMPORT_DESCRIPTOR> out;auto d=p.nt().OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];if(!d.VirtualAddress)return out;for(DWORD n=0;n<2048;++n){size_t off=p.offset(d.VirtualAddress+n*20,20);IMAGE_IMPORT_DESCRIPTOR e;memcpy(&e,p.b.data()+off,20);if(!e.Name)return out;out.push_back(e);}throw std::runtime_error("Import descriptor limit");}
inline DWORD exported(const PE& p,const char* name){auto d=p.nt().OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];size_t off=p.offset(d.VirtualAddress,sizeof(IMAGE_EXPORT_DIRECTORY));IMAGE_EXPORT_DIRECTORY e;memcpy(&e,p.b.data()+off,sizeof(e));if(e.NumberOfNames>1024||e.NumberOfFunctions>1024)throw std::runtime_error("Invalid runtime exports");for(DWORD i=0;i<e.NumberOfNames;++i){DWORD rva=p.dword(p.offset(e.AddressOfNames+i*4,4));auto text=p.z(rva);if(text==name||text==std::string("_")+name){WORD ord=p.word(p.offset(e.AddressOfNameOrdinals+i*2,2));if(ord>=e.NumberOfFunctions)throw std::runtime_error("Invalid export ordinal");return p.dword(p.offset(e.AddressOfFunctions+ord*4,4));}}throw std::runtime_error("Runtime CNInit export missing");}
inline std::string runtimeResource(){HRSRC r=FindResourceW(0,MAKEINTRESOURCEW(101),RT_RCDATA);if(!r)throw std::runtime_error("Embedded runtime missing");HGLOBAL h=LoadResource(0,r);return std::string((const char*)LockResource(h),SizeofResource(0,r));}
inline StringMap markerInfo(const PE& exe){
    for(unsigned i=0;i<exe.sections();++i){auto s=exe.section(i);if(memcmp(s.Name,".mktrans",8))continue;
        if(s.Misc.VirtualSize<15||s.Misc.VirtualSize>s.SizeOfRawData||s.Misc.VirtualSize>16384)throw std::runtime_error("Invalid moekuri_trans marker bounds");
        const char* p=(const char*)exe.b.data()+s.PointerToRawData;if(memcmp(p,"moekuri_trans\0",14))throw std::runtime_error("Invalid moekuri_trans signature");
        auto info=JsonParser(std::string(p+14,s.Misc.VirtualSize-14)).parse();if(info[L"signature"]!=L"moekuri_trans"||info[L"marker_version"]!=L"1")throw std::runtime_error("Unsupported moekuri_trans marker");
        return info;
    }return {};
}
inline std::string pack(const std::string& source,const std::vector<Row>& inputRows,Options opts,const std::string& inputText={}){
    const std::string expected="1c79a2d328d8ccd69765ac624b48f2317d80688c3dc7439ee3c23fc1fc05f847";
    if(hash(source)!=expected)throw std::runtime_error("仅支持未修改的日文 1.10 EXE。SHA256 不匹配，已停止打包。");
    PE game(source),rt(runtimeResource());auto filtered=opts.omitDxa?exeOnlyCatalog(opts.originalExe,inputRows):std::vector<Row>{};const auto& rows=opts.omitDxa?filtered:inputRows;auto originalText=opts.omitDxa?serializeRows(rows):inputText;DWORD init=exported(rt,"CNInit"),oldEntry=game.nt().OptionalHeader.AddressOfEntryPoint;
    auto rs=relocations(game),rtRs=relocations(rt);auto imp=imports(game),ri=imports(rt);game.reserveHeaders(rt.sections()+5);
    DWORD shift=game.nextRva()-rt.section(0).VirtualAddress;DWORD newBase=game.nt().OptionalHeader.ImageBase+shift,baseDelta=newBase-rt.nt().OptionalHeader.ImageBase;
    for(DWORD rva:rtRs){size_t p=rt.offset(rva,4);rt.put32(p,rt.dword(p)+baseDelta);rs.push_back(rva+shift);}
    // Preserve native imports and splice in the freestanding runtime's Win32 imports.
    for(auto e:ri){DWORD original=e.OriginalFirstThunk?e.OriginalFirstThunk:e.FirstThunk;
        for(DWORD n=0;n<16384;++n){size_t a=rt.offset(original+n*4,4),b=rt.offset(e.FirstThunk+n*4,4);DWORD v=rt.dword(a);if(!v)break;if(!(v&0x80000000u))v+=shift;rt.put32(a,v);rt.put32(b,v);if(n==16383)throw std::runtime_error("Runtime import limit");}
        e.OriginalFirstThunk=original+shift;e.FirstThunk+=shift;e.Name+=shift;e.TimeDateStamp=0;e.ForwarderChain=0;imp.push_back(e);
    }
    // Keep RX code, read-only constants and RW state in separate PE sections.
    for(unsigned i=0;i<rt.sections();++i){auto s=rt.section(i);if(!memcmp(s.Name,".reloc",6))continue;Bytes bytes(rt.b.begin()+s.PointerToRawData,rt.b.begin()+s.PointerToRawData+s.SizeOfRawData);const char* name=(s.Characteristics&IMAGE_SCN_MEM_EXECUTE)?".cncode":(s.Characteristics&IMAGE_SCN_MEM_WRITE)?".cnrw":".cnconst";game.add(name,bytes,s.Misc.VirtualSize,(s.Characteristics&~IMAGE_SCN_MEM_DISCARDABLE)|((s.Characteristics&IMAGE_SCN_MEM_EXECUTE)?0:IMAGE_SCN_MEM_WRITE),s.VirtualAddress+shift);}
    if(opts.subsetFont)for(const auto& r:extractExe(game))opts.extraCharacters+=r.source;Bytes data=compressPayload(makePayload(rows,opts,originalText));DWORD ro=game.add(".cnro",data,(DWORD)data.size(),IMAGE_SCN_CNT_INITIALIZED_DATA|IMAGE_SCN_MEM_READ);
    imp.push_back({});Bytes im(imp.size()*20);memcpy(im.data(),imp.data(),im.size());DWORD ip=game.add(".cnimp",im,(DWORD)im.size(),IMAGE_SCN_CNT_INITIALIZED_DATA|IMAGE_SCN_MEM_READ);
    game.nt().OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]={ip,(DWORD)im.size()};
    game.nt().OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT]={0,0};game.nt().OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]={0,0};
    DWORD entryRva=game.nextRva(),base=game.nt().OptionalHeader.ImageBase;
    Bytes entry={0x9c,0x60,0x68,0,0,0,0,0xe8,0,0,0,0,0x83,0xc4,4,0x61,0x9d,0xe9,0,0,0,0};
    DWORD ptr=base+ro,call=shift+init-(entryRva+12),jump=oldEntry-(entryRva+22);memcpy(entry.data()+3,&ptr,4);memcpy(entry.data()+8,&call,4);memcpy(entry.data()+18,&jump,4);rs.push_back(entryRva+3);
    game.add(".cnboot",entry,(DWORD)entry.size(),IMAGE_SCN_CNT_CODE|IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_EXECUTE,entryRva);
    Bytes rb=relocationBytes(rs);DWORD rp=game.add(".cnrel",rb,(DWORD)rb.size(),IMAGE_SCN_CNT_INITIALIZED_DATA|IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_DISCARDABLE);
    game.nt().OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]={rp,(DWORD)rb.size()};game.nt().OptionalHeader.AddressOfEntryPoint=entryRva;game.nt().OptionalHeader.CheckSum=0;game.nt().FileHeader.Characteristics&=~IMAGE_FILE_RELOCS_STRIPPED;
    // A small uncompressed, read-only PE marker is discoverable without running
    // the game or decompressing the translation payload. Versioned UTF-8 JSON
    // follows the NUL-terminated ASCII signature.
    const CNHeader* stored=payload(game);StringMap metadata={{L"signature",L"moekuri_trans"},{L"project",L"moekuri-trans"},{L"marker_version",L"1"},{L"game_version",L"JP 1.10"},{L"original_sha256",wide(expected)},{L"catalog_sha256",wide(hash(std::string((char*)stored+stored->text,stored->textBytes)))},{L"catalog_rows",std::to_wstring(rows.size())},{L"omit_dxa",opts.omitDxa?L"true":L"false"},{L"font_mode",opts.externalFont?L"external":stored->fontBytes?L"embedded":L"system"}};
    std::string marker="moekuri_trans";marker.push_back(0);marker+=serialize(metadata);Bytes markerBytes(marker.begin(),marker.end());game.add(".mktrans",markerBytes,(DWORD)markerBytes.size(),IMAGE_SCN_CNT_INITIALIZED_DATA|IMAGE_SCN_MEM_READ);
    DWORD code=0,initialized=0,uninitialized=0;for(unsigned i=0;i<game.sections();++i){auto s=game.section(i);if(s.Characteristics&IMAGE_SCN_CNT_CODE)code+=s.SizeOfRawData;if(s.Characteristics&IMAGE_SCN_CNT_INITIALIZED_DATA)initialized+=s.SizeOfRawData;if(s.Characteristics&IMAGE_SCN_CNT_UNINITIALIZED_DATA)uninitialized+=s.Misc.VirtualSize;}
    game.nt().OptionalHeader.SizeOfCode=code;game.nt().OptionalHeader.SizeOfInitializedData=initialized;game.nt().OptionalHeader.SizeOfUninitializedData=uninitialized;
    auto decoded=unpack(game);if(serializeRows(decoded)!=serializeRows(rows))throw std::runtime_error("Embedded TXT round-trip failed");
    return std::string((char*)game.b.data(),game.b.size());
}
}
