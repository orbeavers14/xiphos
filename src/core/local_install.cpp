// SPDX-License-Identifier: GPL-2.0-or-later
#include "local_install_internal.hpp"
#include "sword_library.hpp"
#include "uncached_manager.hpp"
#include "bounded_file.hpp"
#include "xiphos/core/editable_document.hpp"
#include <swmodule.h>
#include <glib.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
namespace xiphos::core {
bool local_install_arguments(const LocalInstallRequest &r) noexcept {
 return !r.source.empty()&&r.source.size()<=4096&&editable_document_text(r.source)&&!r.module.empty()&&r.module.size()<=4096&&editable_document_text(r.module)&&editable_document_revision(r.source_revision)&&editable_document_revision(r.destination_revision);
}
namespace detail {
namespace {
namespace fs=std::filesystem;
using Manifest=std::map<std::string,std::string>;
std::string hash(const std::string &text){std::unique_ptr<gchar,decltype(&g_free)> p(g_compute_checksum_for_data(G_CHECKSUM_SHA256,reinterpret_cast<const guchar *>(text.data()),text.size()),g_free);if(!p)throw std::bad_alloc();return p.get();}
bool inside(const fs::path &root,const fs::path &path){auto relative=path.lexically_relative(root);return !relative.empty()&&*relative.begin()!="..";}
fs::path root_path(const std::string &value){
 auto p=fs::u8path(value);if(value.empty()||value.size()>4096||!editable_document_text(value)||fs::is_symlink(p)||!fs::is_directory(p)||!fs::is_directory(p/"mods.d"))throw std::invalid_argument("Installation requires an explicit existing library root with mods.d");
 p=fs::canonical(p);if(p==p.root_path()||std::distance(p.begin(),p.end())<3)throw std::invalid_argument("Installation root is too broad");return p;
}
Manifest scan(const fs::path &root){
 Manifest result;result["."]=std::to_string(static_cast<unsigned>(fs::status(root).permissions()));std::size_t bytes=0,names=0;
 for(const auto &entry:fs::recursive_directory_iterator(root)){
  if(result.size()>=16384||entry.is_symlink()||(!entry.is_directory()&&!entry.is_regular_file())||(entry.is_regular_file()&&fs::hard_link_count(entry.path())!=1))throw std::invalid_argument("Installation root has unsupported files or too many entries");
  auto name=entry.path().lexically_relative(root).generic_u8string();names+=name.size();if(name.size()>4096||names>16*1024*1024)throw std::invalid_argument("Installation paths exceed bounds");
  std::string content=std::to_string(static_cast<unsigned>(entry.status().permissions()));
  if(entry.is_regular_file()){
   if(fs::file_size(entry.path())>512*1024*1024-bytes)throw std::invalid_argument("Installation root exceeds 512MiB");
   std::unique_ptr<GChecksum,decltype(&g_checksum_free)> checksum(g_checksum_new(G_CHECKSUM_SHA256),g_checksum_free);if(!checksum)throw std::bad_alloc();
   std::ifstream input(entry.path(),std::ios::binary);if(!input)throw std::runtime_error("Cannot read installation data");std::array<char,65536> buffer{};
   while(input){input.read(buffer.data(),buffer.size());auto count=static_cast<std::size_t>(input.gcount());if(count>512*1024*1024-bytes)throw std::invalid_argument("Installation root exceeds 512MiB");bytes+=count;g_checksum_update(checksum.get(),reinterpret_cast<const guchar *>(buffer.data()),count);}
   if(!input.eof()||input.bad())throw std::runtime_error("Cannot verify installation data");content+=":";content+=g_checksum_get_string(checksum.get());
  }else name+='/';
  result.emplace(std::move(name),std::move(content));
 }return result;
}
std::string revision(const fs::path &root,const Manifest &files){const auto name=root.u8string();std::string value=std::to_string(name.size())+":"+name;for(const auto &f:files)value+=std::to_string(f.first.size())+":"+f.first+std::to_string(f.second.size())+":"+f.second;return hash(value);}
std::string trim(const std::string &value){auto first=value.find_first_not_of(" \t\r");return first==value.npos?"":value.substr(first,value.find_last_not_of(" \t\r")-first+1);}
struct Layout {fs::path data,config;std::string config_bytes;};
fs::path data_directory(sword::SWModule &m){
 const auto *driver=m.getConfigEntry("ModDrv"),*raw=m.getConfigEntry("DataPath");if(!driver||!raw||!*raw)throw std::invalid_argument("Module installation layout is missing");
 const std::string d(driver),text(raw);static const std::set<std::string> directories{"RawText","RawText4","RawCom","RawCom4","RawFiles","zText","zText4","zCom","zCom4"},prefixes{"RawLD","RawLD4","zLD","RawGenBook"};
 if(!directories.count(d)&&!prefixes.count(d))throw std::invalid_argument("Module installation driver layout is unsupported");
 auto path=fs::u8path(text);if(text.size()>4096||text.find('\\')!=text.npos||text.find(':')!=text.npos||path.is_absolute())throw std::invalid_argument("Module data must use a portable relative path");
 for(const auto &part:path)if(part=="..")throw std::invalid_argument("Module data escapes its root");
 path=path.lexically_normal();while(!path.empty()&&path.filename().empty())path=path.parent_path();if(prefixes.count(d))path=path.parent_path();
 if(path.empty()||path=="."||std::distance(path.begin(),path.end())<2||*path.begin()=="mods.d")throw std::invalid_argument("Module must own an isolated data directory");return path;
}
Layout layout(sword::SWMgr &manager,const fs::path &root,const std::string &id){
 auto *module=manager.getModule(id.c_str());if(!module)throw std::invalid_argument("Module is not present in the selected library");
 if(module->getConfig().find("File")!=module->getConfig().end())throw std::invalid_argument("File-list module installation is not supported");
 Layout result;result.data=data_directory(*module);
 if(!fs::is_directory(root/result.data))throw std::invalid_argument("Module data directory is missing");
 for(const auto &other:manager.Modules)if(other.first!=id.c_str()){auto data=data_directory(*other.second);if(inside(result.data,data)||inside(data,result.data))throw std::invalid_argument("Module data overlaps another module");}
 for(const auto &entry:fs::directory_iterator(root/"mods.d"))if(entry.is_regular_file()&&entry.path().extension()==".conf"){
  auto bytes=read_bounded_file(entry.path(),4*1024*1024,"Module configuration");if(!editable_document_text(bytes))throw std::invalid_argument("Module configuration requires UTF-8 source");
  std::istringstream lines(bytes);std::string line;std::set<std::string> sections;
  while(std::getline(lines,line)){line=trim(line);if(!line.empty()&&line.front()=='['){auto close=line.find(']');if(close!=line.npos)sections.insert(line.substr(1,close-1));}}
  if(sections.count(id)){if(sections.size()!=1||!result.config.empty())throw std::invalid_argument("Installation requires one unambiguous single-module configuration file");result.config=entry.path().filename();result.config_bytes=std::move(bytes);}
 }
 if(result.config.empty())throw std::invalid_argument("Module configuration file is unavailable");return result;
}
}
struct LocalInstallStorage::Impl {
 fs::path source,destination,temp,staged,backup;std::string module,source_revision,destination_revision;bool installed=false,finished=false;
 Impl(const std::string &s,const std::string &d,const std::string &m):source(root_path(s)),destination(root_path(d)),module(m){
  if(m.empty()||m.size()>4096||!editable_document_text(m)||inside(source,destination)||inside(destination,source))throw std::invalid_argument("Installation source and destination must be separate roots");
 }
};
LocalInstallStorage::LocalInstallStorage(const std::string &s,const std::string &d,const std::string &m):impl_(std::make_unique<Impl>(s,d,m)){}
LocalInstallStorage::~LocalInstallStorage(){if(impl_->installed&&!impl_->finished){try{rollback();}catch(...){return;}}if(!impl_->temp.empty()){std::error_code ec;if(!impl_->finished&&fs::exists(impl_->backup,ec))return;if(ec)return;fs::remove_all(impl_->temp,ec);}}
LocalInstallPreview LocalInstallStorage::preview(){
 auto source_files=scan(impl_->source),destination_files=scan(impl_->destination);UncachedEngineManager source(impl_->source.u8string().c_str(),true,nullptr,false,false,true),destination(impl_->destination.u8string().c_str(),true,nullptr,false,false,true);
 auto selected=layout(source,impl_->source,impl_->module);
 if(destination.getModule(impl_->module.c_str()))(void)layout(destination,impl_->destination,impl_->module);
 for(const auto &other:destination.Modules)if(other.first!=impl_->module.c_str()){auto data=data_directory(*other.second);if(inside(selected.data,data)||inside(data,selected.data))throw std::invalid_argument("New data directory overlaps another installed module");}
 auto from=sword_library_catalog(source,"sword.local",false),to=sword_library_catalog(destination,"sword.installed",true);auto plan=plan_library_install(from,to,{"sword",impl_->module});
 return {impl_->module,revision(impl_->source,source_files),revision(impl_->destination,destination_files),plan.source_version,plan.installed_version,plan.decision};
}
LocalInstallResult LocalInstallStorage::stage(const LocalInstallRequest &request){
 if(!local_install_arguments(request)||request.module!=impl_->module||root_path(request.source)!=impl_->source)throw std::invalid_argument("Invalid installation request");
 const auto current=preview();if(current.source_revision!=request.source_revision)throw std::invalid_argument("Source changed; prepare installation again");if(current.destination_revision!=request.destination_revision)throw std::invalid_argument("Destination changed; prepare installation again");
 const bool replacing=current.decision==LibraryInstallDecision::replace_confirmation_required||current.decision==LibraryInstallDecision::already_installed;
 if((replacing&&!request.replace)||(!replacing&&current.decision!=LibraryInstallDecision::install))throw std::invalid_argument("Module replacement needs explicit authorization or source is unavailable");
 impl_->source_revision=current.source_revision;impl_->destination_revision=current.destination_revision;
 static std::atomic<unsigned long> serial{0};for(unsigned attempt=0;attempt<64;++attempt){impl_->temp=impl_->destination.parent_path()/(".xiphos-install-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(serial++));if(fs::create_directory(impl_->temp))break;impl_->temp.clear();}if(impl_->temp.empty())throw std::runtime_error("Cannot create installation staging directory");
 fs::permissions(impl_->temp,fs::perms::owner_all,fs::perm_options::replace);impl_->staged=impl_->temp/"new";impl_->backup=impl_->temp/"old";
 fs::copy(impl_->destination,impl_->staged,fs::copy_options::recursive);
 if(scan(impl_->staged)!=scan(impl_->destination))throw std::runtime_error("Staged library clone did not verify");
 {
  UncachedEngineManager source(impl_->source.u8string().c_str(),true,nullptr,false,false,true),destination(impl_->destination.u8string().c_str(),true,nullptr,false,false,true);auto from=layout(source,impl_->source,impl_->module);
  if(replacing){auto old=layout(destination,impl_->destination,impl_->module);fs::remove_all(impl_->staged/old.data);fs::remove(impl_->staged/"mods.d"/old.config);}
  if(fs::exists(impl_->staged/from.data)||fs::exists(impl_->staged/"mods.d"/from.config))throw std::invalid_argument("Installation would overwrite unrelated data or configuration");
  fs::create_directories((impl_->staged/from.data).parent_path());fs::copy(impl_->source/from.data,impl_->staged/from.data,fs::copy_options::recursive);fs::copy_file(impl_->source/"mods.d"/from.config,impl_->staged/"mods.d"/from.config);
  if(scan(impl_->source/from.data)!=scan(impl_->staged/from.data)||read_bounded_file(impl_->staged/"mods.d"/from.config,4*1024*1024)!=from.config_bytes)throw std::runtime_error("Staged module data/configuration did not verify");
 }
 if(revision(impl_->source,scan(impl_->source))!=impl_->source_revision||revision(impl_->destination,scan(impl_->destination))!=impl_->destination_revision)throw std::invalid_argument("Library changed while staging installation");
 // Scan the complete result and verify SDK discoverability before live changes.
 fs::remove(impl_->staged/"mods.d/modules-conf.cache");
 auto files=scan(impl_->staged);UncachedEngineManager verify(impl_->staged.u8string().c_str(),true,nullptr,false,false,true);if(!verify.getModule(impl_->module.c_str()))throw std::runtime_error("Staged module cannot be loaded");
 return {impl_->module,current.source_version,current.source_revision,revision(impl_->destination,files),replacing};
}
void LocalInstallStorage::install(){
 if(impl_->staged.empty()||revision(impl_->source,scan(impl_->source))!=impl_->source_revision||revision(impl_->destination,scan(impl_->destination))!=impl_->destination_revision)throw std::invalid_argument("Library changed before installation commit");
 fs::rename(impl_->destination,impl_->backup);try{fs::rename(impl_->staged,impl_->destination);impl_->installed=true;}catch(...){fs::rename(impl_->backup,impl_->destination);throw;}
}
void LocalInstallStorage::rollback(){if(!impl_->installed)return;fs::rename(impl_->destination,impl_->staged);fs::rename(impl_->backup,impl_->destination);impl_->installed=false;}
void LocalInstallStorage::finish() noexcept{impl_->finished=true;}
}
}
