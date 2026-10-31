// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "xml_document.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
namespace xiphos::core {
namespace {
struct Bounds {
  std::size_t bytes=0;
  void text(const std::string &s,std::size_t limit=32768,bool required=false) {
    if(required&&s.empty())throw std::invalid_argument("Required session field is empty");
    detail::validate_xml_text(s,limit);
    if(s.size()>maximum_session_bytes-bytes)throw std::invalid_argument("Session text exceeds byte limit");
    bytes+=s.size();
  }
  void module(const std::string &s,bool required=true){text(s,4096,required);}
  void location(const Location &l){module(l.module);module(l.commentary,false);text(l.reference,32768,true);}
  void dictionary(const DictionaryLocation &l){module(l.module);text(l.key);}
  void state(const StudySessionState &s) {
    if(s.history.size()>50||s.dictionary_history.size()>50||s.module_engine.size()>maximum_session_modules||s.presentation.size()>maximum_session_modules)
      throw std::invalid_argument("Session collection exceeds bounds");
    auto cursor=[](std::size_t count,std::size_t at){if((!count&&at)||(count&&at>=count))throw std::invalid_argument("Session cursor is invalid");};
    cursor(s.history.size(),s.history_cursor);cursor(s.dictionary_history.size(),s.dictionary_cursor);
    if(s.reading)location(*s.reading);if(s.dictionary)dictionary(*s.dictionary);
    std::set<std::string> references;
    for(const auto &l:s.history){location(l);if(!references.insert(l.reference).second)throw std::invalid_argument("Duplicate reading history reference");}
    for(const auto &l:s.dictionary_history)dictionary(l);
    if(s.book){module(s.book->requested.module);text(s.book->path);if(const auto *p=std::get_if<std::string>(&s.book->requested.address))text(*p);}
    if(s.parallel){const auto &p=*s.parallel;if(!parallel_read_arguments(p.modules,p.reference,p.max_bytes)||p.modules.empty())throw std::invalid_argument("Invalid saved parallel selection");module(p.control_module);text(p.reference,32768,true);for(const auto &m:p.modules)module(m,false);}
    for(const auto &m:s.module_engine)module(m.first);
    for(const auto &m:s.presentation){module(m.first);auto preference=[](ContentPreference p){return p==ContentPreference::unknown||p==ContentPreference::off||p==ContentPreference::on;};if(!preference(m.second.image_content)||!preference(m.second.respect_font_faces))throw std::invalid_argument("Invalid presentation content preference");}
    if(s.lexicons)for(const auto *v:{&s.lexicons->greek,&s.lexicons->hebrew,&s.lexicons->greek_morph_nt,&s.lexicons->greek_morph_ot,&s.lexicons->hebrew_morph})if(*v)module(**v,false);
    if(s.search_retention.max_elements>maximum_search_retained_elements||s.search_retention.max_key_bytes>maximum_search_retained_key_bytes)
      throw std::invalid_argument("Invalid saved search retention limits");
  }
};
}
void validate_session_state(const StudySessionState &state){Bounds{}.state(state);}
void validate_native_session(const NativeSessionSnapshot &s){
  if(s.version!=1||s.tabs.size()>maximum_saved_tabs)throw std::invalid_argument("Unsupported session version or tab count");
  Bounds bounds;std::set<std::string> keys;
  for(const auto &tab:s.tabs){bounds.text(tab.key,4096,true);if(!keys.insert(tab.key).second)throw std::invalid_argument("Duplicate session tab key");bounds.state(tab.state);bounds.text(tab.host_state);}
  if(s.active_key){bounds.text(*s.active_key,4096,true);if(!keys.count(*s.active_key))throw std::invalid_argument("Active session tab is absent");}
}
StudySessionState StudySession::capture_state() const {
  StudySessionState s;s.reading=reading_selection_;s.dictionary=dictionary_selection_;s.history=history_.entries_;s.history_cursor=history_.cursor_;
  s.dictionary_history=dictionary_history_.entries_;s.dictionary_cursor=dictionary_history_.cursor_;
  if(book_selection_)s.book=SessionBookState{book_selection_->requested,book_selection_->tree.node.offset,book_selection_->tree.node.path,book_selection_->document.has_value()};
  s.parallel=parallel_selection_;s.engine=options_;s.module_engine=module_options_;s.presentation=presentation_options_;
  // Availability is recomputed from the destination SDK and host capabilities.
  for(auto &m:s.presentation){m.second.note_numbering_available=false;m.second.commentary_chapter_available=false;}
  s.module_defaults=module_preferences_;s.cross_reference_preview_list=xrefs_in_verse_list_;s.lexicons=lexicons_;
  s.search_text=search_text_options_;s.search_retention=search_retention_limits_;validate_session_state(s);return s;
}
std::unique_ptr<StudySession> StudySession::from_state(Reader &reader,const StudySessionState &s,SessionCapabilities capabilities){
  validate_session_state(s);auto result=std::make_unique<StudySession>(reader);
  result->options_=s.engine;result->module_options_=s.module_engine;result->presentation_options_=s.presentation;
  for(auto &m:result->presentation_options_){m.second.note_numbering_available=false;m.second.commentary_chapter_available=false;}
  result->module_preferences_=s.module_defaults;result->lexicons_=s.lexicons;result->xrefs_in_verse_list_=s.cross_reference_preview_list;
  result->lexical_layout_enabled_=capabilities.lexical_layout;result->note_numbering_enabled_=capabilities.note_numbering;result->heading_note_links_enabled_=capabilities.heading_note_links;
  result->search_text_options_=s.search_text;result->search_retention_limits_=s.search_retention;
  // Validate exact installed identities, not aliases or SDK fallback modules.
  auto reading=[&](const Location &l){result->require_module(l.module);if(!l.commentary.empty()){
    const auto it=std::find_if(reader.modules().begin(),reader.modules().end(),[&](const Module &m){return m.id==l.commentary;});
    if(it==reader.modules().end()||it->type!="Commentaries")throw std::invalid_argument("Saved commentary is unavailable");
  }(void)reader.navigation_target(l.module,l.reference);};
  auto dictionary=[&](const DictionaryLocation &l){const auto it=std::find_if(reader.modules().begin(),reader.modules().end(),[&](const Module &m){return m.id==l.module;});if(it==reader.modules().end()||!it->dictionary)throw std::invalid_argument("Saved dictionary is unavailable");};
  for(const auto &l:s.history)reading(l);for(const auto &l:s.dictionary_history)dictionary(l);
  if(s.reading){reading(*s.reading);if(s.reading->commentary.empty())(void)result->read_entry(s.reading->module,s.reading->reference);else (void)result->read_passage(s.reading->module,s.reading->commentary,s.reading->reference);}
  if(s.dictionary){dictionary(*s.dictionary);(void)result->read_dictionary(s.dictionary->module,s.dictionary->key);}
  if(s.parallel){auto chapter=result->read_parallel(s.parallel->modules,s.parallel->reference,s.parallel->options,s.parallel->headings,s.parallel->max_bytes);if(chapter.control_module!=s.parallel->control_module||chapter.reference!=s.parallel->reference)throw std::invalid_argument("Saved parallel control/reference no longer resolves identically");}
  if(s.book){const auto resolved=reader.resolve_book_target(book_reading_target(s.book->requested));if(resolved.node.offset!=s.book->offset||resolved.node.path!=s.book->path)throw std::invalid_argument("Saved book address no longer resolves identically");if(s.book->document)(void)result->select_book_document(s.book->requested);else (void)result->select_book(s.book->requested);}
  // Render validation may discover presentation hints. Persisted explicit state
  // wins, and histories/current intent are published without replay/dedup.
  result->presentation_options_=s.presentation;for(auto &m:result->presentation_options_){
    m.second.note_numbering_available=false;
    const auto installed=std::find_if(reader.modules().begin(),reader.modules().end(),[&](const Module &module){return module.id==m.first;});
    m.second.commentary_chapter_available=installed!=reader.modules().end()&&installed->regular_commentary;
  }
  result->reading_selection_=s.reading;result->dictionary_selection_=s.dictionary;result->parallel_selection_=s.parallel;
  result->history_.entries_=s.history;result->history_.cursor_=s.history_cursor;result->dictionary_history_.entries_=s.dictionary_history;result->dictionary_history_.cursor_=s.dictionary_cursor;
  return result;
}
}
