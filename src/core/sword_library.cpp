// SPDX-License-Identifier: GPL-2.0-or-later
#include "sword_library.hpp"
#include "search_source.hpp"
#include "bounded_file.hpp"
#include "xiphos/core/editable_document.hpp"
#include <swmodule.h>
#include <filesystem>
#include <set>
#include <sstream>
#include <stdexcept>
namespace xiphos::core::detail {
namespace {
std::string copy(const char *value,std::size_t &bytes){
 if(!value)return {};std::size_t n=0;while(n<=32768&&value[n])++n;
 if(n>32768||n>16*1024*1024-bytes)throw std::invalid_argument("Library metadata exceeds bounds");
 std::string result(value,n);if(!editable_document_text(result))throw std::invalid_argument("Library metadata is not UTF-8 text");bytes+=n;return result;
}
std::string trim(const std::string &text){auto first=text.find_first_not_of(" \t\r");return first==text.npos?"":text.substr(first,text.find_last_not_of(" \t\r")-first+1);}
// Some SDK builds discard explicitly empty config entries. Preserve only the
// declaration of CipherKey, never its value, to distinguish locked content from
// plain content. SDK metadata remains authoritative for configured keys. This
// read-only marker scan neither loads options nor rewrites configuration.
class CipherMarkers {
 std::set<std::filesystem::path> scanned_;
 std::set<std::string> declared_;
 std::size_t bytes_=0,files_=0;
 void file(const std::filesystem::path &path){
  if(++files_>4096)throw std::invalid_argument("Library configuration has too many files");
  auto source=read_bounded_file(path,16*1024*1024-bytes_,"Library configuration");bytes_+=source.size();
  if(!editable_document_text(source))throw std::invalid_argument("Library configuration is not bounded UTF-8 text");
  std::istringstream lines(source);std::string line,section;
  while(std::getline(lines,line)){line=trim(line);if(line.empty()||line.front()=='#'||line.front()==';')continue;
   if(line.front()=='['){auto close=line.find(']');section=close==line.npos?"":line.substr(1,close-1);continue;}
   auto equal=line.find('=');if(equal!=line.npos&&trim(line.substr(0,equal))=="CipherKey"&&!section.empty())declared_.insert(section);
  }
 }
public:
 void scan(const std::filesystem::path &path){
  auto canonical=std::filesystem::weakly_canonical(path);if(!scanned_.insert(canonical).second)return;
  if(std::filesystem::is_regular_file(path)){file(path);return;}
  if(std::filesystem::is_directory(path))for(const auto &entry:std::filesystem::directory_iterator(path))if(entry.path().extension()==".conf"&&entry.is_regular_file())file(entry.path());
 }
 bool declared(const std::string &module)const{return declared_.count(module)!=0;}
};
}
LibraryCatalog sword_library_catalog(sword::SWMgr &manager,const std::string &source,bool installed){
 LibraryCatalog result;result.source=source;std::size_t bytes=source.size();
 if(bytes>4096)throw std::invalid_argument("Invalid library source identifier");
 auto copied=[&](const char *value){return copy(value,bytes);};
 CipherMarkers markers;if(manager.configPath)markers.scan(std::filesystem::u8path(manager.configPath));
 for(const auto &entry:manager.Modules){const auto *prefix=entry.second->getConfigEntry("PrefixPath");if(prefix&&*prefix)markers.scan(std::filesystem::u8path(prefix)/"mods.d");}
 for(const auto &entry:manager.Modules){
  if(result.items.size()>=4096)throw std::invalid_argument("Library catalog has too many entries");
  if(bytes>16*1024*1024-5)throw std::invalid_argument("Library metadata exceeds bounds");bytes+=5;
  auto &m=*entry.second;LibraryItem item;item.identity={"sword",copied(m.getName())};
  item.title=copied(m.getDescription());item.type=copied(m.getType());item.language=copied(m.getConfigEntry("Lang"));
  item.version=copied(m.getConfigEntry("Version"));item.versification=copied(m.getConfigEntry("Versification"));
  item.attribution=copied(m.getConfigEntry("Copyright"));item.license=copied(m.getConfigEntry("DistributionLicense"));
  // Unknown translation/edition identifiers and rights remain unknown; a module
  // name or a free-form license notice is not a publisher entitlement.
  item.installed=installed;item.delivery=installed?LibraryDelivery::none:LibraryDelivery::local_sword;
  const auto *key=m.getConfigEntry("CipherKey");
  item.access=markers.declared(item.identity.item)&&(!key||!*key)?LibraryAccess::locked:LibraryAccess::available;
  if(installed&&item.access==LibraryAccess::available){item.capabilities|=library_offline_read;if(supports_verse_search(m))item.capabilities|=library_local_search;}
  if(!installed)item.capabilities|=library_local_install;
  result.items.push_back(std::move(item));
 }
 validate_library_catalog(result);return result;
}
LibraryCatalog local_sword_library_catalog(const std::string &root){
 if(root.empty()||root.size()>4096||!editable_document_text(root)||!std::filesystem::is_directory(std::filesystem::u8path(root))||!std::filesystem::is_directory(std::filesystem::u8path(root)/"mods.d"))throw std::invalid_argument("Local SWORD source requires an explicit module root with mods.d");
 // Explicit root, no standard/home augmentation, rendering filters or network.
 sword::SWMgr manager(root.c_str(),true,nullptr,false,false);
 // Source roots are selected separately by the caller; never leak a filesystem
 // location or credential into a provider-neutral catalog identifier.
 return sword_library_catalog(manager,"sword.local",false);
}
}
