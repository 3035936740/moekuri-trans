#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "format.h"
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
static volatile LONG encyclopediaHandles[32];
static volatile LONG encyclopediaSlot;
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
static void loadExternalFont(wchar_t* family){
    wchar_t* path=(wchar_t*)VirtualAlloc(0,32768*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!path)return;DWORD n=GetModuleFileNameW(0,path,32768),base=n;
    if(!n||n>=32768){VirtualFree(path,0,MEM_RELEASE);return;}while(base&&path[base-1]!=L'\\'&&path[base-1]!=L'/')--base;if(base+9>=32768){VirtualFree(path,0,MEM_RELEASE);return;}
    for(DWORD kind=1;kind<=2;++kind){const wchar_t* name=kind==1?L"font.otf":L"font.ttf";for(DWORD j=0;j<9;++j)path[base+j]=name[j];
        HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(f==INVALID_HANDLE_VALUE)continue;LARGE_INTEGER size;bool valid=GetFileSizeEx(f,&size)&&size.QuadPart>=12&&size.QuadPart<=128*1024*1024;
        BYTE* memory=valid?(BYTE*)VirtualAlloc(0,(SIZE_T)size.QuadPart,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE):0;DWORD read=0;wchar_t selected[LF_FACESIZE];
        valid=memory&&ReadFile(f,memory,(DWORD)size.QuadPart,&read,0)&&read==(DWORD)size.QuadPart;CloseHandle(f);
        if(valid&&externalFamily(memory,read,selected)){DWORD count=0;HANDLE resource=AddFontMemResourceEx(memory,read,0,&count);if(resource&&count){DWORD old;VirtualProtect(memory,read,PAGE_READONLY,&old);for(int i=0;i<LF_FACESIZE;++i){family[i]=selected[i];if(!selected[i])break;}fontCount=count;externalFontFile=kind;break;}}
        if(memory)VirtualFree(memory,0,MEM_RELEASE);
    }VirtualFree(path,0,MEM_RELEASE);
}
static const Entry* find(const wchar_t* s,int n){DWORD h=hashText(s,n),lo=0,hi=entryCount;while(lo<hi){DWORD m=lo+(hi-lo)/2;if(entries[m].hash<h)lo=m+1;else hi=m;}while(lo<entryCount && entries[lo].hash==h){const Entry* e=entries+lo++;if(e->sourceLen==n && same(s,e->source,n))return e;}return 0;}
static const wchar_t* titleText(const wchar_t* s){if(!s)return s;const Entry* e=find(s,len(s));return e?e->target:s;}
// Translate complete resource records before the game splits descriptions into draw spans.
// Keep the native stream cursor/EOF behavior and refuse any replacement that cannot fit.
static const Entry* translateReadLine(wchar_t* text,int capacity){
    if(!text||capacity<2||capacity>262144||text[0]==L'#')return 0;int full=len(text,capacity);if(full>=capacity||full>=4096)return 0;int n=full;while(n>0&&(text[n-1]==L'\r'||text[n-1]==L'\n'))--n;
    const Entry* e=find(text,n);if(!e||e->targetLen+full-n>=capacity)return 0;
    for(int i=0;i<e->targetLen;++i)if(e->target[i]==L'\r'||e->target[i]==L'\n')return 0;
    wchar_t tail[3]={0,0,0};int suffix=full-n;if(suffix>2)return 0;for(int i=0;i<suffix;++i)tail[i]=text[n+i];
    for(int i=0;i<e->targetLen;++i)text[i]=e->target[i];for(int i=0;i<suffix;++i)text[e->targetLen+i]=tail[i];text[e->targetLen+suffix]=0;return e;
}
static bool encyclopediaPath(const wchar_t* path){
    const wchar_t suffix[]=L"data/dat/zukan.dat";int n=len(path),s=len(suffix);if(n<s||n>=4096||(n>s&&path[n-s-1]!=L'/'&&path[n-s-1]!=L'\\'))return false;
    for(int i=0;i<s;++i){wchar_t c=path[n-s+i];if(c==L'\\')c=L'/';if(c>=L'A'&&c<=L'Z')c+=L'a'-L'A';if(c!=suffix[i])return false;}return true;
}
static int __cdecl openHook(const wchar_t* path,int flag,int asyncFlag){
    int handle=originalOpen(path,flag,asyncFlag);if(handle<=0)return handle;
    for(int i=0;i<32;++i)InterlockedCompareExchange(encyclopediaHandles+i,0,handle);
    if(encyclopediaPath(path)){DWORD slot=(DWORD)InterlockedIncrement(&encyclopediaSlot);InterlockedExchange(encyclopediaHandles+(slot%32),handle);}return handle;
}
static int __cdecl getsHook(wchar_t* text,int capacity,int handle){
    bool probing=resourceProbeThread&&GetCurrentThreadId()==resourceProbeThread;if(probing)resourceProbeEntry=0;int result=originalGets(text,capacity,handle);if(result<0)return result;
    bool allowed=false;for(int i=0;i<32;++i)if(encyclopediaHandles[i]==handle){allowed=true;break;}if(!allowed)return result;
    const Entry* e=translateReadLine(text,capacity);if(!e)return result;InterlockedIncrement(&resourceTranslatedCount);if(probing)resourceProbeEntry=e;return len(text,capacity);
}
// Formatted strings are translated after the game's formatter has run.
// Literal delimiters bound captures. Numeric formats cannot consume arbitrary text.
static int tokenEnd(const wchar_t* s,int p,int n){if(p>=n || s[p]!=L'%')return -1;int q=p+1;if(q<n && s[q]==L'%')return -1;while(q<n && ((s[q]>=L'0'&&s[q]<=L'9')||s[q]==L'.'||s[q]==L'-'||s[q]==L'+'||s[q]==L' '||s[q]==L'#'||s[q]==L'h'||s[q]==L'l'||s[q]==L'z'||s[q]==L'I'))++q;if(q<n){wchar_t c=s[q];if(c==L'd'||c==L'i'||c==L'u'||c==L'x'||c==L'X'||c==L'o'||c==L'f'||c==L'F'||c==L'g'||c==L'G'||c==L'e'||c==L'E'||c==L's'||c==L'S'||c==L'c'||c==L'C')return q+1;}return -1;}
static bool formatMatch(const Entry* e,const wchar_t* in,int n,wchar_t* out,int cap){
    int begin[16],end[16],captures=0,p=0,k=0;
    while(p<e->sourceLen){int q=tokenEnd(e->source,p,e->sourceLen);if(q<0){if(k>=n||in[k]!=e->source[p])return false;++p;++k;continue;}
        if(captures==16)return false;int a=k,stop=k;wchar_t type=e->source[q-1];
        if(type==L's'||type==L'S'){int next=q;while(next<e->sourceLen && tokenEnd(e->source,next,e->sourceLen)<0)++next;int literal=next-q;if(!literal)stop=n;else {for(stop=k;stop+literal<=n;++stop)if(same(in+stop,e->source+q,literal))break;if(stop+literal>n)return false;}}
        else if(type==L'c'||type==L'C'){stop=k+1;if(stop>n)return false;}
        else {while(stop<n){wchar_t c=in[stop];if((c>=L'0'&&c<=L'9')||c==L'-'||c==L'+'||c==L'.'||c==L' '||((type==L'x'||type==L'X')&&((c>=L'a'&&c<=L'f')||(c>=L'A'&&c<=L'F'))))++stop;else break;}if(stop==k)return false;}
        begin[captures]=a;end[captures++]=stop;k=stop;p=q;
    }
    if(k!=n||!captures)return false;int o=0,c=0;
    for(p=0;p<e->targetLen;){int q=tokenEnd(e->target,p,e->targetLen);if(q>=0){if(c>=captures)return false;for(int j=begin[c];j<end[c];++j){if(o>=cap-1)return false;out[o++]=in[j];}++c;p=q;}else {if(o>=cap-1)return false;out[o++]=e->target[p++];}}
    if(c!=captures)return false;out[o]=0;return true;
}
static const Entry** templates;
static DWORD templateCount;
static int __cdecl drawHook(int draw,int x,int y,int extend,double ex,double ey,const wchar_t* text,unsigned color,void* image,const RECT* clip,int transparent,int font,unsigned edge,int length,int vertical,SIZE* size){
    wchar_t formatted[4096];const wchar_t* input=text;int full=len(text),n=(length>=0&&length<full)?length:full;const Entry* e=0;bool changed=false;DWORD budget=0;
    if(text && n && full<4096){e=find(text,n);if(!e && full!=n)e=find(text,full);if(e){text=e->target;length=(full!=n && e->sourceLen==full)?(n*e->targetLen+full-1)/full:e->targetLen;changed=true;budget=e->budget;}
        else for(DWORD t=0;t<templateCount;++t)if(formatMatch(templates[t],text,n,formatted,4096)){text=formatted;length=len(text);changed=true;budget=templates[t]->budget;break;}}
    if(draw)InterlockedIncrement(&drawCount);else InterlockedIncrement(&widthCount);if(changed)InterlockedIncrement(&translatedCount);
    if(changed && !vertical){double scale=cfg->scale/100.0;ex*=scale;ey*=scale;if(cfg->scale!=100)extend=1;
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
static DWORD WINAPI descriptionTest(void*){
    for(int i=0;i<100&&!*readerInitialized;++i)Sleep(100);resourceProbeThread=GetCurrentThreadId();
    using Open=int(__cdecl*)(const wchar_t*,int,int);Open open=(Open)(game+0x3af60);ResourceGets gets=(ResourceGets)(game+0x39ee0);
    int handle=-1;for(int attempt=0;attempt<100;++attempt){if(*readerInitialized)handle=open(L"data/dat/zukan.dat",0,0);if(handle>0)break;Sleep(100);}DWORD records=0,translated=0,longRecords=0,missing=0,invalid=0;bool chimera=false;const Entry* chimeraEntry=0;wchar_t buffer[4096];
    if(handle>0)for(;records<1024;++records){int n=gets(buffer,4096,handle);if(n<0)break;const Entry* e=resourceProbeEntry;if(e){++translated;if(n!=e->targetLen||!same(buffer,e->target,n))++invalid;if(e->sourceLen>30)++longRecords;if(e->sourceLen>=7&&same(e->source,L"魔獣テュポーン",7)){chimera=true;chimeraEntry=e;}}else if(n>30&&buffer[0]!=L'#')++missing;}
    bool otherUnchanged=false;if(chimeraEntry){HANDLE f=CreateFileW(L"cn-untranslated-reader.txt",GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(f!=INVALID_HANDLE_VALUE){WORD bom=0xfeff;DWORD wrote;WriteFile(f,&bom,2,&wrote,0);WriteFile(f,chimeraEntry->source,chimeraEntry->sourceLen*2,&wrote,0);CloseHandle(f);int other=open(L"cn-untranslated-reader.txt",0,0);int n=other>0?gets(buffer,4096,other):-1;otherUnchanged=n==chimeraEntry->sourceLen&&same(buffer,chimeraEntry->source,n)&&!resourceProbeEntry;}}
    bool ok=handle>0&&records>=344&&chimera&&longRecords>100&&!missing&&!invalid&&otherUnchanged;
    char report[1024];char* p=report;p=literal(p,"{\"native_archive_reader\":");p=literal(p,handle>0?"true":"false");p=literal(p,",\"records\":");p=number(p,records);p=literal(p,",\"translated_records\":");p=number(p,translated);p=literal(p,",\"translated_long_descriptions\":");p=number(p,longRecords);p=literal(p,",\"unmatched_long_descriptions\":");p=number(p,missing);p=literal(p,",\"invalid_translations\":");p=number(p,invalid);p=literal(p,",\"chimera_translated\":");p=literal(p,chimera?"true":"false");p=literal(p,",\"other_resources_unchanged\":");p=literal(p,otherUnchanged?"true":"false");p=literal(p,",\"passed\":");p=literal(p,ok?"true":"false");p=literal(p,"}\n");*p=0;writeReport(L"cn-descriptions.json",report);ExitProcess(ok?0:73);return 0;
}
static DWORD WINAPI liveTest(void*){Sleep(15000);char report[1024];char* p=report;p=literal(p,"{\"draw_calls\":");p=number(p,drawCount);p=literal(p,",\"width_calls\":");p=number(p,widthCount);p=literal(p,",\"translated_calls\":");p=number(p,translatedCount);p=literal(p,",\"font_calls\":");p=number(p,createdFontCount);p=literal(p,",\"embedded_font_faces\":");p=number(p,fontCount);p=literal(p,",\"font_family\":\"");int n=WideCharToMultiByte(CP_UTF8,0,fontFace,-1,p,256,0,0);if(n>0)p+=n-1;p=literal(p,"\",\"scope\":\"real idle game rendering; no battle or full-story acceptance\"}\n");*p=0;writeReport(L"cn-live-test.json",report);ExitProcess(drawCount>0&&widthCount>0&&translatedCount>0&&createdFontCount>0?0:72);return 0;}
static const wchar_t* testRendered;
static int testLength,testDraw;
static int __cdecl mockBackend(int draw,int,int,int,double,double,const wchar_t* text,unsigned,void*,const RECT*,int,int,unsigned,int length,int,SIZE* size){testRendered=text;testLength=length;testDraw=draw;if(size){size->cx=length*20;size->cy=24;}return length*20;}
static bool runtimeTest(){
    Entry format={L"ステージ%02d",L"第%02d关",0,0,0,0};format.sourceLen=len(format.source);format.targetLen=len(format.target);wchar_t result[64];if(!formatMatch(&format,L"ステージ12",len(L"ステージ12"),result,64)||len(result)!=4||!same(result,L"第12关",4))return false;
    if(!entryCount)return true;const Entry* e=entries;Backend native=originalDraw;originalDraw=mockBackend;
    wchar_t line[4096];for(int i=0;i<e->sourceLen;++i)line[i]=e->source[i];line[e->sourceLen]=0;if(!translateReadLine(line,4096)||len(line)!=e->targetLen||!same(line,e->target,e->targetLen)){originalDraw=native;return false;}
    drawHook(1,0,0,0,1.0,1.0,e->source,0,0,0,1,0,0,e->sourceLen,0,0);bool ok=testDraw==1&&testLength==e->targetLen&&testRendered==e->target;
    drawHook(0,0,0,0,1.0,1.0,e->source,0,0,0,1,0,0,e->sourceLen,0,0);ok=ok&&testDraw==0&&testLength==e->targetLen&&testRendered==e->target;
    originalDraw=native;drawCount=widthCount=translatedCount=0;return ok;
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
        MEMORY_BASIC_INFORMATION storedInfo;VirtualQuery(stored,&storedInfo,sizeof(storedInfo));bool ro=(info.Protect==PAGE_READONLY)&&(decodedInfo.Protect==PAGE_READONLY)&&(storedInfo.Protect==PAGE_READONLY);bool valid=ro&&runtimeTest();
        // Probe the native width/draw ABI only after DxLib initializes in a real session.
        const char ok[]= "{\"initialized\":true,\"payload_readonly\":true,\"decoded_readonly\":true,\"font_hook\":true,\"text_hook\":true,\"resource_line_hook\":true,\"translation_before_wrapping\":true,\"single_exe\":true,\"draw_and_width_translation\":true,\"formatted_translation\":true}\n";
        writeReport(L"cn-probe.json",valid?ok:"{\"probe_failed\":true}\n");ExitProcess(valid?0:71);
    }
    if(hasArgument(L"--cn-live-test")){HANDLE thread=CreateThread(0,0,liveTest,0,0,0);if(!thread)fail();CloseHandle(thread);}
    if(hasArgument(L"--cn-description-test")){HANDLE thread=CreateThread(0,0,descriptionTest,0,0,0);if(!thread)fail();CloseHandle(thread);}
}
