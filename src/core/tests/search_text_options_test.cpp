// SPDX-License-Identifier: GPL-2.0-or-later
#include "../sdk_search.hpp"
#include "xiphos/core/session.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "search_source.hpp"
#include <markupfiltmgr.h>
#include <swoptfilter.h>
#include <swfilter.h>
#include <rawtext.h>
#include <versekey.h>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
using namespace sword;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void main_dialog_search_percent_update(char,void *){}
void main_sidebar_search_percent_update(char,void *){}
struct Widgets{bool rb_exact_phrase,cb_include_strongs,cb_include_morphs,cb_include_footnotes;}search1;
EngineManager *active=nullptr;
#define GTK_TOGGLE_BUTTON(value) (value)
bool gtk_toggle_button_get_active(bool value){return value;}
void set_search_global_option(const char *name,bool value){active->setGlobalOption(name,value?"On":"Off");}
#include "legacy_search_text_options.inc"
struct BackEnd{EngineManager *main_mgr;SWModule *search_mod=nullptr;ListKey results,search_scope_list;SWKey *current_scope=nullptr;int do_module_search(char *,const char *,int,int,int);};
#include "legacy_search_backend_actual.inc"
std::vector<std::string> refs(ListKey &list){std::vector<std::string> out;list.setPosition(TOP);while(list.getCount()&&!list.popError()){out.emplace_back(list.getText());list++;}return out;}
bool same(const std::optional<SearchTextOptions> &a,const SearchTextOptions &b){return a&&a->strongs==b.strongs&&a->morphology==b.morphology&&a->footnotes==b.footnotes;}
struct Fault:SWFilter{bool enabled=true;char processText(SWBuf &,const SWKey *,const SWModule *)override{if(enabled)throw std::runtime_error("Original search text strip failure");return 0;}};
struct InspectingModule:RawText {
 EngineManager &manager;SearchTextOptions expected;ListKey result;bool called=false,indexed=false,probed=false,fail_probe=false,expect_probe=true,optimal=true;int expected_type=xiphos::core::detail::sdk_search_external,framework_checks=0;std::string expected_query="+Original ";
 InspectingModule(const char *path,EngineManager &owner,SearchTextOptions wanted):RawText(path),manager(owner),expected(wanted){VerseKey key("John1:1");result.add(key);}
 void inspect_values(){
  std::array<bool,3> found{{false,false,false}};const auto settings=search_text_settings(expected);
  for(auto *filter:manager.option_filters())for(std::size_t i=0;i<settings.size();++i)if(std::string(filter->getOptionName())==settings[i].name){found[i]=true;check(std::string(filter->getOptionValue())==(settings[i].enabled?"On":"Off"),"Actual SDK execution/probe observes requested search-only filter values");}
  check(std::all_of(found.begin(),found.end(),[](bool item){return item;}),"Fixture contains all three real SDK annotation filters");
 }
 bool hasSearchFramework()override{++framework_checks;return indexed;}
 bool isSearchOptimallySupported(const char *query,int type,int flags,SWKey *scope)override{
  check(std::string(query)=="God"&&type==-4&&!flags&&!scope,"Index capability uses exact legacy probe arguments");
  inspect_values();probed=true;
  if(fail_probe){setProcessEntryAttributes(true);setSkipConsecutiveLinks(false);throw std::runtime_error("Original index probe failure");}
  return optimal;
 }
 ListKey &search(const char *query,int type,int,SWKey *,bool *,void(*)(char,void *),void *)override{
  inspect_values();if(indexed)check(probed==expect_probe&&type==expected_type&&std::string(query)==expected_query,"Selected word dialect and preparation follow scoped capability policy");
  called=true;return result;
 }

};
int main(int argc,char **argv){try{
 check(argc==2,"Original search-text fixture required");const std::string root=argv[1];Fault fault;
 Reader reader(root);StudySession first(reader),other(reader);
 const auto baseline=other.search_module({"Text","G140",SearchMode::words}).references;
 EngineManager legacy(root.c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);active=&legacy;BackEnd backend{&legacy};
 for(int bits=0;bits<8;++bits){
  const SearchTextOptions requested(bool(bits&1),bool(bits&2),bool(bits&4));
  for(bool phrase:{false,true}){
   const SearchTextOptions checkbox=phrase?SearchTextOptions(!requested.strongs,!requested.morphology,!requested.footnotes):requested;
   SearchQuery query{"Text","G140",phrase?SearchMode::phrase:SearchMode::words};query.text_options=checkbox;
   const auto result=first.search_module(query);
   search1={phrase,checkbox.strongs,checkbox.morphology,checkbox.footnotes};check_search_global_options();
   auto *module=legacy.getModule("Text");const std::string effective=sword::SWBuf(module->stripText(query.expression.c_str())).c_str();
   check(backend.do_module_search(const_cast<char *>("Text"),effective.c_str(),result.engine_type,XIPHOS_SWORD_SEARCH_ICASE_FLAG,1)>=0,"Actual legacy option/phrase execution completes");
   check(result.references==refs(backend.results)&&result.effective_expression==effective&&same(result.requested.text_options,checkbox)&&same(result.effective_text_options,requested)&&result.text_options_updated==!phrase,"Session query preparation/results and sticky phrase state match actual legacy SDK execution");
   check(same(first.search_text_options().options,requested),"Committed search-only context follows nonphrase choices and ignores phrase checkbox changes");
  }
  check(other.search_module({"Text","G140",SearchMode::words}).references==baseline&&!other.search_text_options().options,"Shared SDK filter overrides do not leak into another study session");
 }
 const SearchTextOptions wanted(true,false,true);first.clear_search_scope();
 SearchQuery empty{"Text","Original",SearchMode::words};empty.within_last_results=true;empty.text_options=wanted;
 const auto skipped=first.search_module(empty);
 check(!skipped.executed&&skipped.text_options_updated&&same(skipped.effective_text_options,wanted)&&same(first.search_text_options().options,wanted),"Empty last scope still commits successfully prepared search options, as legacy setup precedes scope short-circuit");
 auto cancelled=std::make_shared<SearchControl>();cancelled->request_cancel();empty.text_options=SearchTextOptions(false,true,false);
 const auto stopped=first.search_module(empty,{},cancelled);
 check(stopped.cancelled&&!stopped.executed&&!stopped.effective_text_options&&!stopped.text_options_updated&&same(first.search_text_options().options,wanted),"Pre-preparation cancellation cannot change logical search options");
 auto missing=empty;missing.module="Missing";try{first.search_module(missing);throw std::logic_error("Expected missing module");}catch(const std::out_of_range &){}
 check(same(first.search_text_options().options,wanted),"Failed query preserves prior logical options atomically");
 first.search_module({"Text","Original"});const auto prior=first.last_search_scope();first.clear_search_scope();
 check(same(first.search_text_options().options,wanted),"Clearing last results does not reset sticky search options");
 first.search_module({"Text","Original"});first.clear_search_text_options();
 check(!first.search_text_options().options&&first.last_search_scope().element_count==prior.element_count,"Explicit option reset is independent of retained result scope");
 SearchQuery phrase{"Text","G140",SearchMode::phrase};phrase.text_options=wanted;
 const auto reset=first.search_module(phrase);check(!reset.effective_text_options&&!reset.text_options_updated&&!first.search_text_options().options,"Phrase after reset preserves unconfigured SDK baseline and ignores checkbox input");
 auto *module=legacy.getModule("Text");std::vector<std::pair<SWOptionFilter *,std::string>> filters;for(auto *filter:legacy.option_filters())filters.emplace_back(filter,filter->getOptionValue());
 module->addStripFilter(&fault);SearchQuery broken{"Text","Original",SearchMode::words};broken.text_options=SearchTextOptions(true,true,true);
 try{detail::capture_module_search(legacy,*module,broken,{});throw std::logic_error("Expected strip failure");}catch(const std::runtime_error &){}
 fault.enabled=false;for(const auto &filter:filters)check(filter.first->getOptionValue()==filter.second,"Strip failure restores each alias/filter baseline after temporary text overrides");
 check(!detail::capture_module_search(legacy,*module,broken,{}).references.empty(),"Text-filter search recovers after injected preparation failure");
 InspectingModule inspect((std::filesystem::path(root)/"data/text").string().c_str(),legacy,SearchTextOptions(false,false,false));
 SearchQuery inspected{"Text","Original",SearchMode::regular_expression};inspected.text_options=SearchTextOptions(false,false,false);
 detail::capture_module_search(legacy,inspect,inspected,{});check(inspect.called,"Inspect real SDK filters during execution, not only copied metadata");
 inspect.called=false;inspected.mode=SearchMode::phrase;inspected.text_options=SearchTextOptions(true,true,true);
 detail::capture_module_search(legacy,inspect,inspected,{}, {},nullptr,nullptr,{},SearchTextOptions(false,false,false));
 check(inspect.called,"Exact phrase materializes previous private settings rather than changed checkbox values");
 InspectingModule indexed((std::filesystem::path(root)/"data/text").string().c_str(),legacy,SearchTextOptions(false,false,false));indexed.indexed=true;
 inspected.mode=SearchMode::words;inspected.text_options=SearchTextOptions(false,false,false);
 const auto planned=detail::capture_module_search(legacy,indexed,inspected,{});
 check(indexed.called&&indexed.probed&&planned.engine_type==xiphos::core::detail::sdk_search_external,"Controlled index capability sees effective flags before execution selection");
 InspectingModule advanced((std::filesystem::path(root)/"data/text").string().c_str(),legacy,SearchTextOptions(false,false,false));advanced.indexed=true;advanced.expected_query="Original OR beta";
 inspected.mode=SearchMode::indexed;inspected.expression="Original OR beta";
 const auto raw=detail::capture_module_search(legacy,advanced,inspected,{});
 check(raw.engine_type==xiphos::core::detail::sdk_search_external&&!raw.index_fallback&&raw.effective_expression=="Original OR beta","Explicit indexed dialect preserves advanced raw syntax, unlike sidebar AND");
 InspectingModule plain((std::filesystem::path(root)/"data/text").string().c_str(),legacy,SearchTextOptions(false,false,false));plain.indexed=true;plain.expect_probe=false;plain.expected_type=xiphos::core::detail::sdk_search_multiword;plain.expected_query="Original";
 inspected.mode=SearchMode::multiword;inspected.expression="Original";
 const auto forced=detail::capture_module_search(legacy,plain,inspected,{});
 check(forced.engine_type==xiphos::core::detail::sdk_search_multiword&&!forced.index_fallback&&!plain.probed&&!plain.framework_checks,"Explicit advanced multiword bypasses framework probing even when available");
 InspectingModule unsupported((std::filesystem::path(root)/"data/text").string().c_str(),legacy,SearchTextOptions(false,false,false));unsupported.indexed=true;unsupported.optimal=false;unsupported.expected_type=xiphos::core::detail::sdk_search_multiword;unsupported.expected_query="Original";
 inspected.mode=SearchMode::indexed;inspected.expression="Original";
 const auto not_supported=detail::capture_module_search(legacy,unsupported,inspected,{});
 check(unsupported.probed&&not_supported.index_fallback&&not_supported.engine_type==xiphos::core::detail::sdk_search_multiword,"Present framework with unsupported capability also reports explicit fallback");
 InspectingModule unavailable((std::filesystem::path(root)/"data/text").string().c_str(),legacy,SearchTextOptions(false,false,false));inspected.mode=SearchMode::indexed;
 const auto fallback=detail::capture_module_search(legacy,unavailable,inspected,{});
 check(fallback.index_fallback&&fallback.engine_type==xiphos::core::detail::sdk_search_multiword&&fallback.effective_expression=="Original","Missing framework yields explicit owned fallback without sidebar query rewriting");
 inspected.mode=SearchMode::words;
 indexed.fail_probe=true;indexed.setProcessEntryAttributes(false);indexed.setSkipConsecutiveLinks(true);
 try{detail::capture_module_search(legacy,indexed,inspected,{});throw std::logic_error("Expected capability failure");}catch(const std::runtime_error &){}
 check(!indexed.isProcessEntryAttributes()&&indexed.isSkipConsecutiveLinks(),"Failed capability probe restores module flags too");
 for(const auto &filter:filters)check(filter.first->getOptionValue()==filter.second,"Inspection jobs restore each real SDK filter after scoped materialization");
 std::cout<<"Shared search-only options match legacy sticky phrase state, isolated tabs, resets and scoped restoration passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
