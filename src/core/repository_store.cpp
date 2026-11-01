// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/repository_store.hpp"
#include "xiphos/core/editable_document.hpp"
#include "preference_file.hpp"
#include <glib.h>
#include <charconv>
#include <algorithm>
#include <fstream>
#include <limits>
#include <memory>
#include <set>
namespace xiphos::core {
namespace {
void invalid(){throw std::invalid_argument("Invalid repository store schema or bounds");}
std::string hash(const std::string &s){std::unique_ptr<gchar,decltype(&g_free)> p(g_compute_checksum_for_data(G_CHECKSUM_SHA256,reinterpret_cast<const guchar *>(s.data()),s.size()),g_free);if(!p)throw std::bad_alloc();return p.get();}
bool same(const HttpsRepository &a,const HttpsRepository &b){return a.id==b.id&&a.host==b.host&&a.port==b.port&&a.catalog_directory==b.catalog_directory&&a.package_directory==b.package_directory;}
void text(const std::string &s,std::size_t max){if(s.size()>max||!editable_document_text(s))invalid();}
struct Writer {
 std::string bytes="XiphosRepositories:1\n";
 void value(const std::string &v){auto prefix=std::to_string(v.size())+":";if(prefix.size()+v.size()>maximum_repository_store_bytes-bytes.size())invalid();bytes+=prefix;bytes+=v;}
 void number(std::uint64_t n){value(std::to_string(n));}
};
struct Parser {
 const std::string &bytes;std::size_t at=21;
 explicit Parser(const std::string &s):bytes(s){const std::string magic="XiphosRepositories:1\n";at=magic.size();if(s.size()>maximum_repository_store_bytes||s.compare(0,at,magic))invalid();}
 std::string value(std::size_t max=32768){auto end=bytes.find(':',at);if(end==bytes.npos||end-at>10||end==at)invalid();std::size_t n=0;auto p=std::from_chars(bytes.data()+at,bytes.data()+end,n);if(p.ec!=std::errc{}||p.ptr!=bytes.data()+end||(end-at>1&&bytes[at]=='0')||n>max||n>bytes.size()-end-1)invalid();at=end+1;auto v=bytes.substr(at,n);at+=n;text(v,max);return v;}
 std::uint64_t number(std::uint64_t max){auto s=value(20);std::uint64_t n=0;auto p=std::from_chars(s.data(),s.data()+s.size(),n);if(s.empty()||p.ec!=std::errc{}||p.ptr!=s.data()+s.size()||n>max||(s.size()>1&&s[0]=='0'))invalid();return n;}
};
void endpoint(Writer &w,const HttpsRepository &r){w.value(r.id);w.value(r.host);w.number(r.port);w.value(r.catalog_directory);w.value(r.package_directory);}
HttpsRepository endpoint(Parser &p){HttpsRepository r;r.id=p.value(128);r.host=p.value(253);r.port=static_cast<std::uint16_t>(p.number(65535));r.catalog_directory=p.value(4096);r.package_directory=p.value(4096);return r;}
void catalog(Writer &w,const RemoteCatalog &c){w.value(c.catalog.source);w.value(c.revision);w.value(c.destination_revision);w.value(c.archive_sha256);w.number(c.catalog.items.size());for(const auto &i:c.catalog.items){
 for(const auto *s:{&i.identity.provider,&i.identity.item,&i.title,&i.language,&i.type,&i.translation,&i.edition,&i.version,&i.versification,&i.attribution,&i.license})w.value(*s);
 w.number(i.capabilities);w.number(static_cast<unsigned>(i.access));w.number(static_cast<unsigned>(i.delivery));w.number(i.installed);w.number(static_cast<unsigned>(i.download));w.number(static_cast<unsigned>(i.offline_storage));w.number(static_cast<unsigned>(i.text_export));}}
RemoteCatalog catalog(Parser &p,const HttpsRepository &r){RemoteCatalog c;c.repository=r;c.catalog.source=p.value(4096);c.revision=p.value(64);c.destination_revision=p.value(64);c.archive_sha256=p.value(64);auto count=p.number(4096);for(std::uint64_t k=0;k<count;++k){LibraryItem i;
 for(auto *s:{&i.identity.provider,&i.identity.item,&i.title,&i.language,&i.type,&i.translation,&i.edition,&i.version,&i.versification,&i.attribution,&i.license})*s=p.value();
 i.capabilities=static_cast<std::uint32_t>(p.number(511));i.access=static_cast<LibraryAccess>(p.number(4));i.delivery=static_cast<LibraryDelivery>(p.number(2));i.installed=p.number(1)!=0;i.download=static_cast<LibraryPermission>(p.number(2));i.offline_storage=static_cast<LibraryPermission>(p.number(2));i.text_export=static_cast<LibraryPermission>(p.number(2));c.catalog.items.push_back(std::move(i));}return c;}
void path(const std::string &s){if(s.empty())invalid();text(s,4096);}
}
RepositorySnapshot put_repository(RepositorySnapshot s,HttpsRepository endpoint,std::string name,bool enabled){
 validate_repository_snapshot(s);auto found=std::find_if(s.repositories.begin(),s.repositories.end(),[&](const auto &r){return r.endpoint.id==endpoint.id;});
 if(found==s.repositories.end())s.repositories.push_back({std::move(endpoint),std::move(name),enabled,std::nullopt,0});
 else{if(!same(found->endpoint,endpoint)){found->cached.reset();found->fetched_at=0;}found->endpoint=std::move(endpoint);found->name=std::move(name);found->enabled=enabled;}
 (void)serialize_repository_snapshot(s);return s;
}
RepositorySnapshot remove_repository(RepositorySnapshot s,const std::string &id){validate_repository_snapshot(s);auto found=std::find_if(s.repositories.begin(),s.repositories.end(),[&](const auto &r){return r.endpoint.id==id;});if(found==s.repositories.end())throw std::invalid_argument("Repository does not exist");s.repositories.erase(found);return s;}
RepositorySnapshot move_repository(RepositorySnapshot s,const std::string &id,std::size_t position){validate_repository_snapshot(s);auto found=std::find_if(s.repositories.begin(),s.repositories.end(),[&](const auto &r){return r.endpoint.id==id;});if(found==s.repositories.end()||position>=s.repositories.size())throw std::invalid_argument("Invalid repository move");auto value=std::move(*found);s.repositories.erase(found);s.repositories.insert(s.repositories.begin()+position,std::move(value));return s;}
RepositorySnapshot cache_repository(RepositorySnapshot s,RemoteCatalog catalog,std::uint64_t fetched_at){validate_repository_snapshot(s);auto found=std::find_if(s.repositories.begin(),s.repositories.end(),[&](const auto &r){return r.endpoint.id==catalog.repository.id;});if(found==s.repositories.end()||!same(found->endpoint,catalog.repository))throw std::invalid_argument("Repository changed before catalog refresh");found->cached=std::move(catalog);found->fetched_at=fetched_at;(void)serialize_repository_snapshot(s);return s;}
void validate_repository_snapshot(const RepositorySnapshot &s){
 if(s.repositories.size()>64)invalid();std::set<std::string> ids;std::size_t items=0;
 for(const auto &r:s.repositories){(void)repository_catalog_url(r.endpoint);(void)repository_module_url(r.endpoint,"validation");text(r.name,4096);if(r.name.empty()||!ids.insert(r.endpoint.id).second)invalid();
  if(r.cached){const auto &c=*r.cached;if(!same(r.endpoint,c.repository)||!editable_document_revision(c.destination_revision)||!editable_document_revision(c.archive_sha256)||library_catalog_revision(c.catalog)!=c.revision)invalid();items+=c.catalog.items.size();if(items>16384)invalid();}
  else if(r.fetched_at)invalid();
 }
}
std::string serialize_repository_snapshot(const RepositorySnapshot &s){validate_repository_snapshot(s);Writer w;w.number(s.repositories.size());for(const auto &r:s.repositories){endpoint(w,r.endpoint);w.value(r.name);w.number(r.enabled);w.number(r.fetched_at);w.number(r.cached.has_value());if(r.cached)catalog(w,*r.cached);}return std::move(w.bytes);}
RepositorySnapshot parse_repository_snapshot(const std::string &bytes){Parser p(bytes);RepositorySnapshot s;auto count=p.number(64);for(std::uint64_t k=0;k<count;++k){SavedRepository r;r.endpoint=endpoint(p);r.name=p.value(4096);r.enabled=p.number(1)!=0;r.fetched_at=p.number(std::numeric_limits<std::uint64_t>::max());if(p.number(1))r.cached=catalog(p,r.endpoint);s.repositories.push_back(std::move(r));}if(p.at!=bytes.size())invalid();validate_repository_snapshot(s);s.revision=hash(bytes);return s;}
RepositorySnapshot read_repository_snapshot(const std::string &file){path(file);return parse_repository_snapshot(detail::read_preference_file(std::filesystem::u8path(file),maximum_repository_store_bytes));}
RepositorySnapshot save_repository_snapshot(const std::string &file,const std::optional<std::string> &expected,const RepositorySnapshot &s){
 path(file);if(expected&&!editable_document_revision(*expected))invalid();const auto bytes=serialize_repository_snapshot(s);auto result=parse_repository_snapshot(bytes);
 detail::replace_preference_file(file,[&](const std::filesystem::path &temp,const std::string &original,bool existed){
  if(existed!=expected.has_value()||(existed&&parse_repository_snapshot(original).revision!=*expected))throw std::invalid_argument("Repository settings changed; reload before saving");
  std::ofstream out(temp,std::ios::binary);out.exceptions(std::ios::badbit|std::ios::failbit);out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));out.close();if(detail::read_preference_file(temp,maximum_repository_store_bytes)!=bytes)throw std::runtime_error("Staged repository store differs");
 },maximum_repository_store_bytes);return result;
}
}
