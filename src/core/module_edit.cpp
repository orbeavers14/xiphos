// SPDX-License-Identifier: GPL-2.0-or-later
#include "module_edit_internal.hpp"
#include "bounded_file.hpp"
#include "publication.hpp"
#include "sdk_text.hpp"
#include "xiphos/core/search_policy.hpp"
#include <rawfiles.h>
#include <rawgenbook.h>
#include <versekey.h>
#include <treekeyidx.h>
#include <glib.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <map>
#include <stdexcept>
namespace xiphos::core {
bool module_edit_arguments(const ModuleEditTarget &t) noexcept {return !t.module.empty()&&t.module.size()<=4096&&t.module.find('\0')==t.module.npos&&!t.reference.empty()&&t.reference.size()<=32768&&t.reference.find('\0')==t.reference.npos&&editable_document_text(t.module)&&editable_document_text(t.reference);}
namespace detail {
namespace {
std::string hash(const std::string &s){std::unique_ptr<gchar,decltype(&g_free)> p(g_compute_checksum_for_data(G_CHECKSUM_SHA256,reinterpret_cast<const guchar *>(s.data()),s.size()),g_free);if(!p)throw std::bad_alloc();return p.get();}
using Files=std::map<std::string,std::string>;
Files scan(const std::filesystem::path &dir){Files files;files.emplace(".",hash("root:"+std::to_string(static_cast<unsigned>(std::filesystem::status(dir).permissions()))));std::size_t bytes=0;for(const auto &item:std::filesystem::recursive_directory_iterator(dir)){
 if((item.is_regular_file()&&std::filesystem::hard_link_count(item.path())>1)||item.is_symlink()||(!item.is_regular_file()&&!item.is_directory()))throw std::invalid_argument("Editable module data contains unsupported files");if(files.size()>=4096)throw std::invalid_argument("Editable module has too many entries");if(item.is_directory()){files.emplace(item.path().lexically_relative(dir).generic_u8string()+"/",hash("directory:"+std::to_string(static_cast<unsigned>(item.status().permissions()))));continue;}if(files.size()>=4096)throw std::invalid_argument("Editable module has too many files");auto text=read_bounded_file(item.path(),64*1024*1024-bytes,"Editable module data");bytes+=text.size();files.emplace(item.path().lexically_relative(dir).generic_u8string(),hash(text)+":"+std::to_string(static_cast<unsigned>(item.status().permissions())));}return files;}
std::string digest(const Files &files){std::string value;for(const auto &f:files){value+=std::to_string(f.first.size())+":"+f.first+f.second;}return hash(value);}
bool contains(const std::filesystem::path &root,const std::filesystem::path &p){auto relative=p.lexically_relative(root);return !relative.empty()&&*relative.begin()!="..";}
template<class Tree>
auto reparent_book(Tree &tree, unsigned long offset, int) -> decltype(tree.reparent(offset)) {
  return tree.reparent(offset);
}
template<class Tree>
bool reparent_book(Tree &, unsigned long, long) {
  throw std::invalid_argument("This SWORD SDK does not support moving general-book sections");
}
bool full_verse(const std::string &text){
 // VerseKey accepts context-relative input ("3:16" becomes Genesis 3:16).
 // An editing target must name one book, chapter and verse explicitly.
 const auto separator=text.find_last_of(":.");
 if(separator==text.npos||separator+1==text.size())return false;
 auto digit=[](unsigned char c){return c>='0'&&c<='9';};
 if(!std::all_of(text.begin()+separator+1,text.end(),digit))return false;
 auto chapter=separator;while(chapter&&digit(text[chapter-1]))--chapter;
 if(chapter==separator)return false;
 return std::any_of(text.begin(),text.begin()+chapter,[](unsigned char c){return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||c>=128;});
}
}
struct ModuleEditStorage::Impl {
 ModuleEditTarget target;sword::ConfigEntMap configuration;std::string driver,source_type,signature,original_digest;std::filesystem::path data,dir,temp,staged,backup;std::string suffix;
 SWTextEncoding encoding;SWTextDirection direction;SWTextMarkup markup;std::string language;bool installed=false,finished=false;
 explicit Impl(sword::SWMgr &manager,const ModuleEditTarget &t):target(t){
  if(!module_edit_arguments(t))throw std::invalid_argument("Invalid module editing target");auto *module=manager.getModule(t.module.c_str());if(!module)throw std::invalid_argument("Editable module is not installed");
  configuration=module->getConfig();
  auto value=[&](const char *key){const auto *v=module->getConfigEntry(key);if(v&&std::char_traits<char>::length(v)>4096)throw std::invalid_argument("Module editing metadata exceeds bounds");return std::string(v?v:"");};driver=value("ModDrv");source_type=value("SourceType");
  if((driver!="RawFiles"&&driver!="RawGenBook")||!value("CipherKey").empty()||module->getEncoding()!=sword::ENC_UTF8)throw std::invalid_argument("Editing requires an unencrypted UTF-8 RawFiles or RawGenBook module");
  if(!t.rename_book_leaf.empty() &&
     (driver!="RawGenBook" || t.create_book_entry || t.rename_book_leaf.size()>4096 ||
      !editable_document_text(t.rename_book_leaf) || t.rename_book_leaf=="." || t.rename_book_leaf==".." ||
      std::any_of(t.rename_book_leaf.begin(),t.rename_book_leaf.end(),
                  [](unsigned char c){return c=='/' || c<32 || c==127;})))
    throw std::invalid_argument("Invalid general-book leaf name");
  if (!t.move_book_parent.empty() &&
      (driver!="RawGenBook" || t.create_book_entry || !t.rename_book_leaf.empty() ||
       t.move_book_parent.front()!='/' || t.move_book_parent.size()>4096 ||
       !editable_document_text(t.move_book_parent)))
    throw std::invalid_argument("Invalid general-book move target");
  if(t.create_book_entry){
   if(driver!="RawGenBook"||t.reference.size()<2||t.reference.size()>4096||t.reference.front()!='/'||t.reference.back()=='/')throw std::invalid_argument("Book creation requires an absolute new tree path");
   for(unsigned char c:t.reference)if(c<32||c==127)throw std::invalid_argument("Invalid new book path");
   for(std::size_t begin=1;begin<t.reference.size();){auto end=t.reference.find('/',begin);auto part=t.reference.substr(begin,end==std::string::npos?end:end-begin);if(part.empty()||part=="."||part=="..")throw std::invalid_argument("New book path is not canonical");if(end==std::string::npos)break;begin=end+1;}
  }
  if(driver=="RawGenBook"&&!value("KeyType").empty()&&value("KeyType")!="TreeKey")throw std::invalid_argument("Editing requires indexed general-book keys");
  if(driver=="RawFiles"&&!value("Versification").empty()&&value("Versification")!="KJV")throw std::invalid_argument("Personal commentary editing requires its supported KJV canon");
  if(driver=="RawFiles"&&!full_verse(t.reference))throw std::invalid_argument("Personal commentary editing requires one full book, chapter and verse");
  encoding=static_cast<SWTextEncoding>(module->getEncoding());direction=static_cast<SWTextDirection>(module->getDirection());markup=static_cast<SWTextMarkup>(module->getMarkup());language=module->getLanguage();
  data=std::filesystem::u8path(value("DataPath"));if(data.is_relative())data=std::filesystem::u8path(value("PrefixPath"))/data;data=std::filesystem::weakly_canonical(data).lexically_normal();while(data!=data.root_path()&&data.filename().empty())data=data.parent_path();
  dir=driver=="RawFiles"?data:data.parent_path();if(data.empty()||dir.empty()||std::filesystem::is_symlink(dir)||!std::filesystem::is_directory(dir))throw std::invalid_argument("Editable module data path is unsupported");
  dir=std::filesystem::canonical(dir);if(driver=="RawFiles")data=dir;else data=dir/data.filename();auto root=std::filesystem::canonical(std::filesystem::u8path(value("PrefixPath")));while(root!=root.root_path()&&root.filename().empty())root=root.parent_path();auto relative=dir.lexically_relative(root);
  if(root==root.root_path()||!contains(root,dir)||std::distance(relative.begin(),relative.end())<2)throw std::invalid_argument("Editable module must own an isolated data directory below its module root");
  for(const auto &m:manager.Modules)if(m.first!=t.module.c_str()){const auto *path=m.second->getConfigEntry("AbsoluteDataPath");if(path&&*path){auto other=std::filesystem::weakly_canonical(std::filesystem::u8path(path));if(contains(dir,other)||contains(other,dir))throw std::invalid_argument("Module data directory overlaps another module");}}
  for(auto key:{"ModDrv","DataPath","SourceType","Encoding","Versification","CipherKey","KeyType","Lang","Direction"}){const auto v=value(key);signature+=std::to_string(v.size())+":"+v;}
  signature=t.module+":"+signature;suffix=driver=="RawFiles"?"":data.filename().u8string();
 }
 std::unique_ptr<sword::SWModule> open(const std::filesystem::path &base){auto prefix=driver=="RawFiles"?base:base/std::filesystem::u8path(suffix);if(driver=="RawFiles")return std::make_unique<sword::RawFiles>(prefix.u8string().c_str(),target.module.c_str(),nullptr,nullptr,encoding,direction,markup,language.c_str());return std::make_unique<sword::RawGenBook>(prefix.u8string().c_str(),target.module.c_str(),nullptr,nullptr,encoding,direction,markup,language.c_str());}
 ModuleEditEntry entry(sword::SWModule &module,const Files &files,bool created=false){
  std::unique_ptr<sword::SWKey> key(module.createKey());std::uint64_t offset=0;std::string reference;
  if(driver=="RawFiles"){auto *verse=dynamic_cast<sword::VerseKey *>(key.get());if(!verse)throw std::invalid_argument("Personal commentary key is unsupported");verse->setAutoNormalize(false);verse->setText(target.reference.c_str());if(verse->getError()||verse->getChapter()<1||verse->getVerse()<1)throw std::invalid_argument("Invalid personal commentary verse");reference=verse->getOSISRef();}
  else{auto *tree=dynamic_cast<sword::TreeKeyIdx *>(key.get());if(!tree||target.reference.front()!='/')throw std::invalid_argument("General-book editing requires an exact existing path");tree->popError();tree->setText(target.reference.c_str());const bool exists=!tree->getError()&&tree->getOffset()!=0&&std::string(tree->getText())==target.reference;
   if(target.create_book_entry&&!created){if(exists)throw std::invalid_argument("Book entry already exists");return {target.module,target.reference,driver,"",hash(signature+":"+target.reference+":"+digest(files)),0,source_type};}
   if(!exists)throw std::invalid_argument("General-book editing path is unavailable");offset=tree->getOffset();reference=tree->getText();}
  module.setKey(*key);auto text=copy_sdk_text(module.getRawEntryBuf(),maximum_editable_document_bytes);auto revision=hash(signature+":"+reference+":"+digest(files));return {target.module,reference,driver,std::move(text),std::move(revision),offset,source_type};
 }
};
ModuleEditStorage::ModuleEditStorage(sword::SWMgr &m,const ModuleEditTarget &t):impl_(std::make_unique<Impl>(m,t)){}
ModuleEditStorage::~ModuleEditStorage(){if(impl_->installed&&!impl_->finished){try{rollback();}catch(...){return;}}if(!impl_->temp.empty()){std::error_code ignored;
 // A failed install may also fail to put the original directory back. Never
 // remove the only surviving original data in that case.
 if(!impl_->finished&&std::filesystem::exists(impl_->backup,ignored))return;
 if(ignored)return;
 std::filesystem::remove_all(impl_->temp,ignored);}}
ModuleEditEntry ModuleEditStorage::read(){auto files=scan(impl_->dir);auto module=impl_->open(impl_->dir);return impl_->entry(*module,files);}
ModuleEditEntry ModuleEditStorage::stage(const std::string &expected,const std::string &text,bool erase){
 if(!editable_document_revision(expected)||!editable_document_text(text)||(erase&&(!text.empty()||impl_->target.create_book_entry)))throw std::invalid_argument("Invalid module edit arguments");auto before=scan(impl_->dir);impl_->original_digest=digest(before);auto old=impl_->open(impl_->dir);auto current=impl_->entry(*old,before);if(!old->isWritable())throw std::invalid_argument("Module data is read-only");if(current.revision!=expected)throw std::invalid_argument("Module changed; reopen before editing");old.reset();
 static std::atomic<unsigned long> counter{0};for(unsigned attempt=0;attempt<64;++attempt){impl_->temp=impl_->dir.parent_path()/(".xiphos-module-edit-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(counter++));if(std::filesystem::create_directory(impl_->temp))break;impl_->temp.clear();}if(impl_->temp.empty())throw std::runtime_error("Cannot stage module edit");std::filesystem::permissions(impl_->temp,std::filesystem::perms::owner_all,std::filesystem::perm_options::replace);impl_->staged=impl_->temp/"new";impl_->backup=impl_->temp/"old";
 std::filesystem::copy(impl_->dir,impl_->staged,std::filesystem::copy_options::recursive);
 sword::ConfigEntMap staged_configuration=impl_->configuration;
 auto configure=[&](const char *name,const std::filesystem::path &path){staged_configuration.erase(name);staged_configuration.emplace(name,path.u8string().c_str());};
 configure("AbsoluteDataPath",impl_->staged);configure("PrefixPath",impl_->staged);configure("DataPath",impl_->driver=="RawFiles"?impl_->staged:impl_->staged/std::filesystem::u8path(impl_->suffix));
 auto module=impl_->open(impl_->staged);module->setConfig(&staged_configuration);
 // Invalidate only a recognized usable index, never an unrelated asset folder
 // or an index from an unsupported backend. Every path stays in the private copy.
 const auto caches=[&]{return std::filesystem::exists(impl_->staged/"lucene")||std::filesystem::exists(impl_->staged/"xapian");};
 const bool indexed=supports_indexed_search(*module);
 if(caches()&&!indexed)throw std::invalid_argument("Existing search cache needs explicit maintenance before editing");
 if(indexed){
  module.reset(); // Close probe handles before deleting the private cache.
  module=impl_->open(impl_->staged);module->setConfig(&staged_configuration);module->deleteSearchFramework();
  if(caches()||supports_indexed_search(*module))throw std::runtime_error("Staged search index invalidation failed");
 }
 (void)impl_->entry(*module,before);if(!module->isWritable())throw std::invalid_argument("Module data is read-only");module->popError();
 if(impl_->target.create_book_entry){auto *tree=dynamic_cast<sword::TreeKeyIdx *>(module->getKey());if(!tree)throw std::runtime_error("Missing staged tree key");tree->popError();tree->assureKeyPath(impl_->target.reference.c_str());tree->save();if(tree->getError()||tree->getOffset()==0||std::string(tree->getText())!=impl_->target.reference)throw std::runtime_error("Staged book path creation did not verify");}
 if (!impl_->target.rename_book_leaf.empty() || !impl_->target.move_book_parent.empty()) {
  if (erase || !text.empty()) throw std::invalid_argument("Rename takes no replacement text");
  auto *tree = dynamic_cast<sword::TreeKeyIdx *>(module->getKey());
  const auto &parent = impl_->target.move_book_parent;
  if (!tree || (parent.empty() && !impl_->target.rename_book_subtree && tree->hasChildren())) throw std::invalid_argument("Only leaf sections can be renamed");
  const auto separator = current.reference.rfind('/');
  const auto destination = parent.empty() ?
      current.reference.substr(0,separator+1) + impl_->target.rename_book_leaf :
      (parent == "/" ? "" : parent) + current.reference.substr(separator);
  std::unique_ptr<sword::SWKey> candidate(module->createKey());
  candidate->setText(destination.c_str());
  if (!candidate->getError() && std::string(candidate->getText()) == destination)
    throw std::invalid_argument("Book rename destination already exists");
  if (parent.empty()) {
    tree->setLocalName(impl_->target.rename_book_leaf.c_str());
    tree->save();
  } else {
    auto *parent_key = dynamic_cast<sword::TreeKeyIdx *>(candidate.get());
    if (!parent_key) throw std::runtime_error("Missing destination tree key");
    parent_key->popError();
    if (parent == "/") parent_key->root();
    else parent_key->setText(parent.c_str());
    if (parent_key->getError() || (parent != "/" && std::string(parent_key->getText()) != parent) ||
        !reparent_book(*tree,parent_key->getOffset(),0))
      throw std::invalid_argument("General-book move destination is unavailable");
  }
  module->flush();
  if (tree->getError() || module->popError()) throw std::runtime_error("SWORD book rename failed");
  module.reset();
  auto after = scan(impl_->staged);
  module = impl_->open(impl_->staged);
  impl_->target.reference = destination;
  auto result = impl_->entry(*module,after);
  if (result.offset != current.offset || result.text != current.text)
    throw std::runtime_error("Staged book rename did not preserve the entry");
  return result;
 }
 if(erase&&impl_->driver=="RawGenBook")module->setEntry("",0);else if(erase)module->deleteEntry();else module->setEntry(text.c_str(),static_cast<long>(text.size()));module->flush();if(module->popError())throw std::runtime_error("SWORD module edit failed");module.reset();
 auto after=scan(impl_->staged);module=impl_->open(impl_->staged);auto result=impl_->entry(*module,after,true);if(result.reference!=current.reference||(!impl_->target.create_book_entry&&result.offset!=current.offset)||result.text!=(erase?std::string{}:text))throw std::runtime_error("Staged module edit did not verify");return result;
}
void ModuleEditStorage::install() {
  if (digest(scan(impl_->dir)) != impl_->original_digest)
    throw std::invalid_argument("Module changed during save");
  publish_new_path(impl_->dir, impl_->backup);
  try {
    publish_new_path(impl_->staged, impl_->dir);
    impl_->installed = true;
  } catch (...) {
    // Preserve the original in the backup if another writer now owns the live path.
    publish_new_path(impl_->backup, impl_->dir);
    throw;
  }
}
void ModuleEditStorage::rollback() {
  if (!impl_->installed) return;
  publish_new_path(impl_->dir, impl_->staged);
  publish_new_path(impl_->backup, impl_->dir);
  impl_->installed = false;
}
void ModuleEditStorage::finish() noexcept {impl_->finished=true;}
}
}
