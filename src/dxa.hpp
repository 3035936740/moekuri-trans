#pragma once
#include "archive.hpp"
#include "dxa_codec.h"
#include <unordered_map>
namespace cn {
inline void dxaPut(Bytes& b,size_t p,DWORD v){if(p>b.size()||b.size()-p<4)throw std::runtime_error("DXA write bounds");memcpy(b.data()+p,&v,4);}
// Bounded hash chains keep packing off the UI thread and avoid quadratic searches.
inline Bytes dxaCompress(const Bytes& src){
    DWORD frequencies[256]={};for(BYTE c:src)++frequencies[c];BYTE escape=0;for(unsigned i=1;i<256;++i)if(frequencies[i]<frequencies[escape])escape=(BYTE)i;
    Bytes out(9);out[8]=escape;std::vector<int> head(65536,-1),previous(src.size(),-1);
    auto key=[&](size_t p){return ((src[p]*251u+src[p+1])*251u+src[p+2])&65535u;};
    auto add=[&](size_t p){if(p+3<=src.size()){auto h=key(p);previous[p]=head[h];head[h]=(int)p;}};
    for(size_t p=0;p<src.size();){size_t best=0,distance=0;int candidate=p+4<=src.size()?head[key(p)]:-1;
        for(unsigned visits=0;candidate>=0&&visits<32;++visits){size_t d=p-(size_t)candidate;if(d>0x1000000u)break;size_t n=0,limit=std::min<size_t>(8195,src.size()-p);while(n<limit&&src[candidate+n]==src[p+n])++n;if(n>=4&&n>best){best=n;distance=d;if(n==limit)break;}candidate=previous[candidate];}
        unsigned width=distance<=256?1:distance<=65536?2:3;size_t overhead=2+width+(best>=36?1:0);
        if(best>=4&&best>overhead){DWORD count=(DWORD)best-4;BYTE code=(BYTE)(((count&31)<<3)|(count>=32?4:0)|(width-1));if(code>=escape)++code;
            out.push_back(escape);out.push_back(code);if(count>=32)out.push_back((BYTE)(count>>5));DWORD d=(DWORD)distance-1;for(unsigned i=0;i<width;++i)out.push_back((BYTE)(d>>(i*8)));
            for(size_t i=0;i<best;++i)add(p+i);p+=best;
        }else{BYTE v=src[p];out.push_back(v);if(v==escape)out.push_back(v);add(p++);}
    }dxaPut(out,0,(DWORD)src.size());dxaPut(out,4,(DWORD)out.size());return out;
}
inline void dxaSafeName(const std::wstring& name){
    if(name.empty()||name==L"."||name==L".."||name.size()>255||name.back()==L'.'||name.back()==L' '||name.find_first_of(L"/\\:<>\"|?*")!=name.npos)throw std::runtime_error("DXA unsafe filename");
    for(auto c:name)if(c<32)throw std::runtime_error("DXA control character in filename");
    auto stem=name.substr(0,name.find(L'.'));for(auto& c:stem)if(c>=L'a'&&c<=L'z')c-=32;
    if(stem==L"CON"||stem==L"PRN"||stem==L"AUX"||stem==L"NUL"||stem==L"CONIN$"||stem==L"CONOUT$"||(stem.size()==4&&(stem.substr(0,3)==L"COM"||stem.substr(0,3)==L"LPT")&&stem[3]>=L'0'&&stem[3]<=L'9'))throw std::runtime_error("DXA reserved Windows filename");
}
inline DWORD dxaName(Bytes& names,const std::wstring& name){
    dxaSafeName(name);BOOL fallback=FALSE;int n=WideCharToMultiByte(932,WC_NO_BEST_FIT_CHARS,name.c_str(),-1,0,0,0,&fallback);std::string bytes(n,0);
    if(!n||!WideCharToMultiByte(932,WC_NO_BEST_FIT_CHARS,name.c_str(),-1,bytes.data(),n,0,&fallback)||fallback)throw std::runtime_error("DXA v4 filenames must be representable in CP932");
    DWORD offset=(DWORD)names.size(),words=(n+3)/4;names.resize(offset+4+words*8);WORD parity=0;memcpy(names.data()+offset,&words,2);memcpy(names.data()+offset+4+words*4,bytes.data(),n);
    for(int i=0;i<n-1;){BYTE c=(BYTE)bytes[i];unsigned width=IsDBCSLeadByteEx(932,c)?2:1;for(unsigned j=0;j<width&&i<n-1;++j,++i){BYTE v=(BYTE)bytes[i];if(width==1&&v>='a'&&v<='z')v-=32;names[offset+4+i]=v;parity+=v;}}
    memcpy(names.data()+offset+2,&parity,2);return offset;
}
struct DxaNode {std::wstring name;const Bytes* data=nullptr;std::map<std::wstring,DxaNode> children;};
inline Bytes dxaPack(const std::map<std::wstring,Bytes>& files,const std::set<std::wstring>& directories={}){
    DxaNode root;size_t total=0,entries=0;std::set<std::wstring> paths;
    auto insert=[&](const std::wstring& path,const Bytes* bytes){if(std::count(path.begin(),path.end(),L'/')>64)throw std::runtime_error("DXA directory depth");if(path.empty()||path.front()==L'/'||path.find(L'\\')!=path.npos)throw std::runtime_error("DXA unsafe path");DxaNode* node=&root;size_t p=0;
        for(;;){size_t end=path.find(L'/',p);auto name=path.substr(p,end==path.npos?path.npos:end-p);dxaSafeName(name);auto folded=name;for(auto& c:folded)c=(wchar_t)towupper(c);auto& next=node->children[folded];if(next.name.empty()){next.name=name;if(++entries>10000)throw std::runtime_error("DXA entry limit");}else if(next.name!=name)throw std::runtime_error("DXA case-insensitive filename collision");node=&next;if(end==path.npos)break;if(node->data)throw std::runtime_error("DXA file/directory collision");p=end+1;}
        if(bytes){if(node->data||!node->children.empty())throw std::runtime_error("DXA path collision");node->data=bytes;}};
    for(const auto& dir:directories)insert(dir,nullptr);
    for(const auto& f:files){if(f.second.size()>32*1024*1024)throw std::runtime_error("DXA file exceeds 32 MiB");total+=f.second.size();if(total>512u*1024u*1024u)throw std::runtime_error("DXA total exceeds 512 MiB");insert(f.first,&f.second);}
    Bytes names(4),heads,dirs(16),data;
    std::function<void(const DxaNode&,DWORD,DWORD,DWORD)> emit=[&](const DxaNode& node,DWORD dir,DWORD parent,DWORD own){
        DWORD first=(DWORD)heads.size();heads.resize(first+node.children.size()*44);dxaPut(dirs,dir,own);dxaPut(dirs,dir+4,parent);dxaPut(dirs,dir+8,(DWORD)node.children.size());dxaPut(dirs,dir+12,first);
        DWORD i=0;for(const auto& item:node.children){const auto& child=item.second;DWORD h=first+i++*44;dxaPut(heads,h,dxaName(names,child.name));dxaPut(heads,h+4,child.data?FILE_ATTRIBUTE_ARCHIVE:FILE_ATTRIBUTE_DIRECTORY);dxaPut(heads,h+40,0xffffffffu);
            if(!child.data){DWORD next=(DWORD)dirs.size();dirs.resize(next+16);dxaPut(heads,h+32,next);emit(child,next,dir,h);}else{auto compressed=dxaCompress(*child.data);bool use=compressed.size()<child.data->size();dxaPut(heads,h+32,(DWORD)data.size());dxaPut(heads,h+36,(DWORD)child.data->size());if(use)dxaPut(heads,h+40,(DWORD)compressed.size());const auto& bytes=use?compressed:*child.data;data.insert(data.end(),bytes.begin(),bytes.end());data.resize(align((DWORD)data.size(),4));}}
    };emit(root,0,0xffffffffu,0xffffffffu);
    Bytes out(28);out[0]='D';out[1]='X';out[2]=4;dxaPut(out,4,(DWORD)(names.size()+heads.size()+dirs.size()));dxaPut(out,8,28);dxaPut(out,12,28+(DWORD)data.size());dxaPut(out,16,(DWORD)names.size());dxaPut(out,20,(DWORD)(names.size()+heads.size()));dxaPut(out,24,932);
    for(const auto* part:{&data,&names,&heads,&dirs})out.insert(out.end(),part->begin(),part->end());for(size_t i=0;i<out.size();++i)out[i]^=moeDxaKey()[i%12];return out;
}
inline void dxaNoReparse(const std::filesystem::path& path){auto absolute=std::filesystem::absolute(path).lexically_normal();for(auto p=absolute;!p.empty();p=p.parent_path()){DWORD attrs=GetFileAttributesW(p.c_str());if(attrs!=INVALID_FILE_ATTRIBUTES&&(attrs&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("DXA refuses junctions/symlinks");if(p==p.parent_path())break;}}
inline size_t unpackArchive(const std::wstring& archive,const std::wstring& destination){
    auto root=std::filesystem::absolute(destination).lexically_normal();dxaNoReparse(root);Archive input(archive);auto files=input.files();std::vector<std::pair<std::filesystem::path,const Bytes*>> outputs;
    for(const auto& f:files){auto relative=std::filesystem::path(f.first);for(const auto& part:relative)dxaSafeName(part.wstring());auto out=root/relative;dxaNoReparse(out);if(std::filesystem::exists(out))throw std::runtime_error("DXA output file already exists; choose an empty folder");outputs.emplace_back(out,&f.second);}
    std::filesystem::create_directories(root);for(const auto& dir:input.directories()){auto rel=std::filesystem::path(dir);for(const auto& part:rel)dxaSafeName(part.wstring());dxaNoReparse(root/rel);std::filesystem::create_directories(root/rel);}for(const auto& f:outputs){std::filesystem::create_directories(f.first.parent_path());save(f.first.wstring(),std::string((char*)f.second->data(),f.second->size()));}return files.size();
}
inline size_t packArchive(const std::wstring& directory,const std::wstring& destination){
    auto root=std::filesystem::absolute(directory).lexically_normal(),out=std::filesystem::absolute(destination).lexically_normal();dxaNoReparse(root);dxaNoReparse(out);if(!std::filesystem::is_directory(root))throw std::runtime_error("DXA input must be a folder");
    std::map<std::wstring,Bytes> files;std::set<std::wstring> dirs;size_t total=0;
    for(const auto& file:std::filesystem::recursive_directory_iterator(root)){dxaNoReparse(file.path());if(file.path().lexically_normal()==out)throw std::runtime_error("DXA output cannot be inside the input folder");auto rel=file.path().lexically_relative(root).generic_wstring();if(file.is_directory())dirs.insert(rel);else if(file.is_regular_file()){auto size=file.file_size();total+=(size_t)size;if(size>32*1024*1024||total>512u*1024u*1024u||files.size()+dirs.size()>=10000)throw std::runtime_error("DXA size/entry limit");auto b=readFile(file.path().wstring());files[rel]=Bytes(b.begin(),b.end());}else throw std::runtime_error("DXA unsupported file type");}
    // Never include an output archive within the selected tree, including a new one.
    auto relative=out.lexically_relative(root);if(!relative.empty()&&*relative.begin()!=L"..")throw std::runtime_error("DXA output must be outside the input folder");auto bytes=dxaPack(files,dirs);save(destination,std::string((char*)bytes.data(),bytes.size()));return files.size();
}
inline std::string fontArchive(const std::string& bytes){fontFamily(bytes);std::map<std::wstring,Bytes> files;bool otf=bytes.size()>=4&&bytes.compare(0,4,"OTTO")==0;files[otf?L"font.otf":L"font.ttf"]=Bytes(bytes.begin(),bytes.end());auto packed=dxaPack(files);return std::string((char*)packed.data(),packed.size());}
inline std::string selectedFont(const std::vector<Row>& rows,const Options& opts){auto font=opts.font.empty()?opts.fontMemory:readFile(opts.font);if(font.empty())throw std::runtime_error("Select a TTF/OTF file before exporting font.dxa");fontFamily(font);if(opts.subsetFont){std::wstring chars=opts.extraCharacters;bool complex=false;for(const auto& row:rows){chars+=row.source;chars+=row.target;for(auto c:row.target)if(c>=0x590&&c<=0xeff)complex=true;}if(!complex){bool reduced=false;font=reduceFont(font,chars,reduced);}}return font;}
inline void exportExternalFont(const std::wstring& output,const std::vector<Row>& rows,const Options& opts){if(!opts.externalFont&&!opts.fontDxa)return;if(opts.font.empty()&&opts.fontMemory.empty()){if(opts.fontDxa)throw std::runtime_error("font.dxa mode requires a TTF/OTF file");return;}auto bytes=selectedFont(rows,opts);auto name=opts.fontDxa?L"font.dxa":bytes.compare(0,4,"OTTO")==0?L"font.otf":L"font.ttf";auto destination=std::filesystem::path(output).parent_path()/name;
    // Preserve a source font selected directly from the output directory.
    if(!opts.fontDxa&&!opts.font.empty()&&std::filesystem::weakly_canonical(opts.font)==std::filesystem::weakly_canonical(destination))return;
    save(destination.wstring(),opts.fontDxa?fontArchive(bytes):bytes);
}

}
