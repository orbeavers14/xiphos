// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/legacy_repositories.hpp"
#include "xiphos/core/editable_document.hpp"
#include "bounded_file.hpp"
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <stdexcept>
namespace xiphos::core {
namespace {
std::string trim(std::string s){auto first=s.find_first_not_of(" \t\r");if(first==s.npos)return {};return s.substr(first,s.find_last_not_of(" \t\r")-first+1);}
std::vector<std::string> fields(const std::string &s){std::vector<std::string> result;std::size_t at=0;for(;;){auto end=s.find('|',at);result.push_back(s.substr(at,end==s.npos?s.npos:end-at));if(end==s.npos||result.size()>6)break;at=end+1;}return result;}
void invalid(){throw std::invalid_argument("Invalid legacy repository preview or bounds");}
}
LegacyRepositoryPreview preview_legacy_repositories(const std::string &bytes){
 if(bytes.size()>maximum_legacy_repository_bytes||!editable_document_text(bytes))invalid();
 LegacyRepositoryPreview result;std::istringstream input(bytes);std::string row;bool sources=false;std::size_t line=0;
 while(std::getline(input,row)){
  ++line;row=trim(row);if(line==1&&row.compare(0,3,"\xef\xbb\xbf")==0)row=trim(row.substr(3));
  if(row.empty()||row[0]=='#'||row[0]==';')continue;
  if(row[0]=='['){sources=row=="[Sources]";continue;}
  if(!sources)continue;
  if(result.candidates.size()>=256)invalid();
  LegacyRepositoryCandidate candidate;candidate.line=line;
  auto equals=row.find('=');
  if(equals==row.npos){result.candidates.push_back(candidate);continue;}
  auto key=trim(row.substr(0,equals));
  // Unknown configuration keys do not necessarily describe a source.
  if(key.size()<6||key.compare(key.size()-6,6,"Source"))continue;
  auto values=fields(trim(row.substr(equals+1)));
  if(values.size()<3||values.size()>6){result.candidates.push_back(candidate);continue;}
  if(values.size()>3&&(!values[3].empty()||(values.size()>4&&!values[4].empty())))candidate.disposition=LegacyRepositoryDisposition::credentials;
  else if(key=="FTPSource"||key=="HTTPSource")candidate.disposition=LegacyRepositoryDisposition::insecure_transport;
  else if(key=="DIRSource")candidate.disposition=LegacyRepositoryDisposition::local_directory;
  else if(key!="HTTPSSource")candidate.disposition=LegacyRepositoryDisposition::unsupported;
  else {
   try {
    // Validate using the same endpoint rules as the transfer layer. The dummy
    // package path validates only the host/catalog; it is never an import offer.
    HttpsRepository endpoint{"legacy-preview",values[1],values[2],"/"};
    (void)repository_catalog_url(endpoint);
    if(values[0].empty()||values[0].size()>4096||std::any_of(values[0].begin(),values[0].end(),[](unsigned char c){return c<32||c==127;}))invalid();
    candidate.disposition=LegacyRepositoryDisposition::https_review;
    candidate.name=values[0];candidate.host=values[1];candidate.catalog_directory=values[2];
   }catch(const std::invalid_argument &){candidate.disposition=LegacyRepositoryDisposition::malformed;}
  }
  result.candidates.push_back(std::move(candidate));
 }
 return result;
}
LegacyRepositoryPreview read_legacy_repositories(const std::string &path){
 if(path.empty()||path.size()>4096||!editable_document_text(path))invalid();
 const auto selected=std::filesystem::u8path(path);std::error_code error;
 if(!std::filesystem::is_regular_file(selected,error)||error)invalid();
 return preview_legacy_repositories(detail::read_bounded_file(selected,maximum_legacy_repository_bytes,"Legacy repository configuration"));
}
RepositorySnapshot accept_legacy_repository(RepositorySnapshot snapshot,const LegacyRepositoryCandidate &candidate,std::string id,std::string packages){
 if(candidate.disposition!=LegacyRepositoryDisposition::https_review||std::any_of(snapshot.repositories.begin(),snapshot.repositories.end(),[&](const auto &r){return r.endpoint.id==id;}))invalid();
 return put_repository(std::move(snapshot),{std::move(id),candidate.host,candidate.catalog_directory,std::move(packages)},candidate.name,false);
}
}
