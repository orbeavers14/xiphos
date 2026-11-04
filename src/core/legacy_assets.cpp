// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/legacy_assets.hpp"
#include "xiphos/core/editable_document.hpp"
#include "bounded_file.hpp"
#include "remote_io.hpp"
#include <glib.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <type_traits>
namespace xiphos::core {
namespace {
namespace fs=std::filesystem;
constexpr std::uint64_t file_limit=128*1024*1024,tree_limit=1024*1024*1024;
void invalid(){throw std::invalid_argument("Legacy assets require a bounded regular-file tree, portable unique names and a new separate destination");}
void path(const std::string &p){if(p.empty()||p.size()>4096||!editable_document_text(p))invalid();}
std::string hash(const std::string &bytes){std::unique_ptr<gchar,decltype(&g_free)> h(g_compute_checksum_for_data(G_CHECKSUM_SHA256,reinterpret_cast<const guchar *>(bytes.data()),bytes.size()),g_free);if(!h)throw std::bad_alloc();return h.get();}
std::string portable(const std::string &p){std::unique_ptr<gchar,decltype(&g_free)> folded(g_utf8_casefold(p.data(),p.size()),g_free);if(!folded)throw std::bad_alloc();std::unique_ptr<gchar,decltype(&g_free)> normalized(g_utf8_normalize(folded.get(),-1,G_NORMALIZE_NFC),g_free);if(!normalized)throw std::bad_alloc();return normalized.get();}
bool within(const fs::path &child,const fs::path &parent){auto c=child.begin();for(auto p=parent.begin();p!=parent.end();++p,++c)if(c==child.end()||*c!=*p)return false;return true;}
void frame(std::string &s,const std::string &v){s+=std::to_string(v.size())+":"+v;}
void checkpoint(TransferControl *c){if(c)detail::TransferAccess::check(*c);}
std::string read_asset(const fs::path &file,TransferControl *c){return detail::read_bounded_file(file,file_limit,"Legacy asset",[c]{checkpoint(c);});}
LegacyAssets scan(const fs::path &root,TransferControl *control=nullptr){
 checkpoint(control);
 if(!fs::is_directory(fs::symlink_status(root)))invalid();LegacyAssets out;out.source=fs::canonical(root).u8string();std::set<std::string> aliases;
 // GLib's directory reader lets C++ allocation failures unwind normally.
 // Some standard-library directory iterators allocate inside a noexcept
 // advance operation, terminating instead of preserving a staged transaction.
 const auto walk = [&](const auto &self,const fs::path &folder,unsigned depth)->void {
  GError *error=nullptr;std::unique_ptr<GDir,decltype(&g_dir_close)> directory(g_dir_open(folder.u8string().c_str(),0,&error),g_dir_close);
  if(!directory){g_clear_error(&error);throw std::runtime_error("Cannot inspect legacy assets directory");}
  while(const auto *name=g_dir_read_name(directory.get())){checkpoint(control);if(depth>=64||out.entries.size()>=10000)invalid();const auto file=folder/fs::u8path(name);const auto status=fs::symlink_status(file);if(!fs::is_directory(status)&&!fs::is_regular_file(status))invalid();
  LegacyAsset entry;entry.path=file.lexically_relative(root).generic_u8string();path(entry.path);if(!aliases.insert(portable(entry.path)).second)invalid();entry.directory=fs::is_directory(status);
  for(const auto &part:fs::u8path(entry.path)){auto name=part.u8string();if(std::any_of(name.begin(),name.end(),[](unsigned char c){return c<32||c==127;})||name.empty()||name=="."||name==".."||name.find_first_of("\\:*?\"<>|")!=name.npos||name.back()=='.'||name.back()==' ')invalid();auto stem=portable(name.substr(0,name.find('.')));if(stem=="con"||stem=="prn"||stem=="aux"||stem=="nul"||(stem.size()==4&&(stem.rfind("com",0)==0||stem.rfind("lpt",0)==0)&&stem[3]>='1'&&stem[3]<='9'))invalid();}
  if(!entry.directory){auto size=fs::file_size(file);if(size>file_limit||size>tree_limit-out.bytes)invalid();auto bytes=read_asset(file,control);entry.bytes=bytes.size();if(entry.bytes!=size)invalid();entry.revision=hash(bytes);out.bytes+=entry.bytes;}
  const bool subdirectory=entry.directory;out.entries.push_back(std::move(entry));if(control)detail::TransferAccess::expanded(*control,out.bytes,out.entries.size());
  if(subdirectory)self(self,file,depth+1);
  }
 };
 walk(walk,root,0);
 std::sort(out.entries.begin(),out.entries.end(),[](const auto &a,const auto &b){return a.path<b.path;});std::string manifest;
 for(const auto &e:out.entries){frame(manifest,e.path);frame(manifest,e.directory?"directory":"file");frame(manifest,std::to_string(e.bytes));frame(manifest,e.revision);}out.revision=hash(manifest);checkpoint(control);return out;
}
struct Staging {fs::path folder;~Staging(){try{if(!folder.empty()){std::error_code e;fs::remove_all(folder,e);}}catch(...){/* Cleanup must not terminate during allocation failure. */}}};
}
LegacyAssets inspect_legacy_assets(const std::string &source){path(source);return scan(fs::absolute(fs::u8path(source)).lexically_normal());}
LegacyAssets inspect_legacy_assets(const std::string &source,std::shared_ptr<TransferControl> control){if(!control)return inspect_legacy_assets(source);try{detail::TransferAccess::phase(*control,TransferPhase::validating);path(source);auto result=scan(fs::absolute(fs::u8path(source)).lexically_normal(),control.get());detail::TransferAccess::finish(*control);return result;}catch(...){detail::TransferAccess::fail(*control);throw;}}
LegacyAssets copy_legacy_assets(const std::string &source,const std::string &revision,const std::string &destination){return copy_legacy_assets(source,revision,destination,{});}
LegacyAssets copy_legacy_assets(const std::string &source,const std::string &revision,const std::string &destination,std::shared_ptr<TransferControl> control){
 try{
 if(control)detail::TransferAccess::phase(*control,TransferPhase::validating);
 path(source);path(destination);if(!editable_document_revision(revision))invalid();auto original=scan(fs::absolute(fs::u8path(source)).lexically_normal(),control.get());if(original.revision!=revision)throw std::invalid_argument("Legacy assets changed; review again");
 auto requested=fs::absolute(fs::u8path(destination)).lexically_normal();auto target=fs::canonical(requested.parent_path())/requested.filename();auto root=fs::u8path(original.source);if(within(target,root)||within(root,target)||fs::exists(fs::symlink_status(target)))invalid();
 static std::atomic<unsigned long long> sequence{0};Staging stage;for(unsigned n=0;n<64;++n){auto candidate=target.parent_path()/(".xiphos-assets-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(sequence++));if(fs::create_directory(candidate)){stage.folder=std::move(candidate);break;}}if(stage.folder.empty())throw std::runtime_error("Cannot stage legacy asset migration");fs::permissions(stage.folder,fs::perms::owner_all,fs::perm_options::replace);
 if(control){detail::TransferAccess::expanded(*control,0,0);detail::TransferAccess::phase(*control,TransferPhase::extracting);}std::uint64_t written=0,entries=0;
 for(const auto &entry:original.entries){checkpoint(control.get());auto relative=fs::u8path(entry.path),output=stage.folder/relative;if(entry.directory){fs::create_directories(output);fs::permissions(output,fs::perms::owner_all,fs::perm_options::replace);continue;}fs::create_directories(output.parent_path());auto bytes=read_asset(root/relative,control.get());if(bytes.size()!=entry.bytes||hash(bytes)!=entry.revision)throw std::invalid_argument("Legacy assets changed during copy");std::ofstream out(output,std::ios::binary);out.exceptions(std::ios::badbit|std::ios::failbit);for(std::size_t at=0;at<bytes.size();){checkpoint(control.get());auto n=std::min<std::size_t>(65536,bytes.size()-at);out.write(bytes.data()+at,static_cast<std::streamsize>(n));at+=n;written+=n;if(control)detail::TransferAccess::expanded(*control,written,entries);}out.close();++entries;fs::permissions(output,fs::perms::owner_read|fs::perms::owner_write,fs::perm_options::replace);}
 if(control)detail::TransferAccess::phase(*control,TransferPhase::validating);
 auto verified=scan(stage.folder,control.get());if(verified.revision!=revision||scan(root,control.get()).revision!=revision)throw std::invalid_argument("Legacy asset verification changed; originals retained");verified.source=target.u8string(); // Allocate the returned path before publication.
 static_assert(std::is_nothrow_move_constructible_v<LegacyAssets>);
 if(fs::exists(fs::symlink_status(target)))invalid();if(control)detail::TransferAccess::commit(*control);fs::rename(stage.folder,target);stage.folder.clear();if(control)detail::TransferAccess::finish(*control);return verified;
 }catch(...){if(control)detail::TransferAccess::fail(*control);throw;}
}
}
