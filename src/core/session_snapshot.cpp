// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session_state.hpp"
#include "xml_document.hpp"
#include "preference_file.hpp"
#include <array>
#include <charconv>
#include <fstream>
#include <set>
#include <stdexcept>
namespace xiphos::core {
namespace {
using Node=xmlNode;
using BoolField=std::pair<const char *,bool EngineOptions::*>;
const std::array<BoolField,18> engine_fields{{
 {"strongs",&EngineOptions::strongs},{"morphs",&EngineOptions::morphs},{"footnotes",&EngineOptions::footnotes},
 {"greekaccents",&EngineOptions::greekaccents},{"lemmas",&EngineOptions::lemmas},{"scripturerefs",&EngineOptions::scripturerefs},
 {"hebrewpoints",&EngineOptions::hebrewpoints},{"hebrewcant",&EngineOptions::hebrewcant},{"italic_headings",&EngineOptions::italic_headings},
 {"words_in_red",&EngineOptions::words_in_red},{"xlit",&EngineOptions::xlit},{"enumerated",&EngineOptions::enumerated},
 {"glosses",&EngineOptions::glosses},{"morphseg",&EngineOptions::morphseg},{"transliteration",&EngineOptions::transliteration},
 {"variants_primary",&EngineOptions::variants_primary},{"variants_secondary",&EngineOptions::variants_secondary},{"variants_all",&EngineOptions::variants_all}
}};
const std::array<std::pair<const char *,std::optional<std::string> LexiconPreferences::*>,5> lexicon_fields{{
 {"greek",&LexiconPreferences::greek},{"hebrew",&LexiconPreferences::hebrew},{"greek_morph_nt",&LexiconPreferences::greek_morph_nt},
 {"greek_morph_ot",&LexiconPreferences::greek_morph_ot},{"hebrew_morph",&LexiconPreferences::hebrew_morph}
}};
void invalid(){throw std::invalid_argument("Invalid native session schema");}
std::string name(Node *node){return reinterpret_cast<const char *>(node->name);}
Node *child(Node *parent,const char *tag){auto *n=xmlNewChild(parent,nullptr,BAD_CAST tag,nullptr);if(!n)throw std::bad_alloc();return n;}
void attr(Node *n,const char *key,const std::string &v){if(!xmlSetProp(n,BAD_CAST key,BAD_CAST v.c_str()))throw std::bad_alloc();}
void flag(Node *n,const char *key,bool v){attr(n,key,v?"1":"0");}
void number(Node *n,const char *key,std::uint64_t v){attr(n,key,std::to_string(v));}
std::optional<std::string> optional(Node *n,const char *key){
  auto *v=xmlGetProp(n,BAD_CAST key);if(!v)return std::nullopt;
  std::unique_ptr<xmlChar,decltype(xmlFree)> owned(v,xmlFree);
  std::string s(reinterpret_cast<const char *>(v));detail::validate_xml_text(s,32768);return s;
}
std::string required(Node *n,const char *key){auto v=optional(n,key);if(!v)invalid();return *v;}
bool boolean(Node *n,const char *key){auto v=required(n,key);if(v!="0"&&v!="1")invalid();return v=="1";}
std::uint64_t integer(Node *n,const char *key,std::uint64_t max){auto v=required(n,key);std::uint64_t result=0;auto parsed=std::from_chars(v.data(),v.data()+v.size(),result);if(v.empty()||parsed.ec!=std::errc{}||parsed.ptr!=v.data()+v.size()||result>max)invalid();return result;}
void attributes(Node *n,std::set<std::string> allowed){
  if(n->ns||n->nsDef)invalid();
  for(auto *p=n->properties;p;p=p->next)if(p->ns||!allowed.count(reinterpret_cast<const char *>(p->name)))invalid();
}
std::vector<Node *> children(Node *n,const std::set<std::string> &allowed){
  std::vector<Node *> result;
  for(auto *p=n->children;p;p=p->next){
    if(p->type==XML_COMMENT_NODE)continue;
    if(p->type==XML_TEXT_NODE&&xmlIsBlankNode(p))continue;
    if(p->type!=XML_ELEMENT_NODE||!allowed.count(name(p)))invalid();
    result.push_back(p);
  }return result;
}
void leaf(Node *n){if(!children(n,{}).empty())invalid();}
void write_engine(Node *n,const EngineOptions &o){for(const auto &f:engine_fields)flag(n,f.first,o.*f.second);}
EngineOptions read_engine(Node *n,bool with_id=false){std::set<std::string> keys;if(with_id)keys.insert("id");for(const auto &f:engine_fields)keys.insert(f.first);attributes(n,std::move(keys));leaf(n);EngineOptions o;for(const auto &f:engine_fields)o.*f.second=boolean(n,f.first);return o;}
void write_location(Node *n,const Location &l){attr(n,"module",l.module);attr(n,"reference",l.reference);attr(n,"commentary",l.commentary);}
Location read_location(Node *n){attributes(n,{"module","reference","commentary"});leaf(n);return {required(n,"reference"),required(n,"module"),required(n,"commentary")};}
void write_dictionary(Node *n,const DictionaryLocation &l){attr(n,"module",l.module);attr(n,"key",l.key);}
DictionaryLocation read_dictionary(Node *n){attributes(n,{"module","key"});leaf(n);return {required(n,"module"),required(n,"key")};}
void write_presentation(Node *n,const ModulePresentationOptions &o){
 flag(n,"chapter",o.commentary_by_chapter);flag(n,"headings",o.headings);flag(n,"numbers",o.note_numbers);flag(n,"double_space",o.double_space);
 attr(n,"images",std::to_string(static_cast<int>(o.image_content)));attr(n,"fonts",std::to_string(static_cast<int>(o.respect_font_faces)));
}
ContentPreference content(Node *n,const char *key){auto v=required(n,key);if(v=="-1")return ContentPreference::unknown;if(v=="0")return ContentPreference::off;if(v=="1")return ContentPreference::on;invalid();return ContentPreference::unknown;}
ModulePresentationOptions read_presentation(Node *n){
 attributes(n,{"id","chapter","headings","numbers","double_space","images","fonts"});leaf(n);ModulePresentationOptions o;
 o.commentary_by_chapter=boolean(n,"chapter");o.headings=boolean(n,"headings");o.note_numbers=boolean(n,"numbers");o.double_space=boolean(n,"double_space");
 o.image_content=content(n,"images");o.respect_font_faces=content(n,"fonts");return o;
}
void write_state(Node *n,const StudySessionState &s){
 flag(n,"module_defaults",s.module_defaults);flag(n,"xref_list",s.cross_reference_preview_list);number(n,"retained_elements",s.search_retention.max_elements);number(n,"retained_bytes",s.search_retention.max_key_bytes);
 auto *h=child(n,"reading-history");number(h,"cursor",s.history_cursor);for(const auto &l:s.history)write_location(child(h,"location"),l);
 h=child(n,"dictionary-history");number(h,"cursor",s.dictionary_cursor);for(const auto &l:s.dictionary_history)write_dictionary(child(h,"location"),l);
 if(s.reading)write_location(child(n,"reading"),*s.reading);if(s.dictionary)write_dictionary(child(n,"dictionary"),*s.dictionary);
 if(s.engine)write_engine(child(n,"engine"),*s.engine);
 for(const auto &m:s.module_engine){auto *p=child(n,"module-engine");attr(p,"id",m.first);write_engine(p,m.second);}
 for(const auto &m:s.presentation){auto *p=child(n,"presentation");attr(p,"id",m.first);write_presentation(p,m.second);}
 if(s.lexicons){auto *p=child(n,"lexicons");for(const auto &f:lexicon_fields)if(s.lexicons.value().*f.second)attr(p,f.first,*(s.lexicons.value().*f.second));}
 if(s.search_text){auto *p=child(n,"search-text");flag(p,"strongs",s.search_text->strongs);flag(p,"morphology",s.search_text->morphology);flag(p,"footnotes",s.search_text->footnotes);}
 if(s.book){auto *p=child(n,"book");attr(p,"module",s.book->requested.module);flag(p,"document",s.book->document);number(p,"offset",s.book->offset);attr(p,"path",s.book->path);
  if(const auto *v=std::get_if<std::string>(&s.book->requested.address)){attr(p,"address_kind","path");attr(p,"address",*v);}else{attr(p,"address_kind","offset");number(p,"address",std::get<std::uint64_t>(s.book->requested.address));}}
 if(s.parallel){auto *p=child(n,"parallel");attr(p,"control",s.parallel->control_module);attr(p,"reference",s.parallel->reference);flag(p,"headings",s.parallel->headings);number(p,"max_bytes",s.parallel->max_bytes);write_engine(child(p,"engine"),s.parallel->options);for(const auto &m:s.parallel->modules)attr(child(p,"column"),"module",m);}
}
StudySessionState read_state(Node *n){
 attributes(n,{"module_defaults","xref_list","retained_elements","retained_bytes"});StudySessionState s;
 s.module_defaults=boolean(n,"module_defaults");s.cross_reference_preview_list=boolean(n,"xref_list");
 s.search_retention.max_elements=integer(n,"retained_elements",maximum_search_retained_elements);s.search_retention.max_key_bytes=integer(n,"retained_bytes",maximum_search_retained_key_bytes);
 std::set<std::string> seen;
 for(auto *p:children(n,{"reading-history","dictionary-history","reading","dictionary","engine","module-engine","presentation","lexicons","search-text","book","parallel"})){
  const auto tag=name(p);if(tag!="module-engine"&&tag!="presentation"&&!seen.insert(tag).second)invalid();
  if(tag=="reading-history"||tag=="dictionary-history"){
   attributes(p,{"cursor"});auto at=integer(p,"cursor",49);auto rows=children(p,{"location"});if(rows.size()>50)invalid();
   if(tag=="reading-history"){s.history_cursor=at;for(auto *row:rows)s.history.push_back(read_location(row));}
   else{s.dictionary_cursor=at;for(auto *row:rows)s.dictionary_history.push_back(read_dictionary(row));}
  }else if(tag=="reading")s.reading=read_location(p);
  else if(tag=="dictionary")s.dictionary=read_dictionary(p);
  else if(tag=="engine")s.engine=read_engine(p);
  else if(tag=="module-engine"){if(s.module_engine.size()>=maximum_session_modules)invalid();auto id=required(p,"id");if(!s.module_engine.emplace(id,read_engine(p,true)).second)invalid();}
  else if(tag=="presentation"){if(s.presentation.size()>=maximum_session_modules)invalid();auto id=required(p,"id");if(!s.presentation.emplace(id,read_presentation(p)).second)invalid();}
  else if(tag=="lexicons"){std::set<std::string> keys;for(const auto &f:lexicon_fields)keys.insert(f.first);attributes(p,std::move(keys));leaf(p);LexiconPreferences l;for(const auto &f:lexicon_fields)l.*f.second=optional(p,f.first);s.lexicons=std::move(l);}
  else if(tag=="search-text"){attributes(p,{"strongs","morphology","footnotes"});leaf(p);s.search_text=SearchTextOptions{boolean(p,"strongs"),boolean(p,"morphology"),boolean(p,"footnotes")};}
  else if(tag=="book"){
   attributes(p,{"module","document","offset","path","address_kind","address"});leaf(p);SessionBookState b;b.requested.module=required(p,"module");b.offset=integer(p,"offset",UINT64_MAX);b.path=required(p,"path");b.document=boolean(p,"document");auto kind=required(p,"address_kind");if(kind=="path")b.requested.address=required(p,"address");else if(kind=="offset")b.requested.address=integer(p,"address",UINT64_MAX);else invalid();s.book=std::move(b);
  }else if(tag=="parallel"){
   attributes(p,{"control","reference","headings","max_bytes"});ParallelSelection parallel;
   parallel.control_module=required(p,"control");parallel.reference=required(p,"reference");parallel.headings=boolean(p,"headings");parallel.max_bytes=integer(p,"max_bytes",maximum_parallel_snapshot_bytes);bool engine=false;
   for(auto *q:children(p,{"engine","column"}))if(name(q)=="engine"){if(engine)invalid();engine=true;parallel.options=read_engine(q);}else{attributes(q,{"module"});leaf(q);if(parallel.modules.size()>=maximum_parallel_columns)invalid();parallel.modules.push_back(required(q,"module"));}
   if(!engine)invalid();s.parallel=std::move(parallel);
  }
 }
 if(!seen.count("reading-history")||!seen.count("dictionary-history"))invalid();validate_session_state(s);return s;
}
}
std::string serialize_native_session(const NativeSessionSnapshot &s){
 validate_native_session(s);detail::XmlDocument doc(xmlNewDoc(BAD_CAST "1.0"),xmlFreeDoc);if(!doc)throw std::bad_alloc();auto *root=xmlNewNode(nullptr,BAD_CAST "XiphosSession");if(!root)throw std::bad_alloc();xmlDocSetRootElement(doc.get(),root);number(root,"version",s.version);if(s.active_key)attr(root,"active",*s.active_key);
 for(const auto &tab:s.tabs){auto *n=child(root,"tab");attr(n,"key",tab.key);flag(n,"texts",tab.visibility.texts);flag(n,"preview",tab.visibility.preview);flag(n,"commentaries",tab.visibility.commentaries);flag(n,"dictionaries",tab.visibility.dictionaries);flag(n,"commentary_showing",tab.commentary_showing);flag(n,"parallel_showing",tab.parallel_showing);if(!tab.host_state.empty())attr(n,"host",tab.host_state);write_state(child(n,"state"),tab.state);}
 xmlChar *buffer=nullptr;int length=0;xmlDocDumpFormatMemoryEnc(doc.get(),&buffer,&length,"UTF-8",1);std::unique_ptr<xmlChar,decltype(xmlFree)> owned(buffer,xmlFree);if(!buffer||length<=0)throw std::runtime_error("Native session serialization failed");if(static_cast<std::size_t>(length)>maximum_session_bytes)throw std::invalid_argument("Native session file exceeds byte limit");return {reinterpret_cast<const char *>(buffer),static_cast<std::size_t>(length)};
}
NativeSessionSnapshot parse_native_session(const std::string &xml){
 auto doc=detail::parse_xml_document(xml,"XiphosSession");auto *root=xmlDocGetRootElement(doc.get());attributes(root,{"version","active"});NativeSessionSnapshot s;s.version=integer(root,"version",1);s.active_key=optional(root,"active");
 for(auto *n:children(root,{"tab"})){
  if(s.tabs.size()>=maximum_saved_tabs)invalid();attributes(n,{"key","texts","preview","commentaries","dictionaries","commentary_showing","parallel_showing","host"});NativeSessionTab tab;tab.key=required(n,"key");tab.visibility={boolean(n,"texts"),boolean(n,"preview"),boolean(n,"commentaries"),boolean(n,"dictionaries")};tab.commentary_showing=boolean(n,"commentary_showing");tab.parallel_showing=boolean(n,"parallel_showing");tab.host_state=optional(n,"host").value_or("");auto states=children(n,{"state"});if(states.size()!=1)invalid();tab.state=read_state(states.front());s.tabs.push_back(std::move(tab));
 }validate_native_session(s);return s;
}
NativeSessionSnapshot read_native_session(const std::string &path){return parse_native_session(detail::read_preference_file(std::filesystem::u8path(path),maximum_session_bytes));}
NativeSessionSnapshot save_native_session(const std::string &path,const NativeSessionSnapshot &s){
 const auto xml=serialize_native_session(s);auto verified=parse_native_session(xml);if(serialize_native_session(verified)!=xml)throw std::runtime_error("Native session round-trip differs");detail::replace_preference_file(path,[&](const std::filesystem::path &temp,const std::string &original,bool existed){
  // A native snapshot owns its strict versioned document. Refuse unrelated XML
  // or future extensions rather than silently erasing fields we cannot retain.
  if(existed)(void)parse_native_session(original);
  std::ofstream output(temp,std::ios::binary);output.exceptions(std::ios::badbit|std::ios::failbit);output.write(xml.data(),static_cast<std::streamsize>(xml.size()));output.close();
  if(detail::read_preference_file(temp,maximum_session_bytes)!=xml)throw std::runtime_error("Staged native session bytes differ");
 },maximum_session_bytes);return verified;
}
}
