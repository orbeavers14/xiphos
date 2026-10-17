// SPDX-License-Identifier: GPL-2.0-or-later
#include "search_source.hpp"
#include "search_control.hpp"
#include "xiphos/core/search_policy.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/presentation.hpp"
#include <swmodule.h>
#include <swoptfilter.h>
#include <versekey.h>
#include <listkey.h>
#include <glib.h>
#include <stdexcept>
#include <thread>
namespace xiphos::core::detail {
namespace {
class ScopedSearchFilters {
  std::vector<std::pair<sword::SWOptionFilter *,std::string>> saved_;
public:
  explicit ScopedSearchFilters(EngineManager &manager) {
    for(auto *filter:manager.option_filters()) {
      const std::string name=filter->getOptionName();
      if(name!="Greek Accents"&&name!="Hebrew Vowel Points"&&name!="Arabic Vowel Points")continue;
      const auto *value=filter->getOptionValue();
      if(!value)throw std::runtime_error("Search filter has no restorable value");
      saved_.emplace_back(filter,value);
    }
    try {for(auto &value:saved_)value.first->setOptionValue("Off");}
    catch(...){restore();throw;}
  }
  ~ScopedSearchFilters(){restore();}
  void restore(){for(auto &value:saved_)value.first->setOptionValue(value.second.c_str());}
};
struct ScopedSearchModule {
  sword::SWModule &module;bool attributes,skip_links;
  explicit ScopedSearchModule(sword::SWModule &m):module(m),attributes(m.isProcessEntryAttributes()),skip_links(m.isSkipConsecutiveLinks()){}
  ~ScopedSearchModule(){module.setProcessEntryAttributes(attributes);module.setSkipConsecutiveLinks(skip_links);module.terminateSearch=false;}
};
struct ProgressContext {sword::SWModule *module;SearchControl *control;std::thread::id owner;};
void search_percent(char percent,void *user) noexcept {
  auto &context=*static_cast<ProgressContext *>(user);
  if(!context.control)return;
  SearchControlAccess::progress(*context.control,static_cast<unsigned char>(percent));
  // Only the owning engine callback touches the SDK's non-atomic flag.
  if(std::this_thread::get_id()==context.owner&&context.control->cancel_requested())context.module->terminateSearch=true;
}
}
SearchResult capture_module_search(EngineManager &manager,sword::SWModule &module,
                                  SearchQuery query,const SearchLimits &limits,const std::shared_ptr<SearchControl> &control) {
  if(query.module.empty()||query.module.find('\0')!=std::string::npos||query.expression.empty()||
     query.expression.size()>32768||query.expression.find('\0')!=std::string::npos||
     !g_utf8_validate(query.expression.data(),query.expression.size(),nullptr))
    throw std::invalid_argument("Search needs a nonempty UTF-8 query of at most 32768 bytes");
  if(!limits.max_results||limits.max_results>maximum_search_results||!limits.max_reference_bytes||
     limits.max_reference_bytes>maximum_search_reference_bytes)
    throw std::invalid_argument("Unsupported search output limits");
  const std::string type=module.getType();const auto *driver=module.getConfigEntry("ModDrv");
  if(!dynamic_cast<sword::VerseKey *>(module.getKey())||
     (type!="Biblical Texts"&&!regular_commentary(type.c_str(),driver)))
    throw std::invalid_argument("Initial search supports Bible and regular commentary verse modules");
  int engine_type;
  switch(query.mode){
    case SearchMode::regular_expression:engine_type=sword::SWModule::SEARCHTYPE_REGEX;break;
    case SearchMode::phrase:engine_type=sword::SWModule::SEARCHTYPE_PHRASE;break;
    case SearchMode::words:
      engine_type=module.hasSearchFramework()&&module.isSearchOptimallySupported("God",-4,0,0)?
        sword::SWModule::SEARCHTYPE_EXTERNAL:sword::SWModule::SEARCHTYPE_MULTIWORD;break;
    default:throw std::invalid_argument("Unknown search mode");
  }
  if(control&&control->cancel_requested()) {
    SearchResult result{std::move(query),{},engine_type,0,{},false};result.cancelled=true;return result;
  }
  ScopedSearchFilters filters(manager);ScopedSearchModule state(module);
  auto prepared=sidebar_search_text(query.expression,engine_type==sword::SWModule::SEARCHTYPE_EXTERNAL);
  const std::string stripped=module.stripText(prepared.c_str()).c_str();
  if(control&&control->cancel_requested()) {
    SearchResult result{std::move(query),stripped,engine_type,0,{},false};result.cancelled=true;return result;
  }
  ProgressContext context{&module,control.get(),std::this_thread::get_id()};
  auto &list=module.search(stripped.c_str(),engine_type,query.case_sensitive?0:XIPHOS_SWORD_SEARCH_ICASE_FLAG,
                           nullptr,nullptr,search_percent,&context);
  if(engine_type==sword::SWModule::SEARCHTYPE_EXTERNAL)list.sort();
  if(list.getCount()<0)throw std::runtime_error("SDK search returned an invalid count");
  SearchResult result{std::move(query),stripped,engine_type,static_cast<std::uint64_t>(list.getCount()),{},false};
  std::size_t bytes=0;list.setPosition(sword::TOP);
  while(list.getCount()&&!list.popError()) {
    const std::string reference=list.getText();
    if(reference.empty())throw std::runtime_error("SDK search returned an empty reference");
    if(result.references.size()==limits.max_results||reference.size()>limits.max_reference_bytes-bytes){result.limited=true;break;}
    bytes+=reference.size();result.references.push_back(reference);list++;
  }
  result.executed=true;result.cancelled=control&&control->cancel_requested();
  return result;
}
}
