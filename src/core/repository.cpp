// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/repository.hpp"
#include "xiphos/core/editable_document.hpp"
#include <algorithm>
#include <stdexcept>
namespace xiphos::core {
namespace {
bool token(char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-';}
std::string escape(const std::string &s){std::string out;const char *hex="0123456789ABCDEF";for(unsigned char c:s){if(token(c)||c=='_'||c=='.'||c=='~')out+=c;else{out+='%';out+=hex[c>>4];out+=hex[c&15];}}return out;}
std::string path(const std::string &s){
 if(s.empty()||s.front()!='/'||s.size()>4096||!editable_document_text(s)||std::any_of(s.begin(),s.end(),[](unsigned char c){return c<32||c==127||c=='\\'||c=='?'||c=='#';}))throw std::invalid_argument("Invalid HTTPS repository directory");
 std::string out;std::size_t at=1;while(at<s.size()){auto end=s.find('/',at);auto part=s.substr(at,end==s.npos?s.npos:end-at);if(part=="."||part=="..")throw std::invalid_argument("Repository path traversal is invalid");if(!part.empty())out+='/'+escape(part);if(end==s.npos)break;at=end+1;}return out;
}
std::string base(const HttpsRepository &r){
 if(r.id.empty()||r.id.size()>128||!std::all_of(r.id.begin(),r.id.end(),[](char c){return token(c)||c=='_'||c=='.';})||r.id=="."||r.id==".."||r.host.empty()||r.host.size()>253||r.port==0)throw std::invalid_argument("Invalid HTTPS repository identity/host");
 std::size_t at=0;while(at<r.host.size()){auto end=r.host.find('.',at);auto part=r.host.substr(at,end==r.host.npos?r.host.npos:end-at);if(part.empty()||part.size()>63||!token(part.front())||part.front()=='-'||part.back()=='-'||!std::all_of(part.begin(),part.end(),token))throw std::invalid_argument("Repository host must be a DNS name or IPv4 address without credentials/scheme");if(end==r.host.npos)break;at=end+1;if(at==r.host.size())throw std::invalid_argument("Repository host has an empty label");}
 return "https://"+r.host+(r.port==443?"":":"+std::to_string(r.port));
}
std::string url(const HttpsRepository &r,const std::string &directory,const std::string &file){auto result=base(r)+path(directory)+"/"+escape(file);if(result.size()>16384)throw std::invalid_argument("Repository URL exceeds bounds");return result;}
}
std::string repository_catalog_url(const HttpsRepository &r){return url(r,r.catalog_directory,"mods.d.tar.gz");}
std::string repository_module_url(const HttpsRepository &r,const std::string &module){
 if(module.empty()||module.size()>4096||!editable_document_text(module)||module=="."||module==".."||std::any_of(module.begin(),module.end(),[](unsigned char c){return c=='/'||c=='\\'||c<32||c==127;}))throw std::invalid_argument("Invalid repository module filename");return url(r,r.package_directory,module+".zip");
}
}
