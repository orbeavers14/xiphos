// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/legacy_migration.hpp"
#include "xiphos/core/reader.hpp"
#include "xiphos/core/session.hpp"
#include "xiphos/core/editable_document.hpp"
#include "xml_document.hpp"
#include "bounded_file.hpp"
#include "preference_file.hpp"
#include <glib.h>
#include <libxml/tree.h>
#include <filesystem>
#include <fstream>
#include <charconv>
#include <atomic>
#include <chrono>
#include <set>
#include <algorithm>
namespace xiphos::core {
namespace {
namespace fs=std::filesystem;
constexpr std::size_t source_limit=16*1024*1024;
void invalid(){throw std::invalid_argument("Invalid legacy migration schema, input or bounds");}
std::string hash(const std::string &bytes){std::unique_ptr<gchar,decltype(&g_free)> p(g_compute_checksum_for_data(G_CHECKSUM_SHA256,reinterpret_cast<const guchar *>(bytes.data()),bytes.size()),g_free);if(!p)throw std::bad_alloc();return p.get();}
void path(const std::string &p){if(p.empty()||p.size()>4096||!editable_document_text(p))invalid();}
std::string read(const std::string &p,std::size_t limit=source_limit){path(p);std::error_code e;if(!fs::is_regular_file(fs::u8path(p),e)||e)invalid();return detail::read_bounded_file(fs::u8path(p),limit,"Legacy migration file");}
void write(const fs::path &p,const std::string &bytes){std::ofstream out(p,std::ios::binary|std::ios::trunc);if(!out||!out.write(bytes.data(),bytes.size())||!out.flush())throw std::runtime_error("Migration file write failed");out.close();if(!out)throw std::runtime_error("Migration file close failed");if(detail::read_bounded_file(p,maximum_legacy_migration_bytes)!=bytes)throw std::runtime_error("Migration file verification failed");}
struct Scratch {
 fs::path folder;
 Scratch(){static std::atomic<unsigned long long> sequence{0};for(unsigned n=0;n<64;++n){auto p=fs::temp_directory_path()/("xiphos-migration-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(sequence++));if(fs::create_directory(p)){folder=p;try{fs::permissions(p,fs::perms::owner_all,fs::perm_options::replace);}catch(...){std::error_code e;fs::remove(p,e);throw;}return;}}throw std::runtime_error("Cannot stage migration decoder");}
 ~Scratch(){std::error_code e;fs::remove(folder/"modops.conf",e);fs::remove(folder,e);}
};
ModuleOptionValues config(const std::string &bytes){if(bytes.size()>source_limit||bytes.find('\0')!=bytes.npos)invalid();Scratch scratch;write(scratch.folder/"modops.conf",bytes);return read_module_option_values((scratch.folder/"modops.conf").u8string());}
const LegacyMigrationSource *source(const LegacyMigration &m,unsigned kind){auto i=std::find_if(m.sources.begin(),m.sources.end(),[&](const auto &s){return s.kind==kind;});return i==m.sources.end()?nullptr:&*i;}
std::vector<LegacyMigrationSetting> settings(const std::string &bytes){
 auto doc=detail::parse_xml_document(bytes,"Xiphos");std::vector<LegacyMigrationSetting> result;
 for(auto *section=xmlDocGetRootElement(doc.get())->children;section;section=section->next){if(section->type!=XML_ELEMENT_NODE)continue;
  for(auto *item=section->children;item;item=item->next){if(item->type!=XML_ELEMENT_NODE)continue;if(result.size()>=16384)invalid();
   LegacyMigrationSetting row{reinterpret_cast<const char *>(section->name),reinterpret_cast<const char *>(item->name),{},false};
   auto *v=xmlNodeListGetString(doc.get(),item->children,1);if(v){std::unique_ptr<xmlChar,decltype(xmlFree)> text(v,xmlFree);row.value=reinterpret_cast<const char *>(text.get());}
   if(row.section.size()>4096||row.key.size()>4096||(row.value&&row.value->size()>source_limit*2))invalid();result.push_back(std::move(row));
  }
 }return result;
}
void mark_settings(LegacyMigration &m){
 // Mark only the effective first occurrence, never a shadowed duplicate.
 std::set<std::pair<std::string,std::string>> seen;
 for(auto &row:m.settings){bool first=seen.emplace(row.section,row.key).second;row.mapped=first&&((row.section=="lexicons"&&(row.key=="greek"||row.key=="hebrew"||row.key=="greek_morph_nt"||row.key=="greek_morph_ot"||row.key=="hebrew_morph"))||(row.section=="misc"&&(row.key=="xrefsinverselist"||row.key=="showtexts"||row.key=="showpreview"||row.key=="showcomms"||row.key=="showdicts"))||(!source(m,1)&&((row.section=="modules"&&(row.key=="bible"||row.key=="comm"||row.key=="dict"||row.key=="book"))||(row.section=="keys"&&(row.key=="verse"||row.key=="dictionary"||row.key=="offset")))));
 }
}
std::optional<std::string> get(const LegacyMigration &m,const std::string &section,const std::string &key){for(const auto &s:m.settings)if(s.section==section&&s.key==key)return s.value;return {};}
// Bounded legacy atoi-prefix semantics without overflow or process locale effects.
long long number(const std::optional<std::string> &text,long long fallback=0){if(!text)return fallback;const auto &s=*text;std::size_t at=s.find_first_not_of(" \t\r\n\f\v");if(at==s.npos)return 0;bool negative=s[at]=='-';if(s[at]=='+'||s[at]=='-')++at;unsigned long long n=0;while(at<s.size()&&s[at]>='0'&&s[at]<='9'){if(n>214748364ULL||(n==214748364ULL&&static_cast<unsigned>(s[at]-'0')>(negative?8U:7U)))invalid();n=n*10+(s[at++]-'0');}return negative?-static_cast<long long>(n):static_cast<long long>(n);}
std::string mapped(const std::string &id,const LegacyModuleMapping &mapping){auto i=mapping.find(id);return i==mapping.end()?id:i->second;}
void issue(LegacyMigration &m,std::size_t tab,const std::string &module,const char *reason){if(m.issues.size()==1024)invalid();m.issues.push_back({tab,module,reason});}
void module_text(const std::string &s){if(s.empty()||s.size()>4096||!editable_document_text(s))invalid();}
void validate_sources(const LegacyMigration &m){
 if(m.sources.empty()||m.sources.size()>3||m.issues.size()>1024)invalid();std::set<unsigned> seen;
 for(const auto &s:m.sources){if(s.kind>2||!seen.insert(s.kind).second||s.bytes.size()>source_limit||hash(s.bytes)!=s.revision)invalid();if(s.kind==0)(void)settings(s.bytes);else if(s.kind==1)(void)parse_saved_tab_session(s.bytes);else if(s.bytes.find('\0')!=s.bytes.npos)invalid();}
 if(!seen.count(0))invalid();
 if(m.session){if(!m.issues.empty())invalid();validate_native_session(*m.session);}
 for(const auto &i:m.issues){if(i.tab>256||i.module.size()>4096||i.reason.size()>128||!editable_document_text(i.module)||!editable_document_text(i.reason))invalid();}
}
void value(std::string &out,const std::string &v){auto prefix=std::to_string(v.size())+":";if(prefix.size()+v.size()>maximum_legacy_migration_bytes-out.size())invalid();out+=prefix;out+=v;}
struct Parser {const std::string &s;std::size_t at;
 std::string value(std::size_t max){auto end=s.find(':',at);if(end==s.npos||end==at||end-at>10)invalid();std::size_t n=0;auto p=std::from_chars(s.data()+at,s.data()+end,n);if(p.ec!=std::errc{}||p.ptr!=s.data()+end||(end-at>1&&s[at]=='0')||n>max||n>s.size()-end-1)invalid();at=end+1;auto v=s.substr(at,n);at+=n;return v;}
 std::size_t number(std::size_t max){auto v=value(10);std::size_t n=0;auto p=std::from_chars(v.data(),v.data()+v.size(),n);if(v.empty()||p.ec!=std::errc{}||p.ptr!=v.data()+v.size()||n>max||(v.size()>1&&v[0]=='0'))invalid();return n;}
};
}
LegacyMigration replan_legacy_migration(Reader &reader,LegacyMigration m,const LegacyModuleMapping &mapping){
 validate_sources(m);if(mapping.size()>256)invalid();for(const auto &p:mapping){module_text(p.first);module_text(p.second);}
 m.session.reset();m.issues.clear();m.revision.clear();m.settings=settings(source(m,0)->bytes);
 const auto profile=parse_study_profile(source(m,0)->bytes);const auto preview=parse_study_preview_preferences(source(m,0)->bytes);
 const auto parallels=parse_saved_parallel_profile(source(m,0)->bytes);(void)parse_saved_search_lists(source(m,0)->bytes);
 ModuleOptionValues options;if(const auto *s=source(m,2))options=config(s->bytes);
 StudySessionState base;base.lexicons=profile.lexicons;base.cross_reference_preview_list=preview.cross_references_in_verse_list;base.module_defaults=true;
 for(auto *v:{&base.lexicons->greek,&base.lexicons->hebrew,&base.lexicons->greek_morph_nt,&base.lexicons->greek_morph_ot,&base.lexicons->hebrew_morph})if(*v)**v=mapped(**v,mapping);
 std::set<std::string> preferences;
 for(const auto &p:options){auto id=mapped(p.first,mapping);if(!preferences.insert(id).second)throw std::invalid_argument("Mapped module preference identities collide");base.module_engine[id]=engine_options_from_preferences(p.second);base.presentation[id]=presentation_options_from_preferences(p.second,false);}
 for(const auto &mod:reader.modules()){
  auto found=std::find_if(options.begin(),options.end(),[&](const auto &p){return mapped(p.first,mapping)==mod.id;});auto v=found==options.end()?EngineOptionPreferences{}:found->second;
  base.module_engine[mod.id]=engine_options_from_preferences(v);base.presentation[mod.id]=presentation_options_from_preferences(v,mod.regular_commentary);
 }
 TabVisibility fallback{number(get(m,"misc","showtexts"),1)!=0,number(get(m,"misc","showpreview"),1)!=0,number(get(m,"misc","showcomms"),1)!=0,number(get(m,"misc","showdicts"),1)!=0};
 SavedTabSession saved;
 if(const auto *s=source(m,1))saved=parse_saved_tab_session(s->bytes);
 else {SavedTab tab;tab.text_module=get(m,"modules","bible");tab.commentary_module=get(m,"modules","comm");tab.dictionary_module=get(m,"modules","dict");tab.book_module=get(m,"modules","book");tab.reference=get(m,"keys","verse");tab.dictionary_key=get(m,"keys","dictionary");tab.book_offset_text=get(m,"keys","offset");tab.show_parallel=number(get(m,"misc","showparatab"))?"yes":"no";saved.tabs.push_back(std::move(tab));}
 NativeSessionSnapshot native;native.tabs.reserve(saved.tabs.size());
 for(std::size_t index=0;index<saved.tabs.size();++index){const auto &old=saved.tabs[index];NativeSessionTab tab;tab.key="legacy-"+std::to_string(index+1);tab.visibility=saved_tab_visibility(old,fallback);tab.commentary_showing=saved_tab_yes(old.commentary_showing);tab.parallel_showing=saved_tab_yes(old.show_parallel);
  auto session=StudySession::from_state(reader,base);auto start=m.issues.size();
  auto id=[&](const std::optional<std::string> &name,const char *type)->std::string{
   if(!name||name->empty())return {};module_text(*name);auto resolved=mapped(*name,mapping);auto it=std::find_if(reader.modules().begin(),reader.modules().end(),[&](const Module &mod){return mod.id==resolved;});
   if(it==reader.modules().end()){issue(m,index,*name,"Module is not installed");return {};}
   if((type&&it->type!=type)||(!type&&it->type!="Biblical Texts")){issue(m,index,*name,"Module type is incompatible");return {};}
   return resolved;
  };
  auto bible=id(old.text_module,nullptr),comm=id(old.commentary_module,"Commentaries"),dictionary=id(old.dictionary_module,"Lexicons / Dictionaries"),book=id(old.book_module,"Generic Books");
  if(!bible.empty()){
   if(!old.reference||old.reference->empty())issue(m,index,bible,"Saved reading reference is absent");
   else try{(void)session->visit_passage(bible,comm,*old.reference);}catch(const std::bad_alloc &){throw;}catch(const std::exception &){issue(m,index,bible,"Saved reading selection cannot be restored");}
  }
  if(!dictionary.empty()){
   if(!old.dictionary_key)issue(m,index,dictionary,"Saved dictionary key is absent");
   else try{(void)session->visit_dictionary(dictionary,*old.dictionary_key);}catch(const std::bad_alloc &){throw;}catch(const std::exception &){issue(m,index,dictionary,"Saved dictionary selection cannot be restored");}
  }
  if(!book.empty())try{
   std::optional<BookSelectionTarget> target;
   if(!source(m,1)){auto selection=parse_saved_book_selection(source(m,0)->bytes);selection.module=book;target=saved_book_target(selection,BookRestoreMode::initial_window);}
   else target=book_display_target(book,old.book_offset_text.value_or("0"));
   if(target)(void)session->select_book_document(*target);
  }catch(const std::bad_alloc &){throw;}catch(const std::exception &){issue(m,index,book,"Saved book selection cannot be restored");}
  if(tab.parallel_showing)try{
   if(!parallels.startup_modules||!old.reference||!parallel_read_arguments(*parallels.startup_modules,*old.reference,maximum_parallel_snapshot_bytes))throw std::invalid_argument("Parallel selection absent");std::vector<std::string> modules;for(const auto &module:*parallels.startup_modules){auto actual=id(module,nullptr);modules.push_back(actual);}
   if(m.issues.size()!=start)throw std::invalid_argument("Parallel modules absent");EngineOptions opts;
   const std::pair<const char *,bool EngineOptions::*> fields[]={
    {"Strong_s_Numbers",&EngineOptions::strongs},{"Morphological_Tags",&EngineOptions::morphs},{"Hebrew_Vowel_Points",&EngineOptions::hebrewpoints},{"Hebrew_Cantillation",&EngineOptions::hebrewcant},{"Footnotes",&EngineOptions::footnotes},{"Cross-references",&EngineOptions::scripturerefs},{"Transliteration",&EngineOptions::transliteration},{"Words_of_Christ_in_Red",&EngineOptions::words_in_red},{"Morpheme_Segmentation",&EngineOptions::morphseg},{"Italic_Headings",&EngineOptions::italic_headings},{"Lemmas",&EngineOptions::lemmas},{"Primary_Reading",&EngineOptions::variants_primary},{"Secondary_Reading",&EngineOptions::variants_secondary},{"All_Readings",&EngineOptions::variants_all},{"Greek_Accents",&EngineOptions::greekaccents},{"Transliterated_Forms",&EngineOptions::xlit},{"Enumerations",&EngineOptions::enumerated},{"Glosses",&EngineOptions::glosses}};
   for(const auto &f:fields){bool fallback=f.second==&EngineOptions::hebrewpoints||f.second==&EngineOptions::footnotes||f.second==&EngineOptions::scripturerefs||f.second==&EngineOptions::words_in_red||f.second==&EngineOptions::variants_primary||f.second==&EngineOptions::greekaccents;opts.*f.second=number(get(m,"parallel",f.first),fallback?1:0)!=0;}
   bool headings=number(get(m,"parallel","Headings"),1)!=0;
   (void)session->visit_parallel(modules,*old.reference,opts,headings);
  }catch(const std::bad_alloc &){throw;}catch(const std::exception &){issue(m,index,"","Saved parallel selection cannot be restored");}
  tab.state=session->capture_state();native.tabs.push_back(std::move(tab));
 }
 if(!native.tabs.empty())native.active_key=native.tabs.front().key; // Retained gui_load_tabs selects the first tab.
 if(m.issues.empty()){validate_native_session(native);m.session=std::move(native);}
 mark_settings(m);
 return m;
}
LegacyMigration prepare_legacy_migration(Reader &reader,const LegacyMigrationInput &input,const LegacyModuleMapping &mapping){LegacyMigration m;unsigned kind=0;for(const auto *p:{&input.settings,&input.tabs,&input.module_options}){if(!p->empty()){auto bytes=read(*p);m.sources.push_back({kind,bytes,hash(bytes),*p});}++kind;}return replan_legacy_migration(reader,std::move(m),mapping);}
std::string serialize_legacy_migration(const LegacyMigration &m){validate_sources(m);std::string payload;value(payload,std::to_string(m.sources.size()));for(const auto &s:m.sources){value(payload,std::to_string(s.kind));value(payload,s.revision);value(payload,s.bytes);}value(payload,m.session?serialize_native_session(*m.session):"");value(payload,std::to_string(m.issues.size()));for(const auto &i:m.issues){value(payload,std::to_string(i.tab));value(payload,i.module);value(payload,i.reason);}auto bytes="XiphosMigration:1\n"+hash(payload)+"\n"+payload;if(bytes.size()>maximum_legacy_migration_bytes)invalid();return bytes;}
LegacyMigration parse_legacy_migration(const std::string &bytes){const std::string magic="XiphosMigration:1\n";auto offset=magic.size()+65;if(bytes.size()<offset||bytes.size()>maximum_legacy_migration_bytes||bytes.compare(0,magic.size(),magic)||bytes[offset-1]!='\n'||hash(bytes.substr(offset))!=bytes.substr(magic.size(),64))invalid();Parser p{bytes,offset};LegacyMigration m;auto count=p.number(3);for(std::size_t i=0;i<count;++i){LegacyMigrationSource s;s.kind=static_cast<unsigned>(p.number(2));s.revision=p.value(64);s.bytes=p.value(source_limit);m.sources.push_back(std::move(s));}auto xml=p.value(maximum_session_bytes);if(!xml.empty())m.session=parse_native_session(xml);count=p.number(1024);for(std::size_t i=0;i<count;++i)m.issues.push_back({p.number(256),p.value(4096),p.value(128)});if(p.at!=bytes.size())invalid();validate_sources(m);m.settings=settings(source(m,0)->bytes);mark_settings(m);m.revision=hash(bytes);return m;}
LegacyMigration read_legacy_migration(const std::string &file){return parse_legacy_migration(read(file,maximum_legacy_migration_bytes));}
LegacyMigration save_legacy_migration(const std::string &file,const LegacyMigration &m){path(file);auto bytes=serialize_legacy_migration(m);auto result=parse_legacy_migration(bytes);for(const auto &s:m.sources)if(!s.path.empty()&&hash(read(s.path))!=s.revision)throw std::invalid_argument("A migration source changed; preview again");detail::replace_preference_file(file,[&](const fs::path &temp,const std::string &,bool existed){if(existed)throw std::invalid_argument("Choose a new migration package filename");write(temp,bytes);},maximum_legacy_migration_bytes);return result;}
NativeSessionSnapshot export_legacy_migration_session(const std::string &file,const LegacyMigration &m){path(file);validate_sources(m);if(!m.session)throw std::invalid_argument("Resolve migration issues before exporting a session");for(const auto &s:m.sources)if(!s.path.empty()&&hash(read(s.path))!=s.revision)throw std::invalid_argument("A migration source changed; preview again");auto bytes=serialize_native_session(*m.session);auto result=parse_native_session(bytes);detail::replace_preference_file(file,[&](const fs::path &temp,const std::string &,bool existed){if(existed)throw std::invalid_argument("Choose a new migrated session filename");write(temp,bytes);},maximum_session_bytes);return result;}
void restore_legacy_migration_sources(const std::string &folder,const LegacyMigration &m){path(folder);validate_sources(m);auto destination=fs::u8path(folder);if(!fs::create_directory(destination))throw std::invalid_argument("Choose a new empty backup folder name");std::vector<fs::path> created;try{fs::permissions(destination,fs::perms::owner_all,fs::perm_options::replace);for(const auto &s:m.sources){const char *names[]={"settings.xml","tabs.xml","modops.conf"};auto target=destination/names[s.kind];created.push_back(target);write(target,s.bytes);}}catch(...){std::error_code error;for(const auto &f:created)fs::remove(f,error);fs::remove(destination,error);throw;}}
}
