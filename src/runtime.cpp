#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "format.h"
#include "dxa_codec.h"
#include <compressapi.h>
extern "C" int _fltused = 0;
extern "C" void* __cdecl memset(void* p,int c,size_t n) { unsigned char* b=(unsigned char*)p; while(n--)*b++=(unsigned char)c; return p; }
extern "C" void* __cdecl memcpy(void* d,const void* s,size_t n) { unsigned char* a=(unsigned char*)d;const unsigned char* b=(const unsigned char*)s;while(n--)*a++=*b++;return d; }
#pragma function(memcpy, memset)
struct Entry { const wchar_t* source; const wchar_t* target; DWORD hash,budget; int sourceLen,targetLen; };
static const CNHeader* cfg;
static Entry* entries;
static DWORD entryCount;
static const wchar_t* fontFace;
static BYTE* game;
static DWORD fontCount;
static DWORD externalFontFile;
static volatile LONG drawCount, widthCount, translatedCount;
static volatile LONG createdFontCount;
using ResourceGets=int(__cdecl*)(wchar_t*,int,int);
static ResourceGets originalGets;
using ResourceOpen=int(__cdecl*)(const wchar_t*,int,int);
static ResourceOpen originalOpen;
struct ResourceState { volatile LONG handle,kind,dialogue; };
static ResourceState resourceStates[64];
static volatile LONG resourceSlot;
static int* readerInitialized;
static volatile LONG resourceTranslatedCount;
static DWORD resourceProbeThread;
static const Entry* resourceProbeEntry;
using Backend=int(__cdecl*)(int,int,int,int,double,double,const wchar_t*,unsigned,void*,const RECT*,int,int,unsigned,int,int,SIZE*);
static Backend originalDraw;
static decltype(&CreateFontW) originalFont;
static decltype(&SetWindowTextW) originalTitle;
static decltype(&MessageBoxW) originalMessage;
static decltype(&CreateWindowExW) originalWindow;
static int len(const wchar_t* s,int bound=4096){int n=0;if(s)while(n<bound && s[n])++n;return n;}
static DWORD hashText(const wchar_t* s,int n){DWORD h=2166136261u;for(int i=0;i<n;++i){h^=(unsigned short)s[i];h*=16777619u;}return h;}
static bool same(const wchar_t* a,const wchar_t* b,int n){for(int i=0;i<n;++i)if(a[i]!=b[i])return false;return true;}
static DWORD be16(const BYTE* b,DWORD p){return ((DWORD)b[p]<<8)|b[p+1];}
static DWORD be32(const BYTE* b,DWORD p){return (be16(b,p)<<16)|be16(b,p+2);}
static bool externalFamily(const BYTE* b,DWORD bytes,wchar_t* family){
    if(bytes<12)return false;DWORD signature=be32(b,0);if(signature!=0x10000&&signature!=0x4f54544f&&signature!=0x74727565)return false;
    DWORD tables=be16(b,4);if(!tables||tables>256||tables>(bytes-12)/16)return false;
    for(DWORD t=0;t<tables;++t){DWORD p=12+t*16;if(be32(b,p)!=0x6e616d65)continue;DWORD start=be32(b,p+8),size=be32(b,p+12);
        if(start>bytes||size>bytes-start||size<6)return false;DWORD count=be16(b,start+2),strings=be16(b,start+4);if(count>(size-6)/12||strings>size)return false;int best=-1;
        for(DWORD i=0;i<count;++i){p=start+6+i*12;DWORD platform=be16(b,p),language=be16(b,p+4),name=be16(b,p+6),n=be16(b,p+8),off=be16(b,p+10);
            if(name!=1||(platform!=0&&platform!=3))continue;if(!n||n%2||n/2>=LF_FACESIZE||off>size-strings||n>size-strings-off)continue;
            wchar_t value[LF_FACESIZE];bool valid=true;for(DWORD j=0;j<n/2;++j){value[j]=(wchar_t)be16(b,start+strings+off+j*2);if(!value[j]||(value[j]>=0xd800&&value[j]<=0xdfff))valid=false;}value[n/2]=0;
            int score=(platform==3?10:0)+(language==0x409?5:language==0x804?4:0);if(valid&&score>best){for(DWORD j=0;j<=n/2;++j)family[j]=value[j];best=score;}}
        return best>=0;
    }return false;
}
static BYTE* archiveFont(BYTE* archive,DWORD bytes,DWORD* fontBytes){
    if(bytes<28)return 0;for(DWORD i=0;i<bytes;++i)archive[i]^=moeDxaKey()[i%12];if(archive[0]!='D'||archive[1]!='X'||archive[2]!=4||archive[3]!=0||moeDxa32(archive+24)!=932)return 0;
    DWORD tableBytes=moeDxa32(archive+4),data=moeDxa32(archive+8),table=moeDxa32(archive+12),heads=moeDxa32(archive+16),dirs=moeDxa32(archive+20);
    if(data<28||data>table||table>bytes||tableBytes>bytes-table||dirs>tableBytes||tableBytes-dirs<16||heads>tableBytes)return 0;
    DWORD count=moeDxa32(archive+table+dirs+8),first=moeDxa32(archive+table+dirs+12);if(first>tableBytes-heads||count>10000||count>(tableBytes-heads-first)/44)return 0;
    for(DWORD kind=1;kind<=2;++kind)for(DWORD i=0;i<count;++i){const BYTE* h=archive+table+heads+first+i*44;DWORD name=moeDxa32(h);if(moeDxa32(h+4)&16)continue;if(name>tableBytes||tableBytes-name<4)return 0;
        DWORD words=(DWORD)archive[table+name]|((DWORD)archive[table+name+1]<<8);if(words>(tableBytes-name-4)/8)return 0;DWORD n=name+4+words*4;
        const char* expected=kind==1?"font.otf":"font.ttf";bool match=n+9<=tableBytes;for(DWORD j=0;match&&j<9;++j){BYTE c=archive[table+n+j];if(c>='A'&&c<='Z')c+=32;if(c!=(BYTE)expected[j])match=false;}if(!match)continue;
        DWORD offset=moeDxa32(h+32),size=moeDxa32(h+36),packed=moeDxa32(h+40),amount=packed==0xffffffffu?size:packed;
        if(size<12||size>32u*1024u*1024u||offset>table-data||amount>table-data-offset)return 0;BYTE* memory=(BYTE*)VirtualAlloc(0,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!memory)return 0;
        bool valid=true;if(packed==0xffffffffu)memcpy(memory,archive+data+offset,size);else valid=moeDxaDecode(archive+data+offset,amount,memory,size);
        wchar_t family[LF_FACESIZE];if(valid&&externalFamily(memory,size,family)){*fontBytes=size;return memory;}VirtualFree(memory,0,MEM_RELEASE);
    }return 0;
}
static void loadExternalFont(wchar_t* family){
    wchar_t* path=(wchar_t*)VirtualAlloc(0,32768*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!path)return;DWORD n=GetModuleFileNameW(0,path,32768),base=n;
    if(!n||n>=32768){VirtualFree(path,0,MEM_RELEASE);return;}while(base&&path[base-1]!=L'\\'&&path[base-1]!=L'/')--base;if(base+9>=32768){VirtualFree(path,0,MEM_RELEASE);return;}
    for(DWORD attempt=0;attempt<3;++attempt){DWORD kind=attempt==0?3:attempt;if(kind==3&&!(cfg->reserved&8))continue;const wchar_t* name=kind==3?L"font.dxa":kind==1?L"font.otf":L"font.ttf";for(DWORD j=0;j<9;++j)path[base+j]=name[j];
        HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(f==INVALID_HANDLE_VALUE)continue;LARGE_INTEGER size;bool valid=GetFileSizeEx(f,&size)&&size.QuadPart>=12&&size.QuadPart<=128*1024*1024;
        BYTE* memory=valid?(BYTE*)VirtualAlloc(0,(SIZE_T)size.QuadPart,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE):0;DWORD read=0;wchar_t selected[LF_FACESIZE];
        valid=memory&&ReadFile(f,memory,(DWORD)size.QuadPart,&read,0)&&read==(DWORD)size.QuadPart;CloseHandle(f);
        if(valid&&kind==3){DWORD fontBytes=0;BYTE* decoded=archiveFont(memory,read,&fontBytes);VirtualFree(memory,0,MEM_RELEASE);memory=decoded;read=fontBytes;valid=decoded!=0;}
        if(valid&&externalFamily(memory,read,selected)){DWORD count=0;HANDLE resource=AddFontMemResourceEx(memory,read,0,&count);if(resource&&count){DWORD old;VirtualProtect(memory,read,PAGE_READONLY,&old);for(int i=0;i<LF_FACESIZE;++i){family[i]=selected[i];if(!selected[i])break;}fontCount=count;externalFontFile=kind;break;}}
        if(memory)VirtualFree(memory,0,MEM_RELEASE);
    }VirtualFree(path,0,MEM_RELEASE);
}
static const Entry* find(const wchar_t* s,int n){DWORD h=hashText(s,n),lo=0,hi=entryCount;while(lo<hi){DWORD m=lo+(hi-lo)/2;if(entries[m].hash<h)lo=m+1;else hi=m;}while(lo<entryCount && entries[lo].hash==h){const Entry* e=entries+lo++;if(e->sourceLen==n && same(s,e->source,n))return e;}return 0;}
static const wchar_t* titleText(const wchar_t* s){if(!s)return s;const Entry* e=find(s,len(s));return e?e->target:s;}
// Translate complete resource records before the game splits descriptions into draw spans.
// Keep the native stream cursor/EOF behavior and refuse any replacement that cannot fit.
static const Entry* translateReadLine(wchar_t* text,int capacity,bool story=false){
    if(!text||capacity<2||capacity>262144||text[0]==L'#')return 0;int full=len(text,capacity);if(full>=capacity||full>=4096)return 0;int n=full;while(n>0&&(text[n-1]==L'\r'||text[n-1]==L'\n'))--n;
    const Entry* e=find(text,n);if(!e||e->targetLen+full-n>=capacity||(story&&e->targetLen&&e->target[0]==L'#'))return 0;
    for(int i=0;i<e->targetLen;++i)if(e->target[i]==L'\r'||e->target[i]==L'\n')return 0;
    wchar_t tail[3]={0,0,0};int suffix=full-n;if(suffix>2)return 0;for(int i=0;i<suffix;++i)tail[i]=text[n+i];
    for(int i=0;i<e->targetLen;++i)text[i]=e->target[i];for(int i=0;i<suffix;++i)text[e->targetLen+i]=tail[i];text[e->targetLen+suffix]=0;return e;
}
static bool encyclopediaPath(const wchar_t* path){
    const wchar_t suffix[]=L"data/dat/zukan.dat";int n=len(path),s=len(suffix);if(n<s||n>=4096||(n>s&&path[n-s-1]!=L'/'&&path[n-s-1]!=L'\\'))return false;
    for(int i=0;i<s;++i){wchar_t c=path[n-s+i];if(c==L'\\')c=L'/';if(c>=L'A'&&c<=L'Z')c+=L'a'-L'A';if(c!=suffix[i])return false;}return true;
}
static bool storyPath(const wchar_t* path){
    if(!path)return false;const wchar_t prefix[]=L"data/dat/story/";int n=len(path),s=len(prefix);if(n>=4096||n<s+5)return false;
    for(int start=0;start+s+4<n;++start){if(start&&path[start-1]!=L'/'&&path[start-1]!=L'\\')continue;int i=0;for(;i<s;++i){wchar_t c=path[start+i];if(c==L'\\')c=L'/';if(c>=L'A'&&c<=L'Z')c+=L'a'-L'A';if(c!=prefix[i])break;}if(i!=s)continue;
        for(i=start+s;i<n;++i)if(path[i]==L'/'||path[i]==L'\\')return false;
        return path[n-4]==L'.'&&(path[n-3]==L'd'||path[n-3]==L'D')&&(path[n-2]==L'a'||path[n-2]==L'A')&&(path[n-1]==L't'||path[n-1]==L'T');
    }return false;
}
// Same display-column whitelist as the MOD's resource.hpp. IDs and numeric
// gameplay fields retain their original CSV bytes and column positions.
static LONG csvKind(const wchar_t* path){
    if(!path)return 0;const wchar_t* names[]={L"data/csv/w_para.csv",L"data/csv/n_para.csv",L"data/csv/h_para.csv",L"data/csv/mj_para.csv",L"data/csv/t_para.csv",L"data/csv/s_para.csv"};int n=len(path);if(n>=4096)return 0;
    for(int kind=0;kind<6;++kind){int count=len(names[kind]);if(n<count||(n>count&&path[n-count-1]!=L'/'&&path[n-count-1]!=L'\\'))continue;int i=0;for(;i<count;++i){wchar_t c=path[n-count+i];if(c==L'\\')c=L'/';if(c>=L'A'&&c<=L'Z')c+=L'a'-L'A';if(c!=names[kind][i])break;}if(i==count)return kind+3;}return 0;
}
static bool csvDisplayColumn(LONG kind,int column){return kind==3?(column==1||column==15):kind==4?(column==1||column==5):kind==5?column==2:kind==6?column==1:kind==7?(column==0||column==19||column==20||column==21):kind==8;}
static bool translateCsvLine(wchar_t* text,int capacity,LONG kind){
    if(!text||capacity<2)return false;int full=len(text,capacity);if(full>=capacity||full>=4096)return false;int n=full;while(n&&(text[n-1]==L'\r'||text[n-1]==L'\n'))--n;
    wchar_t output[4096],field[4096];int p=0,o=0,column=0;bool changed=false;
    do{int start=p,used=0;bool quoted=p<n&&text[p]==L'"';if(quoted){++p;bool closed=false;while(p<n){wchar_t c=text[p++];if(c==L'"'){if(p<n&&text[p]==L'"'){++p;field[used++]=c;}else{closed=true;break;}}else field[used++]=c;}if(!closed||(p<n&&text[p]!=L','))return false;}else while(p<n&&text[p]!=L',')field[used++]=text[p++];field[used]=0;
        const Entry* e=csvDisplayColumn(kind,column)?find(field,used):0;bool replace=e&&e->targetLen&&e->target[0]!=L'@';if(replace)for(int i=0;i<e->targetLen;++i)if(e->target[i]==L'\r'||e->target[i]==L'\n'){replace=false;break;}
        if(replace){bool quote=quoted;for(int i=0;i<e->targetLen;++i)if(e->target[i]==L','||e->target[i]==L'"')quote=true;int size=e->targetLen+(quote?2:0);if(quote)for(int i=0;i<e->targetLen;++i)if(e->target[i]==L'"')++size;if(size>=capacity-o||size>=4096-o)return false;if(quote)output[o++]=L'"';for(int i=0;i<e->targetLen;++i){output[o++]=e->target[i];if(quote&&e->target[i]==L'"')output[o++]=L'"';}if(quote)output[o++]=L'"';changed=true;}
        else{if(p-start>=capacity-o||p-start>=4096-o)return false;while(start<p)output[o++]=text[start++];}
        ++column;if(p>=n)break;if(o+1>=capacity||o+1>=4096)return false;output[o++]=text[p++];
    }while(p<=n);
    if(!changed||o+full-n>=capacity||o+full-n>=4096)return false;for(int i=n;i<full;++i)output[o++]=text[i];output[o]=0;for(int i=0;i<=o;++i)text[i]=output[i];return true;
}
static int __cdecl openHook(const wchar_t* path,int flag,int asyncFlag){
    int handle=originalOpen(path,flag,asyncFlag);if(handle<=0)return handle;
    for(int i=0;i<64;++i)InterlockedCompareExchange(&resourceStates[i].handle,0,handle);
    LONG kind=encyclopediaPath(path)?1:storyPath(path)?2:csvKind(path);
    if(kind){DWORD slot=(DWORD)InterlockedIncrement(&resourceSlot);ResourceState* state=resourceStates+(slot%64);InterlockedExchange(&state->handle,0);state->kind=kind;state->dialogue=0;InterlockedExchange(&state->handle,handle);}return handle;
}
static int __cdecl getsHook(wchar_t* text,int capacity,int handle){
    bool probing=resourceProbeThread&&GetCurrentThreadId()==resourceProbeThread;if(probing)resourceProbeEntry=0;int result=originalGets(text,capacity,handle);if(result<0||(cfg->reserved&4))return result;
    ResourceState* state=0;for(int i=0;i<64;++i)if(resourceStates[i].handle==handle){state=resourceStates+i;break;}if(!state)return result;
    if(state->kind>=3){if(!state->dialogue++){return result;}if(translateCsvLine(text,capacity,state->kind)){InterlockedIncrement(&resourceTranslatedCount);return len(text,capacity);}return result;}
    if(state->kind==2){if(text&&text[0]==L'#'){state->dialogue=len(text)>=5&&same(text,L"#txt@",5);return result;}if(!state->dialogue)return result;}
    const Entry* e=translateReadLine(text,capacity,state->kind==2);if(!e)return result;InterlockedIncrement(&resourceTranslatedCount);if(probing)resourceProbeEntry=e;return len(text,capacity);
}
// Formatted strings are translated after the game's formatter has run.
// Literal delimiters bound captures. Numeric formats cannot consume arbitrary text.
static int tokenEnd(const wchar_t* s,int p,int n){if(p>=n || s[p]!=L'%')return -1;int q=p+1;if(q<n && s[q]==L'%')return -1;while(q<n && ((s[q]>=L'0'&&s[q]<=L'9')||s[q]==L'.'||s[q]==L'-'||s[q]==L'+'||s[q]==L' '||s[q]==L'#'||s[q]==L'h'||s[q]==L'l'||s[q]==L'z'||s[q]==L'I'))++q;if(q<n){wchar_t c=s[q];if(c==L'd'||c==L'i'||c==L'u'||c==L'x'||c==L'X'||c==L'o'||c==L'f'||c==L'F'||c==L'g'||c==L'G'||c==L'e'||c==L'E'||c==L's'||c==L'S'||c==L'c'||c==L'C')return q+1;}return -1;}
// Match complete display strings, as in Moe2Launcher/src/dictionary.hpp.
static const Entry** templates;
static DWORD templateCount;
static bool translateText(const wchar_t*,int,wchar_t*,int,unsigned,DWORD*);
static bool formatMatch(const Entry* e,const wchar_t* in,int n,wchar_t* out,int cap,unsigned depth=0){
    int begin[16],end[16],captures=0,p=0,k=0;wchar_t types[16];
    while(p<e->sourceLen){int q=tokenEnd(e->source,p,e->sourceLen);if(q<0){if(k>=n||in[k]!=e->source[p])return false;if(e->source[p]==L'%'&&p+1<e->sourceLen&&e->source[p+1]==L'%')++p;++p;++k;continue;}
        if(captures==16)return false;int a=k,stop=k;wchar_t type=e->source[q-1];
        if(type==L's'||type==L'S'){int next=q;while(next<e->sourceLen&&tokenEnd(e->source,next,e->sourceLen)<0)++next;int literal=next-q;if(!literal)stop=n;else {for(stop=k;stop+literal<=n;++stop)if(same(in+stop,e->source+q,literal))break;if(stop+literal>n)return false;}}
        else if(type==L'c'||type==L'C'){stop=k+1;if(stop>n)return false;}
        else {
            while(stop<n&&in[stop]==L' ')++stop;
            if(stop<n&&(in[stop]==L'-'||in[stop]==L'+'))++stop;
            int digits=stop;bool floating=type==L'f'||type==L'F'||type==L'g'||type==L'G'||type==L'e'||type==L'E';
            while(stop<n){wchar_t c=in[stop];if((c>=L'0'&&c<=L'9')||(floating&&c==L'.')||((type==L'x'||type==L'X')&&((c>=L'a'&&c<=L'f')||(c>=L'A'&&c<=L'F'))))++stop;else break;}
            if(stop==digits)return false;
            if(floating&&stop<n&&(in[stop]==L'e'||in[stop]==L'E')){++stop;if(stop<n&&(in[stop]==L'+'||in[stop]==L'-'))++stop;int exponent=stop;while(stop<n&&in[stop]>=L'0'&&in[stop]<=L'9')++stop;if(stop==exponent)return false;}
        }
        begin[captures]=a;end[captures]=stop;types[captures++]=type;k=stop;p=q;
    }
    if(k!=n||!captures)return false;int o=0,c=0;wchar_t argument[4096];
    for(p=0;p<e->targetLen;){int q=tokenEnd(e->target,p,e->targetLen);if(q>=0){if(c>=captures)return false;const wchar_t* value=in+begin[c];int size=end[c]-begin[c];
            if((types[c]==L's'||types[c]==L'S')&&size<n&&translateText(value,size,argument,4096,depth+1,0)){value=argument;size=len(argument);}
            if(size>=cap-o)return false;for(int j=0;j<size;++j)out[o++]=value[j];++c;p=q;
        }else {if(o>=cap-1)return false;out[o++]=e->target[p];if(e->target[p]==L'%'&&p+1<e->targetLen&&e->target[p+1]==L'%')++p;++p;}}
    if(c!=captures)return false;out[o]=0;return true;
}
static bool translateText(const wchar_t* s,int n,wchar_t* out,int cap,unsigned depth,DWORD* budget){
    if(!s||n<=0||n>=4096||cap<2||depth>8||s[0]==L'@'||s[0]==L'#')return false;
    const Entry* e=find(s,n);if(e){if(e->targetLen>=cap)return false;for(int i=0;i<=e->targetLen;++i)out[i]=e->target[i];if(budget)*budget=e->budget;return true;}
    wchar_t normalized[4096];bool half=false;for(int i=0;i<n;++i)if(s[i]>=0xff61&&s[i]<=0xff9f){half=true;break;}
    if(half){int used=0;for(int i=0;i<n;){if(s[i]<0xff61||s[i]>0xff9f){normalized[used++]=s[i++];continue;}int begin=i;while(i<n&&s[i]>=0xff61&&s[i]<=0xff9f)++i;int copied=LCMapStringEx(LOCALE_NAME_INVARIANT,LCMAP_FULLWIDTH,s+begin,i-begin,normalized+used,4095-used,0,0,0);if(!copied)return false;used+=copied;}normalized[used]=0;e=find(normalized,used);if(e&&e->targetLen<cap){for(int i=0;i<=e->targetLen;++i)out[i]=e->target[i];if(budget)*budget=e->budget;return true;}}
    int begin=0,end=n;while(begin<end&&(s[begin]==L' '||s[begin]==L'　'))++begin;while(end>begin&&(s[end-1]==L' '||s[end-1]==L'　'))--end;
    if(begin<end&&(begin||end<n)&&translateText(s+begin,end-begin,normalized,4096,depth+1,budget)){int size=len(normalized);if(begin+size+n-end>=cap)return false;int o=0;for(int i=0;i<begin;++i)out[o++]=s[i];for(int i=0;i<size;++i)out[o++]=normalized[i];for(int i=end;i<n;++i)out[o++]=s[i];out[o]=0;return true;}
    bool lines=false;for(int i=0;i<n;++i)if(s[i]==L'\n'){lines=true;break;}
    for(DWORD t=0;t<templateCount;++t){if(lines){bool multiline=false;for(int i=0;i<templates[t]->sourceLen;++i)if(templates[t]->source[i]==L'\n'){multiline=true;break;}if(!multiline)continue;}if(formatMatch(templates[t],s,n,out,cap,depth)){if(budget)*budget=templates[t]->budget;return true;}}
    if(n>2&&s[0]==L'['&&s[n-1]==L']'&&translateText(s+1,n-2,normalized,4096,depth+1,budget)){int size=len(normalized);if(size+2>=cap)return false;out[0]=L'[';for(int i=0;i<size;++i)out[i+1]=normalized[i];out[size+1]=L']';out[size+2]=0;return true;}
    if(n>1&&(s[0]==L'▲'||s[0]==L'○'||s[0]==L'〇')){int p=1;while(p<n&&(s[p]==L' '||s[p]==L'　'))++p;if(translateText(s+p,n-p,normalized,4096,depth+1,budget)){int size=len(normalized);if(p+size>=cap)return false;for(int i=0;i<p;++i)out[i]=s[i];for(int i=0;i<=size;++i)out[p+i]=normalized[i];return true;}}
    if(n>2&&(s[0]==L'*'||s[0]==L'＊')&&s[n-1]==s[0]&&translateText(s+1,n-2,normalized,4096,depth+1,budget)){int size=len(normalized);if(size+2>=cap)return false;out[0]=s[0];for(int i=0;i<size;++i)out[i+1]=normalized[i];out[size+1]=s[n-1];out[size+2]=0;return true;}
    int o=0;bool changed=false;if(!lines)return false;
    for(int p=0;p<n;){int stop=p;while(stop<n&&s[stop]!=L'\n')++stop;int tail=stop;bool cr=tail>p&&s[tail-1]==L'\r';if(cr)--tail;bool translated=translateText(s+p,tail-p,normalized,4096,depth+1,budget);int size=translated?len(normalized):tail-p;if(size+(cr?1:0)+(stop<n?1:0)>=cap-o)return false;for(int i=0;i<size;++i)out[o++]=translated?normalized[i]:s[p+i];if(cr)out[o++]=L'\r';if(stop<n)out[o++]=L'\n';changed=changed||translated;p=stop+1;}out[o]=0;return changed;
}
static int __cdecl drawHook(int draw,int x,int y,int extend,double ex,double ey,const wchar_t* text,unsigned color,void* image,const RECT* clip,int transparent,int font,unsigned edge,int length,int vertical,SIZE* size){
    wchar_t formatted[4096];const wchar_t* input=text;int full=len(text),n=(length>=0&&length<full)?length:full;const Entry* e=0;bool changed=false;DWORD budget=0;
    if(text && n && full<4096){e=find(text,n);if(!e && full!=n)e=find(text,full);if(e){text=e->target;length=(full!=n && e->sourceLen==full)?(n*e->targetLen+full-1)/full:e->targetLen;changed=true;budget=e->budget;}
        else if(translateText(text,n,formatted,4096,0,&budget)){text=formatted;length=len(text);changed=true;}}
    if(draw)InterlockedIncrement(&drawCount);else InterlockedIncrement(&widthCount);if(changed)InterlockedIncrement(&translatedCount);
    if(changed && !vertical){DWORD rowScale=(cfg->reserved&16)?budget>>16:100;if(cfg->reserved&16)budget&=65535;double scale=(cfg->scale/100.0)*(rowScale/100.0);ex*=scale;ey*=scale;if(scale!=1.0)extend=1;
        DWORD available=budget?budget:cfg->maxWidth;
        if(available){SIZE measured={0,0};int pixels=originalDraw(0,0,0,extend,ex,ey,text,0,0,0,1,font,0,length,0,&measured);if(measured.cx>0)pixels=measured.cx;
            if(pixels>(int)available){double fit=(double)available/pixels;double minimum=cfg->minScale/100.0;if(fit<minimum)fit=minimum;ex*=fit;extend=1;}}
    }
    return originalDraw(draw,x,y,extend,ex,ey,text,color,image,clip,transparent,font,edge,length,vertical,size);
}
static HFONT WINAPI fontHook(int h,int w,int a,int o,int weight,DWORD italic,DWORD underline,DWORD strike,DWORD charset,DWORD out,DWORD clip,DWORD quality,DWORD pitch,LPCWSTR face){
    InterlockedIncrement(&createdFontCount);int nh=h+cfg->height,nw=w+cfg->width;if(!nh)nh=h;
    return originalFont(nh,nw,a,o,weight,italic,underline,strike,fontFace&&*fontFace?cfg->charset:charset,out,clip,quality,pitch,fontFace&&*fontFace?fontFace:face);
}
static BOOL WINAPI titleHook(HWND w,LPCWSTR s){return originalTitle(w,titleText(s));}
static int WINAPI messageHook(HWND w,LPCWSTR s,LPCWSTR t,UINT f){return originalMessage(w,titleText(s),titleText(t),f);}
static HWND WINAPI windowHook(DWORD e,LPCWSTR c,LPCWSTR t,DWORD s,int x,int y,int w,int h,HWND p,HMENU m,HINSTANCE i,LPVOID v){return originalWindow(e,c,titleText(t),s,x,y,w,h,p,m,i,v);}
static bool cstrEqual(const char* a,const char* b){while(*a && *a==*b){++a;++b;}return *a==*b;}
static bool magicEqual(const void* bytes,const char* magic){const char* p=(const char*)bytes;for(int i=0;i<8;++i)if(p[i]!=magic[i])return false;return true;}
static void* patchImport(const char* name,void* fn){auto nt=(IMAGE_NT_HEADERS32*)(game+((IMAGE_DOS_HEADER*)game)->e_lfanew);auto dir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];auto d=(IMAGE_IMPORT_DESCRIPTOR*)(game+dir.VirtualAddress);
    for(;d->Name;++d){if(!d->OriginalFirstThunk)continue;auto names=(DWORD*)(game+d->OriginalFirstThunk);auto slots=(DWORD*)(game+d->FirstThunk);for(DWORD n=0;names[n];++n){if(names[n]&0x80000000)continue;const char* s=(char*)(game+names[n]+2);if(cstrEqual(s,name)){DWORD old;if(!VirtualProtect(slots+n,4,PAGE_READWRITE,&old))return 0;void* previous=(void*)slots[n];slots[n]=(DWORD)fn;DWORD ignored;VirtualProtect(slots+n,4,old,&ignored);return previous;}}}return 0;}
