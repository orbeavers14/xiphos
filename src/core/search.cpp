// SPDX-License-Identifier: GPL-2.0-or-later
#include "search_source.hpp"
#include "book_tree_source.hpp"
#include "search_control.hpp"
#include "search_results.hpp"
#include "search_generation.hpp"
#include "search_validation.hpp"
#include "sdk_text.hpp"
#include "sdk_render.hpp"
#include "xiphos/core/search_policy.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/presentation.hpp"
#include "sdk_search.hpp"
#include <swoptfilter.h>
#include <versekey.h>
#include <swld.h>
#include <swgenbook.h>
#include <listkey.h>
#include <stdexcept>
#include <thread>
#include <algorithm>
#include <set>
namespace xiphos::core::detail {
namespace {
class ScopedSearchFilters {
  struct Setting {sword::SWOptionFilter *filter;std::string prior,desired;};
  std::vector<Setting> saved_;
public:
  explicit ScopedSearchFilters(EngineManager &manager,const std::optional<SearchTextOptions> &text, bool surface) {
    for(auto *filter:manager.option_filters()) {
      const std::string name=filter->getOptionName();const char *desired=nullptr;
      if(name=="Greek Accents"||name=="Hebrew Vowel Points"||name=="Arabic Vowel Points")
        desired=surface?"On":"Off";
      else if(surface&&name=="Hebrew Cantillation")desired="On";
      else if(text){for(const auto &setting:search_text_settings(*text))if(name==setting.name)desired=setting.enabled?"On":"Off";}
      if(!desired)continue;
      const auto *value=filter->getOptionValue();
      if(!value)throw std::runtime_error("Search filter has no restorable value");
      saved_.push_back({filter,value,desired});
    }
    try {for(auto &value:saved_)value.filter->setOptionValue(value.desired.c_str());}
    catch(...){restore();throw;}
  }
  ~ScopedSearchFilters(){restore();}
  void restore(){for(auto &value:saved_)value.filter->setOptionValue(value.prior.c_str());}
};
struct ScopedSearchModule {
  sword::SWModule &module;bool attributes,skip_links;
  explicit ScopedSearchModule(sword::SWModule &m):module(m),attributes(m.isProcessEntryAttributes()),skip_links(m.isSkipConsecutiveLinks()){}
  ~ScopedSearchModule(){module.setProcessEntryAttributes(attributes);module.setSkipConsecutiveLinks(skip_links);module.terminateSearch=false;}
};
std::string surface_key(const std::string &text, bool unmarked, bool sensitive) {
  using OwnedText = std::unique_ptr<char, decltype(&g_free)>;
  OwnedText folded(sensitive ? g_strdup(text.c_str()) : g_utf8_casefold(text.data(),text.size()), &g_free);
  if (!folded) throw std::runtime_error("Unable to case-fold surface text");
  OwnedText normalized(g_utf8_normalize(folded.get(),-1,G_NORMALIZE_NFD), &g_free);
  if (!normalized) throw std::runtime_error("Unable to normalize surface text");
  if (!unmarked) return normalized.get();
  std::string result;
  GUnicodeScript base = G_UNICODE_SCRIPT_UNKNOWN;
  for (const char *at = normalized.get(); *at; at = g_utf8_next_char(at)) {
    const auto scalar = g_utf8_get_char(at);
    if (g_unichar_ismark(scalar)) {
      if (base == G_UNICODE_SCRIPT_GREEK || base == G_UNICODE_SCRIPT_HEBREW ||
          base == G_UNICODE_SCRIPT_SYRIAC) continue;
    } else {
      base = g_unichar_get_script(scalar);
    }
    result.append(at,g_utf8_next_char(at));
  }
  return result;
}
bool contains_surface(const std::string &text,const std::string &needle) {
  for (auto at = text.find(needle); at != std::string::npos; at = text.find(needle,at+1)) {
    const auto end = at + needle.size();
    // A literal query ending in a base letter cannot match a prefix of a
    // marked grapheme, including Hebrew marks without precomposed forms.
    if (end == text.size() || !g_unichar_ismark(g_utf8_get_char(text.data()+end))) return true;
  }
  return false;
}
sword::ListKey scan_surface(sword::SWModule &module, const sword::ListKey *range,
    const std::string &needle, bool unmarked, bool sensitive,
    const std::shared_ptr<SearchControl> &control, unsigned begin, unsigned end) {
  sword::ListKey matches;
  std::unique_ptr<sword::SWKey> scope(range ? range->clone() : nullptr);
  auto *original = module.getKey();
  std::unique_ptr<sword::SWKey> saved(original->isPersist() ? nullptr : original->clone());
  struct Restore {
    sword::SWModule &module;
    const sword::SWKey &key;
    ~Restore() { module.setKey(key); }
  } restore{module, saved ? *saved : *original};
  if (scope) {
    scope->setPersist(true);
    module.setKey(*scope);
  } else if (original->isPersist()) {
    scope.reset(original->clone());
    scope->setPersist(true);
    module.setKey(*scope);
  }
  module.setProcessEntryAttributes(false);
  module.setPosition(sword::BOTTOM);
  const auto last = module.getKey()->getIndex();
  module.setPosition(sword::TOP);
  std::unique_ptr<sword::SWKey> match(module.createKey());
  auto *verse = dynamic_cast<sword::VerseKey *>(match.get());
  if (!verse) throw std::runtime_error("Surface search requires a verse key");
  std::size_t count = 0, bytes = 0;
  while (!module.popError() && !(control && control->cancel_requested())) {
    if (++count > maximum_search_retained_elements) throw std::length_error("Surface verse traversal limit");
    auto plain = copy_sdk_text(strip_sdk_buffer(module,nullptr),1024*1024);
    if (plain.size() > 128*1024*1024 - bytes) throw std::length_error("Surface text traversal limit");
    bytes += plain.size();
    if (contains_surface(surface_key(plain,unmarked,sensitive),needle)) {
      verse->positionFrom(*module.getKey());
      verse->clearBounds();
      matches.add(*verse);
    }
    if (control && last > 0) {
      const auto index = std::max(0L,std::min(last,module.getKey()->getIndex()));
      SearchControlAccess::progress(*control,begin + static_cast<unsigned>(index*(end-begin)/last));
    }
    module++;
  }
  return matches;
}
struct ProgressContext {sword::SWModule *module;SearchControl *control;std::thread::id owner;unsigned begin,end;};
void search_percent(char percent,void *user) noexcept {
  auto &context=*static_cast<ProgressContext *>(user);
  if(!context.control)return;
  const auto value=std::min(100u,static_cast<unsigned>(static_cast<unsigned char>(percent)));
  SearchControlAccess::progress(*context.control,context.begin+(context.end-context.begin)*value/100);
  // Only the owning engine callback touches the SDK's non-atomic flag.
  if(std::this_thread::get_id()==context.owner&&context.control->cancel_requested())context.module->terminateSearch=true;
}
}
bool supports_verse_search(sword::SWModule &module) {
  const std::string type=module.getType();
  return dynamic_cast<sword::VerseKey *>(module.getKey())&&
    (type=="Biblical Texts"||regular_commentary(type.c_str(),module.getConfigEntry("ModDrv")));
}
SearchResult capture_module_search(EngineManager &manager,sword::SWModule &module,
                                  SearchQuery query,const SearchLimits &limits,const std::shared_ptr<SearchControl> &control,
                                  const SearchScope *previous,std::shared_ptr<const SearchScope> *next,
                                  const SearchScopeLimits &scope_limits,const std::optional<SearchTextOptions> &previous_text,
                                  unsigned progress_begin,unsigned progress_end,bool allow_empty_output,
                                  SearchRetainedKeys *retained,SearchRetentionLimits retention_limits,bool keyed_only) {
  if(progress_begin>progress_end||progress_end>100)throw std::invalid_argument("Invalid search progress interval");
  if(next)next->reset();
  if(retained)*retained={};
  if(query.within_last_results&&(query.range||(!keyed_only&&!next)||(keyed_only&&!previous)))
    throw std::invalid_argument("Last-search scope requires a session and excludes explicit range");
  validate_search_query(query);
  if((!allow_empty_output&&(!limits.max_results||!limits.max_reference_bytes))||limits.max_results>maximum_search_results||
     limits.max_reference_bytes>maximum_search_reference_bytes)
    throw std::invalid_argument("Unsupported search output limits");
  if(keyed_only){
    const bool dictionary=dynamic_cast<sword::SWLD *>(&module)&&std::string(module.getType())=="Lexicons / Dictionaries";
    const bool book=dynamic_cast<sword::SWGenBook *>(&module)&&indexed_book(module);
    if((!dictionary&&!book)||query.range||next||
       (query.mode!=SearchMode::regular_expression&&query.mode!=SearchMode::phrase&&query.mode!=SearchMode::words&&query.mode!=SearchMode::multiword&&query.mode!=SearchMode::indexed&&query.mode!=SearchMode::lemma&&query.mode!=SearchMode::morphology&&query.mode!=SearchMode::footnote))
      throw std::invalid_argument("Keyed search requires a dictionary/general book and a supported search mode");
  }else if(query.keyed_interval||!supports_verse_search(module))throw std::invalid_argument("Initial search supports Bible and regular commentary verse modules");
  int engine_type;
  bool whole_entry=false;
  std::optional<AttributeSearch> attribute;
  switch(query.mode){
    case SearchMode::regular_expression:engine_type=xiphos::core::detail::sdk_search_regex;break;
    case SearchMode::surface_literal:
    case SearchMode::surface_unmarked:
    case SearchMode::phrase:engine_type=xiphos::core::detail::sdk_search_phrase;break;
    case SearchMode::words:
    case SearchMode::multiword:
    case SearchMode::indexed:engine_type=xiphos::core::detail::sdk_search_multiword;break;
    case SearchMode::lemma:attribute=AttributeSearch::lemma;engine_type=xiphos::core::detail::sdk_search_attribute;break;
    case SearchMode::morphology:attribute=AttributeSearch::morphology;engine_type=xiphos::core::detail::sdk_search_attribute;break;
    case SearchMode::footnote:attribute=AttributeSearch::footnote;engine_type=xiphos::core::detail::sdk_search_attribute;break;
    default:throw std::invalid_argument("Unknown search mode");
  }
  std::optional<sword::ListKey> range;
  if(query.range){
    const auto &requested=*query.range;
    auto *source=manager.getModule(requested.module.c_str());
    if(!source)throw std::out_of_range("Search range module is not installed");
    auto *key=dynamic_cast<sword::VerseKey *>(source->getKey());
    if(!key)throw std::invalid_argument("Search range source requires a verse-key module");
    sword::VerseKey parser(*key);
    parser.setText(source->getKeyText());
    range=parser.parseVerseList(requested.expression.c_str(),"",true);
    if(range->getCount()<0||range->getCount()>64)throw std::invalid_argument("Search range exceeds the interval limit");
  }
  if(query.within_last_results&&previous)range=previous->keys();
  if(control&&control->cancel_requested()) {
    SearchResult result{std::move(query),{},engine_type,0,{},false};result.cancelled=true;return result;
  }
  const bool surface = surface_search_mode(query.mode);
  const bool unmarked = query.mode == SearchMode::surface_unmarked;
  const auto effective_text=query.mode!=SearchMode::phrase&&!surface&&query.text_options?query.text_options:previous_text;
  ScopedSearchFilters filters(manager,effective_text,surface);ScopedSearchModule state(module);
  std::optional<sword::ListKey> interval;
  if(query.keyed_interval){
    // Enumerate real SDK order, not lexical/verse guesses. Never fuzzy-resolve
    // endpoints; indexed engines are post-filtered against these exact keys.
    const std::unique_ptr<sword::SWKey> saved(module.getKey()->clone());
    struct Restore {sword::SWModule &module;const sword::SWKey &key;~Restore(){module.setKey(key);}} restore{module,*saved};
    interval.emplace();bool found=false,finished=false;std::size_t count=0,bytes=0;
    module.setPosition(sword::TOP);
    while(!module.popError()){
      if(control&&control->cancel_requested())break;
      if(++count>maximum_search_retained_elements)throw std::length_error("Keyed interval traversal limit");
      const auto key=copy_sdk_text(sword::SWBuf(module.getKeyText()),4096);
      if(key.size()+1>maximum_search_retained_key_bytes-bytes)throw std::length_error("Keyed interval byte limit");
      bytes+=key.size()+1;
      if(key==query.keyed_interval->first)found=true;
      if(found)interval->add(*module.getKey());
      if(key==query.keyed_interval->last){finished=found;break;}
      module++;
    }
    if(!(control&&control->cancel_requested())&&(!found||!finished))throw std::invalid_argument("Keyed interval endpoints must exist in forward module order");
    range=*interval;
  }
  // Capability probing must see this request's search-only filter context too.
  if((query.mode==SearchMode::words||query.mode==SearchMode::indexed)&&supports_indexed_search(module))
    engine_type=xiphos::core::detail::sdk_search_external;
  const bool index_fallback=query.mode==SearchMode::indexed&&engine_type!=xiphos::core::detail::sdk_search_external;
  auto prepared=sidebar_search_text(query.expression,query.mode==SearchMode::words&&engine_type==xiphos::core::detail::sdk_search_external);
  if(attribute){auto value=attribute_search_text(query.expression,*attribute);prepared=std::move(value.expression);whole_entry=value.whole_entry;}
  const auto stripped_buffer=strip_sdk_buffer(module,prepared.c_str());
  const auto stripped=copy_sdk_text(stripped_buffer,maximum_search_effective_bytes);
  const auto needle = surface ? surface_key(stripped,unmarked,query.case_sensitive) : std::string{};
  if (surface && needle.empty()) throw std::invalid_argument("Surface search has no searchable text");
  if(control&&control->cancel_requested()) {
    SearchResult result{std::move(query),stripped,engine_type,0,{},false};result.cancelled=true;result.effective_text_options=effective_text;result.index_fallback=index_fallback;return result;
  }
  if(query.within_last_results&&(!previous||!previous->info().element_count)){
    SearchResult result{std::move(query),stripped,engine_type,0,{},false};result.effective_text_options=effective_text;result.index_fallback=index_fallback;return result;
  }
  ProgressContext context{&module,control.get(),std::this_thread::get_id(),progress_begin,progress_end};
  const int flags=(query.case_sensitive?0:XIPHOS_SWORD_SEARCH_ICASE_FLAG)|(whole_entry?xiphos::core::detail::sdk_search_whole_entry:0);
  sword::ListKey surface_matches;
  if (surface) surface_matches = scan_surface(module,range?&*range:nullptr,needle,unmarked,
      query.case_sensitive,control,progress_begin,progress_end);
  auto &sdk_list=surface ? surface_matches : module.search(stripped.c_str(),engine_type,flags,
                           range?&*range:nullptr,nullptr,search_percent,&context);
  // Some SDK indexed tree searches ignore arbitrary-key ranges. Enforce the
  // authoritative same-source scope before counts, sorting, retention or export.
  std::optional<sword::ListKey> filtered;
  if(keyed_only&&(interval||(query.within_last_results&&previous))){
    std::set<std::string> allowed;
    const auto &scope_keys=interval?*interval:previous->keys();
    for(int i=0;i<scope_keys.getCount();++i){
      const auto *key=scope_keys.getElement(i);if(!key)throw std::runtime_error("Missing scoped key");
      allowed.emplace(key->getText());
    }
    filtered.emplace();
    for(int i=0;i<sdk_list.getCount();++i){
      if(control&&control->cancel_requested())break;
      const auto *key=sdk_list.getElement(i);if(!key)throw std::runtime_error("Missing SDK result key");
      if(allowed.erase(key->getText()))filtered->add(*key);
    }
  }
  auto &list=filtered?*filtered:sdk_list;
  // Legacy copies the complete scope before sorting indexed display results.
  auto staged=next?std::make_shared<const SearchScope>(list,scope_limits):nullptr;
  if(engine_type==xiphos::core::detail::sdk_search_external)list.sort();
  SearchRetainedKeys staged_retained;
  if(retained)staged_retained=retain_search_keys(list,
    engine_type==xiphos::core::detail::sdk_search_external?nullptr:staged,retention_limits);
  auto copied=copy_search_references(list,limits,allow_empty_output);
  SearchResult result{std::move(query),stripped,engine_type,copied.engine_count,std::move(copied.references),copied.limited};
  result.executed=true;result.cancelled=control&&control->cancel_requested();
  result.effective_text_options=effective_text;result.index_fallback=index_fallback;
  if(retained)*retained=std::move(staged_retained);
  if(next)*next=std::move(staged); // Publish only after all owned output is ready.
  return result;
}
}
