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
inline std::vector<Row> importCSV(const std::string& bytes){auto records=csvFields(bytes);if(records.empty())throw std::runtime_error("CSV header missing");int original=-1,translation=-1,budget=-1;for(size_t i=0;i<records[0].size();++i){auto c=columnName(records[0][i]);if(c==L"original"||c==L"source"||c==L"原文")original=(int)i;else if(c==L"translation"||c==L"target"||c==L"译文"||c==L"翻译")translation=(int)i;else if(c==L"widthpixels"||c==L"width"||c==L"budget"||c==L"宽度像素"||c==L"宽度px")budget=(int)i;}
    if(original<0||translation<0)throw std::runtime_error("CSV requires Original,Translation headers");std::vector<Row> rows;for(size_t i=1;i<records.size();++i){const auto& r=records[i];if((size_t)std::max(original,translation)>=r.size())throw std::runtime_error("CSV row missing a required column: "+std::to_string(i+1));DWORD px=0;if(budget>=0&&(size_t)budget<r.size()&&!r[budget].empty()){auto v=r[budget];if(v.find_first_not_of(L"0123456789")!=v.npos||v.size()>5)throw std::runtime_error("CSV invalid WidthPixels: "+std::to_string(i+1));px=(DWORD)std::stoul(v);}rows.push_back({r[original],r[translation],px});}validate(rows);return rows;
}
inline std::string exportCSV(const std::vector<Row>& rows){validate(rows);auto quote=[](const std::wstring& s){std::wstring r=L"\"";for(wchar_t c:s){if(c==L'"')r+=L'"';r+=c;}return r+L'"';};std::wstring out=L"Original,Translation,WidthPixels\r\n";for(const auto& r:rows)out+=quote(r.source)+L","+quote(r.target)+L","+std::to_wstring(r.budget)+L"\r\n";return std::string("\xef\xbb\xbf")+utf8(out);}
inline std::wstring catalogExtension(const std::wstring& path){auto ext=std::filesystem::path(path).extension().wstring();for(auto& c:ext)if(c>=L'A'&&c<=L'Z')c+=32;return ext;}
inline std::vector<Row> loadCatalog(const std::wstring& path,const std::string& raw){return catalogExtension(path)==L".csv"?importCSV(raw):parse(raw);}
}