static void putJump(BYTE* a,void* b){a[0]=0xe9;*(DWORD*)(a+1)=(DWORD)((BYTE*)b-a-5);}
static bool detour(){BYTE* target=game+cfg->textRva;const BYTE expected[6]={0x55,0x8b,0xec,0x83,0xe4,0xc0};for(int i=0;i<6;++i)if(target[i]!=expected[i])return false;BYTE* trampoline=(BYTE*)VirtualAlloc(0,11,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(!trampoline)return false;memcpy(trampoline,target,6);putJump(trampoline+6,target+6);DWORD old;if(!VirtualProtect(trampoline,11,PAGE_EXECUTE_READ,&old))return false;originalDraw=(Backend)trampoline;
    if(!VirtualProtect(target,6,PAGE_EXECUTE_READWRITE,&old))return false;putJump(target,(void*)drawHook);target[5]=0x90;DWORD ignored;VirtualProtect(target,6,old,&ignored);FlushInstructionCache(GetCurrentProcess(),target,6);return true;}
static bool detourGets(){
    BYTE* target=game+0x39ee0; // Reviewed original JP 1.10 SHA256 and FileRead_gets ABI.
    if(target[0]!=0x83||target[1]!=0xec||target[2]!=8||target[3]!=0x83||target[4]!=0x3d||target[9]!=0||target[10]!=0x56||target[11]!=0x74)return false;
    readerInitialized=(int*)(*(DWORD*)(target+5));BYTE* trampoline=(BYTE*)VirtualAlloc(0,15,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(!trampoline)return false;
    memcpy(trampoline,target,10);putJump(trampoline+10,target+10);DWORD old;if(!VirtualProtect(trampoline,15,PAGE_EXECUTE_READ,&old))return false;originalGets=(ResourceGets)trampoline;
    if(!VirtualProtect(target,10,PAGE_EXECUTE_READWRITE,&old))return false;putJump(target,(void*)getsHook);for(int i=5;i<10;++i)target[i]=0x90;DWORD ignored;VirtualProtect(target,10,old,&ignored);FlushInstructionCache(GetCurrentProcess(),target,10);return true;
}
static bool detourOpen(){
    BYTE* target=game+0x3af60;const BYTE expected[6]={0x81,0xec,0x04,0x08,0,0};for(int i=0;i<6;++i)if(target[i]!=expected[i])return false;
    BYTE* trampoline=(BYTE*)VirtualAlloc(0,11,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(!trampoline)return false;memcpy(trampoline,target,6);putJump(trampoline+6,target+6);DWORD old;
    if(!VirtualProtect(trampoline,11,PAGE_EXECUTE_READ,&old))return false;originalOpen=(ResourceOpen)trampoline;if(!VirtualProtect(target,6,PAGE_EXECUTE_READWRITE,&old))return false;
    putJump(target,(void*)openHook);target[5]=0x90;DWORD ignored;VirtualProtect(target,6,old,&ignored);FlushInstructionCache(GetCurrentProcess(),target,6);return true;
}
static bool hasArgument(const wchar_t* key){const wchar_t* s=GetCommandLineW();for(;*s;++s){int n=0;while(key[n] && s[n]==key[n])++n;if(!key[n] && (s[n]==0||s[n]==L' '||s[n]==L'\"'))return true;}return false;}
static void writeReport(const wchar_t* path,const char* s){HANDLE f=CreateFileW(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(f!=INVALID_HANDLE_VALUE){DWORD wrote;DWORD n=0;while(s[n])++n;WriteFile(f,s,n,&wrote,0);CloseHandle(f);}}
static char* number(char* p,DWORD n){char digits[16];unsigned used=0;do{digits[used++]=(char)('0'+n%10);n/=10;}while(n);while(used)*p++=digits[--used];return p;}
static char* literal(char* p,const char* s){while(*s)*p++=*s++;return p;}
static DWORD WINAPI csvTest(void*){
    for(int i=0;i<100&&!*readerInitialized;++i)Sleep(100);const wchar_t* names[]={L"w_para",L"n_para",L"h_para",L"mj_para",L"t_para",L"s_para"};DWORD rows=0,changed=0;bool ok=*readerInitialized!=0;
    for(int file=0;file<6&&ok;++file){wchar_t path[128]=L"data/csv/",rawPath[128]=L"cn-",translatedPath[128]=L"cn-";int n=len(names[file]);for(int i=0;i<n;++i){path[9+i]=i<(file==3?2:1)?names[file][i]-(L'a'-L'A'):names[file][i];rawPath[3+i]=names[file][i];translatedPath[3+i]=names[file][i];}const wchar_t* tail=L".csv";for(int i=0;i<5;++i)path[9+n+i]=tail[i];tail=L"-original.csv";for(int i=0;i<=len(tail);++i)rawPath[3+n+i]=tail[i];tail=L"-translated.csv";for(int i=0;i<=len(tail);++i)translatedPath[3+n+i]=tail[i];
        int translated=-1;for(int attempt=0;attempt<100;++attempt){translated=openHook(path,0,0);if(translated>0)break;Sleep(100);}int raw=originalOpen(path,0,0);HANDLE rawFile=CreateFileW(rawPath,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0),translatedFile=CreateFileW(translatedPath,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(translated<=0||raw<=0||rawFile==INVALID_HANDLE_VALUE||translatedFile==INVALID_HANDLE_VALUE){ok=false;if(rawFile!=INVALID_HANDLE_VALUE)CloseHandle(rawFile);if(translatedFile!=INVALID_HANDLE_VALUE)CloseHandle(translatedFile);break;}
        WORD bom=0xfeff;DWORD wrote;WriteFile(rawFile,&bom,2,&wrote,0);WriteFile(translatedFile,&bom,2,&wrote,0);wchar_t a[1024],b[1024];
        for(int row=0;row<4096;++row){int x=originalGets(a,1024,raw),y=getsHook(b,1024,translated);if(x<0||y<0){ok=ok&&(x<0&&y<0)&&row>1;break;}++rows;if(x!=y||!same(a,b,x))++changed;ok=ok&&WriteFile(rawFile,a,len(a)*2,&wrote,0)&&WriteFile(translatedFile,b,len(b)*2,&wrote,0);const wchar_t newline[]=L"\r\n";WriteFile(rawFile,newline,4,&wrote,0);WriteFile(translatedFile,newline,4,&wrote,0);if(row==4095)ok=false;}
        CloseHandle(rawFile);CloseHandle(translatedFile);
    }
    if(cfg->reserved&4){const wchar_t* paths[]={L"data/dat/zukan.dat",L"data/dat/story/story0.dat"};for(int file=0;file<2;++file){const wchar_t* path=paths[file];int translated=openHook(path,0,0),raw=originalOpen(path,0,0);wchar_t a[4096],b[4096];DWORD checked=0;if(translated<=0||raw<=0){ok=false;break;}for(;checked<4096;++checked){int x=originalGets(a,4096,raw),y=getsHook(b,4096,translated);if(x<0||y<0){ok=ok&&x<0&&y<0;break;}if(x!=y||!same(a,b,len(a)))ok=false;}ok=ok&&checked>30&&checked<4096;}}
    ok=ok&&rows>1500&&((cfg->reserved&4)?changed==0:changed>500);char report[256];char* p=literal(report,"{\"csv_rows\":");p=number(p,rows);p=literal(p,",\"translated_rows\":");p=number(p,changed);p=literal(p,",\"passed\":");p=literal(p,ok?"true":"false");p=literal(p,"}\n");*p=0;writeReport(L"cn-csv.json",report);ExitProcess(ok?0:74);return 0;
}
static bool progressiveStoryTest(const wchar_t* text,int n);
static DWORD WINAPI descriptionTest(void*){
    for(int i=0;i<100&&!*readerInitialized;++i)Sleep(100);resourceProbeThread=GetCurrentThreadId();
    using Open=int(__cdecl*)(const wchar_t*,int,int);Open open=(Open)(game+0x3af60);ResourceGets gets=(ResourceGets)(game+0x39ee0);
    int handle=-1;for(int attempt=0;attempt<100;++attempt){if(*readerInitialized)handle=open(L"data/dat/zukan.dat",0,0);if(handle>0)break;Sleep(100);}DWORD records=0,translated=0,longRecords=0,missing=0,invalid=0;bool chimera=false;const Entry* chimeraEntry=0;wchar_t buffer[4096];
    if(handle>0)for(;records<1024;++records){int n=gets(buffer,4096,handle);if(n<0)break;const Entry* e=resourceProbeEntry;if(e){++translated;if(n!=e->targetLen||!same(buffer,e->target,n))++invalid;if(e->sourceLen>30)++longRecords;if(e->sourceLen>=7&&same(e->source,L"魔獣テュポーン",7)){chimera=true;chimeraEntry=e;}}else if(n>30&&buffer[0]!=L'#')++missing;}
    bool otherUnchanged=false;if(chimeraEntry){HANDLE f=CreateFileW(L"cn-untranslated-reader.txt",GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(f!=INVALID_HANDLE_VALUE){WORD bom=0xfeff;DWORD wrote;WriteFile(f,&bom,2,&wrote,0);WriteFile(f,chimeraEntry->source,chimeraEntry->sourceLen*2,&wrote,0);CloseHandle(f);int other=open(L"cn-untranslated-reader.txt",0,0);int n=other>0?gets(buffer,4096,other):-1;otherUnchanged=n==chimeraEntry->sourceLen&&same(buffer,chimeraEntry->source,n)&&!resourceProbeEntry;}}

    // Exercise real story control commands through the hooked native reader.
    const wchar_t* commands[]={L"#cld@イルセ@ブラフマー@リーア@ペナルゥ@ブラリボン",L"#cst@ブラリボン@[600]",L"#cex@ブラリボン哀",L"#cin@ブラリボン",L"#cex@ブラリボン素",L"#cex@ブラリボン怒",L"#cou@ブラリボン"};
    DWORD storyMask=0,storyChecks=0;bool storyUntouched=true;int story=open(L"data/dat/story/story49.dat",0,0);
    if(story>0)for(int i=0;i<2048;++i){int n=gets(buffer,4096,story);if(n<0)break;if(resourceProbeEntry&&buffer[0]==L'#')storyUntouched=false;for(int c=0;c<7;++c)if(n==len(commands[c])&&same(buffer,commands[c],n)){storyMask|=1u<<c;++storyChecks;}}
    bool storyUnchanged=story>0&&storyUntouched&&storyMask==127;
    int intro=open(L"data/dat/story/story0.dat",0,0);DWORD introTranslated=0,revealLines=0;bool revealOk=true;
    if(intro>0)for(int i=0;i<2048;++i){int n=gets(buffer,4096,intro);if(n<0)break;const Entry* e=resourceProbeEntry;if(!e)continue;++introTranslated;const wchar_t* expected=0;
        if(e->sourceLen==len(L"「言ったわね～。")&&same(e->source,L"「言ったわね～。",e->sourceLen))expected=L"「这可是你说的哦～";
        if(e->sourceLen==len(L"　今日は一段ときびしくするわよ？」")&&same(e->source,L"　今日は一段ときびしくするわよ？」",e->sourceLen))expected=L"　今天我可要格外严格了？」";
        if(expected){++revealLines;revealOk=revealOk&&n==len(expected)&&same(buffer,expected,n)&&progressiveStoryTest(buffer,n);}}
    bool beforeReveal=intro>0&&introTranslated>10&&revealLines==2&&revealOk;
    bool ok=handle>0&&records>=344&&chimera&&longRecords>100&&!missing&&!invalid&&otherUnchanged&&storyUnchanged&&beforeReveal;

    char report[1024];char* p=report;p=literal(p,"{\"native_archive_reader\":");p=literal(p,handle>0?"true":"false");p=literal(p,",\"records\":");p=number(p,records);p=literal(p,",\"translated_records\":");p=number(p,translated);p=literal(p,",\"translated_long_descriptions\":");p=number(p,longRecords);p=literal(p,",\"unmatched_long_descriptions\":");p=number(p,missing);p=literal(p,",\"invalid_translations\":");p=number(p,invalid);p=literal(p,",\"chimera_translated\":");p=literal(p,chimera?"true":"false");p=literal(p,",\"other_resources_unchanged\":");p=literal(p,otherUnchanged?"true":"false");p=literal(p,",\"story_identifiers_unchanged\":");p=literal(p,storyUnchanged?"true":"false");p=literal(p,",\"story_identifier_lines_checked\":");p=number(p,storyChecks);p=literal(p,",\"story_dialogue_before_typewriter\":");p=literal(p,beforeReveal?"true":"false");p=literal(p,",\"story0_translated_lines\":");p=number(p,introTranslated);p=literal(p,",\"screenshot_lines_reveal_checked\":");p=number(p,revealLines);p=literal(p,",\"passed\":");p=literal(p,ok?"true":"false");p=literal(p,"}\n");*p=0;writeReport(L"cn-descriptions.json",report);ExitProcess(ok?0:73);return 0;
}
static DWORD WINAPI liveTest(void*){Sleep(15000);char report[1024];char* p=report;p=literal(p,"{\"draw_calls\":");p=number(p,drawCount);p=literal(p,",\"width_calls\":");p=number(p,widthCount);p=literal(p,",\"translated_calls\":");p=number(p,translatedCount);p=literal(p,",\"font_calls\":");p=number(p,createdFontCount);p=literal(p,",\"embedded_font_faces\":");p=number(p,fontCount);p=literal(p,",\"font_family\":\"");int n=WideCharToMultiByte(CP_UTF8,0,fontFace,-1,p,256,0,0);if(n>0)p+=n-1;p=literal(p,"\",\"scope\":\"real idle game rendering; no battle or full-story acceptance\"}\n");*p=0;writeReport(L"cn-live-test.json",report);ExitProcess(drawCount>0&&widthCount>0&&translatedCount>0&&createdFontCount>0?0:72);return 0;}
static const wchar_t* testRendered;
static int testLength,testDraw;static double testEy;
static wchar_t testRenderedCopy[4096];
static int __cdecl mockBackend(int draw,int,int,int,double,double ey,const wchar_t* text,unsigned,void*,const RECT*,int,int,unsigned,int length,int,SIZE* size){testEy=ey;testRendered=text;testLength=length;testDraw=draw;int used=length>=0?length:len(text);if(used>4095)used=4095;for(int i=0;i<used;++i)testRenderedCopy[i]=text[i];testRenderedCopy[used]=0;if(size){size->cx=length*20;size->cy=24;}return length*20;}
static bool progressiveStoryTest(const wchar_t* text,int n){
    Backend native=originalDraw;originalDraw=mockBackend;bool ok=true;wchar_t prefix[4096];
    for(int cut=0;cut<=n&&ok;++cut){for(int i=0;i<cut;++i)prefix[i]=text[i];prefix[cut]=0;
        drawHook(1,0,0,0,1.0,1.0,prefix,0,0,0,1,0,0,cut,0,0);ok=testLength==cut&&same(testRendered,text,cut);
        drawHook(1,0,0,0,1.0,1.0,text,0,0,0,1,0,0,cut,0,0);ok=ok&&testLength==cut&&same(testRendered,text,cut);}
    originalDraw=native;return ok;
}
static bool runtimeTest(){
    Entry format={L"ステージ%02d",L"第%02d关",0,0,0,0};format.sourceLen=len(format.source);format.targetLen=len(format.target);wchar_t result[64];if(!formatMatch(&format,L"ステージ12",len(L"ステージ12"),result,64)||len(result)!=4||!same(result,L"第12关",4))return false;
    if(!entryCount)return true;const Entry* e=entries;Backend native=originalDraw;originalDraw=mockBackend;
    wchar_t line[4096];for(int i=0;i<e->sourceLen;++i)line[i]=e->source[i];line[e->sourceLen]=0;if(!translateReadLine(line,4096)||len(line)!=e->targetLen||!same(line,e->target,e->targetLen)){originalDraw=native;return false;}
    drawHook(1,0,0,0,1.0,1.0,e->source,0,0,0,1,0,0,e->sourceLen,0,0);double expectedScale=(cfg->scale/100.0)*((cfg->reserved&16)?(e->budget>>16)/100.0:1.0);bool ok=testDraw==1&&testLength==e->targetLen&&testRendered==e->target&&testEy==expectedScale;
    drawHook(0,0,0,0,1.0,1.0,e->source,0,0,0,1,0,0,e->sourceLen,0,0);ok=ok&&testDraw==0&&testLength==e->targetLen&&testRendered==e->target;
    originalDraw=native;drawCount=widthCount=translatedCount=0;return ok;
}
static bool tooltipTest(){
    const wchar_t* input[]={L"[氷の矢] 氷属性 魔法",L"攻撃値60  命中率100  特攻率5  射程2  範囲1  コスト2",L"攻撃値73 命中率92 特攻率17 射程3 範囲6 コスト4",L"　ﾌﾞﾗﾌﾏｰ　",L"[氷の矢] 氷属性 魔法\r\n攻撃値60  命中率100  特攻率5  射程2  範囲1  コスト2",L"修練値を20消費して防御力の基礎値+1",L"修練値を257消費してSPの基礎値+1(基礎値5でSP1アップ)",L"[修練値を20消費して防御力の基礎値+1]",L"攻撃80",L"修練値8",L"召コ3"};
    const wchar_t* expected[]={L"[冰之箭] 冰属性（魔法）",L"攻击值60  命中率100  暴击率5  射程2  范围1  消耗2",L"攻击值73 命中率92 暴击率17 射程3 范围6 消耗4",L"　布拉玛　",L"[冰之箭] 冰属性（魔法）\r\n攻击值60  命中率100  暴击率5  射程2  范围1  消耗2",L"消耗20修炼值，使防御力基础值+1",L"消耗257修炼值，使SP基础值+1（基础值每增加5，SP增加1）",L"[消耗20修炼值，使防御力基础值+1]",L"攻击80",L"修炼值8",L"召唤耗SP3"};
    Backend native=originalDraw;originalDraw=mockBackend;bool ok=true;
    for(int i=0;i<sizeof(input)/sizeof(input[0])&&ok;++i)for(int draw=0;draw<=1&&ok;++draw){drawHook(draw,0,0,0,1.0,1.0,input[i],0,0,0,1,0,0,len(input[i]),0,0);ok=testLength==len(expected[i])&&same(testRenderedCopy,expected[i],testLength);if(!ok){char report[16384];char* p=literal(report,"tooltip case ");p=number(p,i);p=literal(p,"\nactual:\n");int copied=WideCharToMultiByte(CP_UTF8,0,testRenderedCopy,-1,p,12000,0,0);if(copied>0)p+=copied-1;p=literal(p,"\nexpected:\n");copied=WideCharToMultiByte(CP_UTF8,0,expected[i],-1,p,2000,0,0);if(copied>0)p+=copied-1;*p=0;writeReport(L"cn-tooltip-failure.txt",report);}}
    wchar_t untouched[128];ok=ok&&!translateText(L"文中の氷の矢は未訳",len(L"文中の氷の矢は未訳"),untouched,128,0,0)&&!translateText(L"@ブラリボン",len(L"@ブラリボン"),untouched,128,0,0);
    originalDraw=native;return ok;
}
static void fail(){MessageBoxW(0,L"汉化初始化失败。请使用工具从未修改的 1.10 EXE 重新打包。",L"MoeKuri 1.10 汉化",MB_ICONERROR);ExitProcess(70);}
extern "C" __declspec(dllexport) void __cdecl CNInit(const CNHeader* header){
    const CNHeader* stored=header;
    game=(BYTE*)GetModuleHandleW(0);if(!header)fail();auto image=(IMAGE_NT_HEADERS32*)(game+((IMAGE_DOS_HEADER*)game)->e_lfanew);auto sections=IMAGE_FIRST_SECTION(image);SIZE_T available=0;
    for(unsigned i=0;i<image->FileHeader.NumberOfSections;++i){const BYTE* start=game+sections[i].VirtualAddress;if((const BYTE*)header>=start&&(SIZE_T)((const BYTE*)header-start)<sections[i].Misc.VirtualSize)available=sections[i].Misc.VirtualSize-((const BYTE*)header-start);}
    if(available<sizeof(CNCompressedHeader))fail();
    if(magicEqual(header,"MOECNZ11")){
        const CNCompressedHeader* c=(const CNCompressedHeader*)header;if(c->version!=2||c->total>available||c->total<sizeof(*c)||c->packedBytes!=c->total-sizeof(*c)||c->rawBytes<sizeof(CNHeader)||c->rawBytes>256u*1024u*1024u||c->algorithm!=COMPRESS_ALGORITHM_XPRESS_HUFF)fail();
        BYTE* raw=(BYTE*)VirtualAlloc(0,c->rawBytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!raw)fail();DECOMPRESSOR_HANDLE decompressor=0;if(!CreateDecompressor(c->algorithm,0,&decompressor))fail();SIZE_T count=0;BOOL ok=Decompress(decompressor,(const BYTE*)c+sizeof(*c),c->packedBytes,raw,c->rawBytes,&count);CloseDecompressor(decompressor);
        if(!ok||count!=c->rawBytes||payloadChecksum(raw,count)!=c->checksum)fail();DWORD old;if(!VirtualProtect(raw,count,PAGE_READONLY,&old))fail();header=(const CNHeader*)raw;available=count;
    }
    cfg=header;if(!magicEqual(cfg,"MOECN110")||cfg->version!=1||cfg->count>100000||cfg->total<sizeof(CNHeader)||cfg->total>available||cfg->records>cfg->total||cfg->count>(cfg->total-cfg->records)/sizeof(CNRecord)||cfg->font>cfg->total||cfg->fontBytes>cfg->total-cfg->font||cfg->face>=cfg->total)fail();
    entryCount=cfg->count;const BYTE* payload=(const BYTE*)cfg;auto records=(const CNRecord*)(payload+cfg->records);
    SIZE_T need=(entryCount+1)*sizeof(Entry)+(entryCount+1)*sizeof(Entry*)+256;for(DWORD i=0;i<entryCount;++i){int a=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(char*)payload+records[i].source,-1,0,0),b=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(char*)payload+records[i].target,-1,0,0);if(a<=0||b<=0||a>4096||b>4096)fail();need+=(a+b)*2;}
    BYTE* decoded=(BYTE*)VirtualAlloc(0,need,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(!decoded)fail();entries=(Entry*)decoded;templates=(const Entry**)(entries+entryCount+1);wchar_t* w=(wchar_t*)(templates+entryCount+1);
    for(DWORD i=0;i<entryCount;++i){Entry* e=entries+i;e->source=w;int a=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(char*)payload+records[i].source,-1,w,4096);e->sourceLen=a-1;w+=a;e->target=w;int b=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(char*)payload+records[i].target,-1,w,4096);e->targetLen=b-1;w+=b;e->hash=records[i].hash;e->budget=records[i].budget;for(int j=0;j<e->sourceLen;++j)if(tokenEnd(e->source,j,e->sourceLen)>=0){templates[templateCount++]=e;break;}}
    fontFace=w;if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(char*)payload+cfg->face,-1,w,64))fail();if(cfg->reserved&2)loadExternalFont(w);DWORD old;if(!VirtualProtect(decoded,need,PAGE_READONLY,&old))fail();
    if(cfg->fontBytes && !AddFontMemResourceEx((void*)(payload+cfg->font),cfg->fontBytes,0,&fontCount))fail();
    originalFont=(decltype(originalFont))patchImport("CreateFontW",(void*)fontHook);originalTitle=(decltype(originalTitle))patchImport("SetWindowTextW",(void*)titleHook);originalMessage=(decltype(originalMessage))patchImport("MessageBoxW",(void*)messageHook);originalWindow=(decltype(originalWindow))patchImport("CreateWindowExW",(void*)windowHook);
    if(!originalFont || !detour() || !detourGets() || !detourOpen())fail();
    if(hasArgument(L"--cn-font-probe")){char report[256];char* p=literal(report,"{\"external_requested\":");p=literal(p,(cfg->reserved&2)?"true":"false");p=literal(p,",\"external_file\":");p=number(p,externalFontFile);p=literal(p,",\"font_faces\":");p=number(p,fontCount);p=literal(p,",\"embedded_bytes\":");p=number(p,cfg->fontBytes);p=literal(p,"}\n");*p=0;writeReport(L"cn-font-probe.json",report);ExitProcess(0);}
    // The extra IAT lives outside the original PE's contiguous IAT directory.
    // Its containing section is loader-writable, then made immutable here.
    auto nt=(IMAGE_NT_HEADERS32*)(game+((IMAGE_DOS_HEADER*)game)->e_lfanew);
    auto section=IMAGE_FIRST_SECTION(nt);
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)if(section[i].Name[0]=='.'&&section[i].Name[1]=='c'&&section[i].Name[2]=='n'&&section[i].Name[3]=='c'&&section[i].Name[4]=='o'&&section[i].Name[5]=='n'){
        DWORD before;if(!VirtualProtect(game+section[i].VirtualAddress,section[i].Misc.VirtualSize,PAGE_READONLY,&before))fail();
    }
    if(hasArgument(L"--cn-probe")){
        const wchar_t* test=L"設定";const Entry* e=find(test,len(test));MEMORY_BASIC_INFORMATION info;VirtualQuery(cfg,&info,sizeof(info));MEMORY_BASIC_INFORMATION decodedInfo;VirtualQuery(decoded,&decodedInfo,sizeof(decodedInfo));
        MEMORY_BASIC_INFORMATION storedInfo;VirtualQuery(stored,&storedInfo,sizeof(storedInfo));bool ro=(info.Protect==PAGE_READONLY)&&(decodedInfo.Protect==PAGE_READONLY)&&(storedInfo.Protect==PAGE_READONLY);bool valid=ro&&runtimeTest()&&((cfg->reserved&4)||tooltipTest());
        // Probe the native width/draw ABI only after DxLib initializes in a real session.
        const char ok[]= "{\"initialized\":true,\"payload_readonly\":true,\"decoded_readonly\":true,\"font_hook\":true,\"text_hook\":true,\"resource_line_hook\":true,\"translation_before_wrapping\":true,\"single_exe\":true,\"draw_and_width_translation\":true,\"formatted_translation\":true,\"skill_tooltip_translation\":true}\n";
        writeReport(L"cn-probe.json",valid?ok:"{\"probe_failed\":true}\n");ExitProcess(valid?0:71);
    }
    if(hasArgument(L"--cn-live-test")){HANDLE thread=CreateThread(0,0,liveTest,0,0,0);if(!thread)fail();CloseHandle(thread);}
    if(hasArgument(L"--cn-description-test")){HANDLE thread=CreateThread(0,0,descriptionTest,0,0,0);if(!thread)fail();CloseHandle(thread);}
    if(hasArgument(L"--cn-csv-test")){HANDLE thread=CreateThread(0,0,csvTest,0,0,0);if(!thread)fail();CloseHandle(thread);}
}
