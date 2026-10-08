#pragma once
#include "core.hpp"
namespace cn {
inline std::vector<std::vector<std::wstring>> csvFields(std::string bytes){
    if(bytes.compare(0,3,"\xef\xbb\xbf")==0)bytes.erase(0,3);
    auto w=wide(bytes);std::vector<std::vector<std::wstring>> records;std::vector<std::wstring> row;std::wstring cell;bool quoted=false,closed=false,start=true;
    for(size_t p=0;p<w.size();++p){wchar_t c=w[p];if(quoted){if(c==L'"'){if(p+1<w.size()&&w[p+1]==L'"'){cell+=c;++p;}else{quoted=false;closed=true;}}else cell+=c;continue;}
        if(c==L'"'){if(!start)throw std::runtime_error("CSV quote inside an unquoted field");quoted=true;start=false;}
        else if(c==L','||c==L'\r'||c==L'\n'){row.push_back(std::move(cell));cell.clear();closed=false;start=true;if(c!=L','){if(c==L'\r'&&p+1<w.size()&&w[p+1]==L'\n')++p;if(row.size()!=1||!row[0].empty())records.push_back(std::move(row));row.clear();}}
        else{if(closed)throw std::runtime_error("CSV data after closing quote");cell+=c;start=false;}
    }
    if(quoted)throw std::runtime_error("CSV unclosed quoted field");if(!row.empty()||!cell.empty()||closed){row.push_back(std::move(cell));records.push_back(std::move(row));}return records;
}
inline std::wstring columnName(std::wstring s){size_t a=s.find_first_not_of(L" \t"),b=s.find_last_not_of(L" \t");if(a==s.npos)return L"";s=s.substr(a,b-a+1);for(auto& c:s)if(c>=L'A'&&c<=L'Z')c+=32;return s;}
inline std::wstring taggedText(const std::wstring& s){std::wstring out;for(auto c:s)switch(c){case L'<':out+=L"<lt>";break;case L'\r':out+=L"<cr>";break;case L'\n':out+=L"<lf>";break;case L'\t':out+=L"<tab>";break;default:out+=c;}return out;}
inline std::wstring untaggedText(const std::wstring& s){std::wstring out;for(size_t p=0;p<s.size();){if(s[p]!=L'<'){out+=s[p++];continue;}size_t end=s.find(L'>',p);if(end==s.npos)throw std::runtime_error("Unclosed control-character tag");auto tag=s.substr(p,end-p+1);if(tag==L"<lt>")out+=L'<';else if(tag==L"<cr>")out+=L'\r';else if(tag==L"<lf>"||tag==L"<newline>")out+=L'\n';else if(tag==L"<tab>")out+=L'\t';else throw std::runtime_error("Unknown control-character tag: "+utf8(tag));p=end+1;}return out;}
inline std::vector<Row> importCSV(const std::string& bytes){auto records=csvFields(bytes);if(records.empty())throw std::runtime_error("CSV header missing");int original=-1,translation=-1,budget=-1,scale=-1,escapes=-1;for(size_t i=0;i<records[0].size();++i){auto c=columnName(records[0][i]);if(c==L"original"||c==L"source"||c==L"原文")original=(int)i;else if(c==L"translated"||c==L"translation"||c==L"target"||c==L"译文"||c==L"翻译")translation=(int)i;else if(c==L"widthpixels"||c==L"width"||c==L"budget"||c==L"宽度像素"||c==L"宽度px")budget=(int)i;else if(c==L"width%"||c==L"scalepercent"||c==L"scale%")scale=(int)i;else if(c==L"escapes")escapes=(int)i;}
    if(original<0||translation<0)throw std::runtime_error("CSV requires Original,Translation/Translated headers");std::vector<Row> rows;
    for(size_t i=1;i<records.size();++i){const auto& r=records[i];if((size_t)std::max(original,translation)>=r.size())throw std::runtime_error("CSV row missing a required column: "+std::to_string(i+1));
        auto numeric=[&](int column,DWORD fallback){if(column<0||(size_t)column>=r.size()||r[column].empty())return fallback;auto v=r[column];if(v.find_first_not_of(L"0123456789")!=v.npos||v.size()>5)throw std::runtime_error("CSV invalid layout value: "+std::to_string(i+1));return (DWORD)std::stoul(v);};
        auto source=r[original],target=r[translation];if(escapes>=0&&(size_t)escapes<r.size()&&!r[escapes].empty()){if(r[escapes]!=L"moekuri-tags-v1")throw std::runtime_error("Unsupported CSV escape format");source=untaggedText(source);target=untaggedText(target);}
        rows.push_back({source,target,numeric(budget,0),numeric(scale,100)});
    }validate(rows);return rows;
}
inline std::string exportCSV(const std::vector<Row>& rows){validate(rows);auto quote=[](const std::wstring& s){std::wstring r=L"\"";for(wchar_t c:s){if(c==L'"')r+=L'"';r+=c;}return r+L'"';};std::wstring out=L"Original,Translated,WidthPixels,Width%,Escapes\r\n";for(const auto& r:rows)out+=quote(taggedText(r.source))+L","+quote(taggedText(r.target))+L","+std::to_wstring(r.budget)+L","+std::to_wstring(r.scale)+L",moekuri-tags-v1\r\n";return std::string("\xef\xbb\xbf")+utf8(out);}
inline std::wstring catalogExtension(const std::wstring& path){auto ext=std::filesystem::path(path).extension().wstring();for(auto& c:ext)if(c>=L'A'&&c<=L'Z')c+=32;return ext;}
inline std::vector<Row> loadCatalog(const std::wstring& path,const std::string& raw){return catalogExtension(path)==L".csv"?importCSV(raw):parse(raw);}
}
