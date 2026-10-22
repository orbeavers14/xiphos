// SPDX-License-Identifier: GPL-2.0-or-later
#include "search_source.hpp"
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
#include <listkey.h>
#include <stdexcept>
#include <thread>
#include <algorithm>
namespace xiphos::core::detail {
namespace {
class ScopedSearchFilters {
  struct Setting {sword::SWOptionFilter *filter;std::string prior,desired;};
  std::vector<Setting> saved_;
public:
  explicit ScopedSearchFilters(EngineManager &manager,const std::optional<SearchTextOptions> &text) {
    for(auto *filter:manager.option_filters()) {
      const std::string name=filter->getOptionName();const char *desired=nullptr;
      if(name=="Greek Accents"||name=="Hebrew Vowel Points"||name=="Arabic Vowel Points")desired="Off";
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
                                  SearchRetainedKeys *retained,SearchRetentionLimits retention_limits) {
  if(progress_begin>progress_end||progress_end>100)throw std::invalid_argument("Invalid search progress interval");
  if(next)next->reset();
  if(retained)*retained={};
  if(query.within_last_results&&(!next||query.range))
    throw std::invalid_argument("Last-search scope requires a session and excludes explicit range");
  validate_search_query(query);
  if((!allow_empty_output&&(!limits.max_results||!limits.max_reference_bytes))||limits.max_results>maximum_search_results||
     limits.max_reference_bytes>maximum_search_reference_bytes)
    throw std::invalid_argument("Unsupported search output limits");
  if(!supports_verse_search(module))
    throw std::invalid_argument("Initial search supports Bible and regular commentary verse modules");
  int engine_type;
  bool whole_entry=false;
  std::optional<AttributeSearch> attribute;
  switch(query.mode){
    case SearchMode::regular_expression:engine_type=xiphos::core::detail::sdk_search_regex;break;
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
  const auto effective_text=query.mode!=SearchMode::phrase&&query.text_options?query.text_options:previous_text;
  ScopedSearchFilters filters(manager,effective_text);ScopedSearchModule state(module);
  // Capability probing must see this request's search-only filter context too.
  if((query.mode==SearchMode::words||query.mode==SearchMode::indexed)&&supports_indexed_search(module))
    engine_type=xiphos::core::detail::sdk_search_external;
  const bool index_fallback=query.mode==SearchMode::indexed&&engine_type!=xiphos::core::detail::sdk_search_external;
  auto prepared=sidebar_search_text(query.expression,query.mode==SearchMode::words&&engine_type==xiphos::core::detail::sdk_search_external);
  if(attribute){auto value=attribute_search_text(query.expression,*attribute);prepared=std::move(value.expression);whole_entry=value.whole_entry;}
  const auto stripped_buffer=strip_sdk_buffer(module,prepared.c_str());
  const auto stripped=copy_sdk_text(stripped_buffer,maximum_search_effective_bytes);
  if(control&&control->cancel_requested()) {
    SearchResult result{std::move(query),stripped,engine_type,0,{},false};result.cancelled=true;result.effective_text_options=effective_text;result.index_fallback=index_fallback;return result;
  }
  if(query.within_last_results&&(!previous||!previous->info().element_count)){
    SearchResult result{std::move(query),stripped,engine_type,0,{},false};result.effective_text_options=effective_text;result.index_fallback=index_fallback;return result;
  }
  ProgressContext context{&module,control.get(),std::this_thread::get_id(),progress_begin,progress_end};
  const int flags=(query.case_sensitive?0:XIPHOS_SWORD_SEARCH_ICASE_FLAG)|(whole_entry?xiphos::core::detail::sdk_search_whole_entry:0);
  auto &list=module.search(stripped.c_str(),engine_type,flags,
                           range?&*range:nullptr,nullptr,search_percent,&context);
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
