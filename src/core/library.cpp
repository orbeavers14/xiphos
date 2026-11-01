// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/library.hpp"
#include "xiphos/core/editable_document.hpp"
#include <glib.h>
#include <algorithm>
#include <memory>
#include <set>
#include <tuple>
#include <stdexcept>
namespace xiphos::core {
namespace {
constexpr std::size_t maximum_bytes=16*1024*1024;
void identity(const LibraryIdentity &id){
 if(id.provider.empty()||id.provider.size()>128||!std::all_of(id.provider.begin(),id.provider.end(),[](char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-';})||
    id.item.empty()||id.item.size()>4096||!editable_document_text(id.item))throw std::invalid_argument("Invalid library identity");
}
bool same(const LibraryIdentity &a,const LibraryIdentity &b){return a.provider==b.provider&&a.item==b.item;}
void add(std::string &out,const std::string &text){out+=std::to_string(text.size())+":"+text;}
}
void validate_library_catalog(const LibraryCatalog &c){
 if(c.source.empty()||c.source.size()>4096||!editable_document_text(c.source)||c.items.size()>4096)throw std::invalid_argument("Invalid library catalog bounds");
 std::size_t bytes=c.source.size();std::set<std::pair<std::string,std::string>> seen;
 for(const auto &i:c.items){
  identity(i.identity);if(!seen.emplace(i.identity.provider,i.identity.item).second)throw std::invalid_argument("Duplicate library identity");
  for(const auto *text:{&i.identity.provider,&i.identity.item,&i.title,&i.language,&i.type,&i.translation,&i.edition,&i.version,&i.versification,&i.attribution,&i.license}){
   if(text->size()>32768||!editable_document_text(*text)||text->size()>maximum_bytes-bytes)throw std::invalid_argument("Invalid library metadata bounds");bytes+=text->size();
  }
  auto permission=[](LibraryPermission p){return static_cast<int>(p)>=0&&static_cast<int>(p)<=2;};
  if(i.capabilities&~511u||static_cast<int>(i.access)<0||static_cast<int>(i.access)>4||static_cast<int>(i.delivery)<0||static_cast<int>(i.delivery)>2||!permission(i.download)||!permission(i.offline_storage)||!permission(i.text_export))throw std::invalid_argument("Invalid library capabilities/access");
 }
}
std::string library_catalog_revision(const LibraryCatalog &c){
 validate_library_catalog(c);std::vector<const LibraryItem *> rows;for(const auto &i:c.items)rows.push_back(&i);
 std::sort(rows.begin(),rows.end(),[](auto a,auto b){return std::tie(a->identity.provider,a->identity.item)<std::tie(b->identity.provider,b->identity.item);});
 std::string text;add(text,c.source);
 for(auto i:rows){for(const auto *v:{&i->identity.provider,&i->identity.item,&i->title,&i->language,&i->type,&i->translation,&i->edition,&i->version,&i->versification,&i->attribution,&i->license})add(text,*v);
  for(auto v:{static_cast<int>(i->capabilities),static_cast<int>(i->access),static_cast<int>(i->delivery),i->installed?1:0,static_cast<int>(i->download),static_cast<int>(i->offline_storage),static_cast<int>(i->text_export)})add(text,std::to_string(v));
 }
 std::unique_ptr<gchar,decltype(&g_free)> result(g_compute_checksum_for_data(G_CHECKSUM_SHA256,reinterpret_cast<const guchar *>(text.data()),text.size()),g_free);
 if(!result)throw std::bad_alloc();return result.get();
}
LibraryInstallPlan plan_library_install(const LibraryCatalog &source,const LibraryCatalog &installed,const LibraryIdentity &id){
 identity(id);LibraryInstallPlan plan;plan.identity=id;plan.source_revision=library_catalog_revision(source);plan.destination_revision=library_catalog_revision(installed);
 // Destination means installed inventory, never a second repository listing.
 for(const auto &i:installed.items)if(!i.installed)throw std::invalid_argument("Install destination contains uninstalled entries");
 auto found=std::find_if(source.items.begin(),source.items.end(),[&](const auto &i){return same(i.identity,id);});
 if(found==source.items.end()){plan.decision=LibraryInstallDecision::unavailable;return plan;}
 const auto &item=*found;plan.delivery=item.delivery;plan.source_version=item.version;
 auto existing=std::find_if(installed.items.begin(),installed.items.end(),[&](const auto &i){return same(i.identity,id);});
 if(existing!=installed.items.end())plan.installed_version=existing->version;
 if(item.access==LibraryAccess::sign_in_required){plan.decision=LibraryInstallDecision::sign_in_required;return plan;}
 if(item.access==LibraryAccess::license_required){plan.decision=LibraryInstallDecision::license_required;return plan;}
 if(item.access==LibraryAccess::unavailable){plan.decision=LibraryInstallDecision::unavailable;return plan;}
 if(item.installed||item.delivery==LibraryDelivery::none||
   (item.delivery==LibraryDelivery::local_sword&&(id.provider!="sword"||!(item.capabilities&library_local_install)))||
   (item.delivery==LibraryDelivery::provider_package&&!(item.capabilities&library_offline_package))){plan.decision=LibraryInstallDecision::unsupported;return plan;}
 if(item.delivery==LibraryDelivery::provider_package){
  if(item.download!=LibraryPermission::allowed){plan.decision=item.download==LibraryPermission::denied?LibraryInstallDecision::permission_denied:LibraryInstallDecision::permission_unknown;return plan;}
  // Permission to download alone is insufficient to promise retained offline use.
  if(item.offline_storage!=LibraryPermission::allowed){plan.decision=item.offline_storage==LibraryPermission::denied?LibraryInstallDecision::permission_denied:LibraryInstallDecision::permission_unknown;return plan;}
 }
 plan.decision=existing==installed.items.end()?LibraryInstallDecision::install:
  (!item.version.empty()&&item.version==existing->version?LibraryInstallDecision::already_installed:LibraryInstallDecision::replace_confirmation_required);
 return plan;
}
}
