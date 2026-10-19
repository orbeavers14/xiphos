// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "search_source.hpp"
#include "search_control.hpp"
#include "search_results.hpp"
#include <markupfiltmgr.h>
#include <rawtext.h>
#include <versekey.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
using namespace xiphos::core::detail;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected scope failure");}
struct PartialModule:sword::RawText {
 sword::ListKey result;bool fail=false;std::shared_ptr<SearchControl> control;
 PartialModule(const char *path,std::shared_ptr<SearchControl> cancel):RawText(path),control(std::move(cancel)){
  sword::VerseKey key("John1:3");result.add(key);
 }
 sword::ListKey &search(const char *,int,int,sword::SWKey *,bool *,void(*percent)(char,void *),void *user)override{
  if(fail)throw std::runtime_error("Original scope failure");
  control->request_cancel();percent(50,user);return result;
 }
};
int main(int argc,char **argv){try{
 check(argc==2,"Original scope fixture required");const std::string root=argv[1];SearchResult owned;
 {
  Reader reader(root);StudySession first(reader),other(reader);
  first.visit("Text","John1:2");SearchQuery last{"Text","Original"};last.within_last_results=true;
  auto empty=first.search_module(last);check(!empty.executed&&!empty.cancelled&&!empty.scope.present&&!empty.scope_updated&&empty.references.empty(),"Absent last scope skips SDK execution without inventing a committed scope");
  rejects<std::invalid_argument>([&]{reader.search_module(last);});
  auto seed=first.search_module({"Text","Original"},{1,1024});
  check(seed.limited&&seed.references.size()==1&&seed.scope_updated&&seed.scope.present&&seed.scope.element_count==3,"Session stores complete scope independently of visible prefix");
  check(!other.last_search_scope().present&&other.search_module(last).references.empty(),"Another tab cannot inherit scope from the common module SDK results");
  last.module="Commentary";last.expression="beta";owned=first.search_module(last);
  check(owned.references==std::vector<std::string>{"John 1:1","John 1:3"}&&owned.scope.element_count==2&&owned.scope_updated,"Complete retained keys include omitted prefix matches and cross-module scope");
  auto cancel=std::make_shared<SearchControl>();cancel->request_cancel();auto stopped=first.search_module(last,{},cancel);
  check(stopped.cancelled&&!stopped.executed&&!stopped.scope_updated&&stopped.scope.element_count==2,"Pre-execution cancellation preserves committed complete scope");
  auto invalid=last;invalid.range=SearchRange{"Text","John1:1"};rejects<std::invalid_argument>([&]{first.search_module(invalid);});
  invalid=last;invalid.module="Missing";rejects<std::out_of_range>([&]{first.search_module(invalid);});
  invalid=last;invalid.expression="";rejects<std::invalid_argument>([&]{first.search_module(invalid);});
  check(first.last_search_scope().element_count==2,"Invalid/failed requests retain previous scope atomically");
  last.expression="omega";auto narrow=first.search_module(last);check(narrow.references==std::vector<std::string>{"John 1:3"}&&narrow.scope.element_count==1,"Repeated last-result search narrows newly committed complete scope");
  last.expression="unmatched-original-value";auto zero=first.search_module(last);check(zero.executed&&zero.scope_updated&&zero.scope.present&&!zero.scope.element_count,"Executed no-match search commits a present empty scope");
  last.expression="Original";auto still_empty=first.search_module(last);check(!still_empty.executed&&!still_empty.scope_updated&&still_empty.scope.present&&still_empty.references.empty(),"Empty last scope cannot accidentally fall back to whole-module search");
  SearchQuery range{"Text","Original"};range.range=SearchRange{"Text","John1:2-John1:3"};first.search_module(range,{1,1024});
  last.module="Text";auto bounds=first.search_module(last);check(bounds.references==std::vector<std::string>{"John 1:2","John 1:3"},"Explicit range search reseeds the complete retained scope");
  first.clear_search_scope();check(!first.last_search_scope().present&&!first.search_module(last).executed,"Explicit clear resets to absent last scope without searching");
  check(first.history().entries().size()==1&&first.history().entries().front().reference=="John 1:2"&&other.history().entries().empty(),"Scope updates preserve reading and independent histories");
  auto fresh=first.search_module({"Text","Original"});check(fresh.scope.element_count==3&&!fresh.requested.within_last_results,"Normal searches reseed scope without inheriting prior range");
 }
 check(owned.requested.within_last_results&&owned.references.size()==2&&owned.scope.element_count==2,"Public copied metadata outlives session/Reader destruction without SDK keys");
 {
  sword::VerseKey parser;auto intervals=parser.parseVerseList("John1:1-John1:3;Gen1:1","",true);SearchScope scope(intervals);
  const auto *original=dynamic_cast<const sword::VerseKey *>(intervals.getElement(0));const auto *clone=dynamic_cast<const sword::VerseKey *>(scope.keys().getElement(0));
  check(original&&clone&&original!=clone&&std::string(original->getVersificationSystem())==clone->getVersificationSystem()&&std::string(original->getRangeText())==clone->getRangeText(),"Private snapshot clones full typed SDK interval keys and versification");
  intervals.clear();auto copy=scope.keys();check(scope.info().element_count==2&&copy_search_references(copy,{}).references.size()==4,"Owned interval scope survives source replacement and retains expanded matches");
  sword::VerseKey alternate("John1:1","John1:3","Vulg");sword::ListKey alternate_list;alternate_list.add(alternate);SearchScope alternate_scope(alternate_list);
  const auto *alternate_key=dynamic_cast<const sword::VerseKey *>(alternate_scope.keys().getElement(0));
  check(alternate_key&&std::string(alternate_key->getVersificationSystem())=="Vulg"&&std::string(alternate_key->getRangeText())==alternate.getRangeText(),"Alternate versification and bounds remain typed, not reparsed as target strings");
  rejects<std::length_error>([&]{SearchScope small(scope.keys(),{1,1024});});rejects<std::length_error>([&]{SearchScope small(scope.keys(),{2,1});});
 }
 {
  EngineManager manager(root.c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);auto *module=manager.getModule("Text");
  std::shared_ptr<const SearchScope> staged;auto original=capture_module_search(manager,*module,{"Text","Original"},{}, {},nullptr,&staged);
  auto previous=staged;check(previous&&previous->info().element_count==3,"Internal capture stages complete owned scope");
  rejects<std::length_error>([&]{capture_module_search(manager,*module,{"Text","Original"},{},{},previous.get(),&staged,{2,1024});});
  check(!staged&&previous->info().element_count==3,"Retention overflow publishes neither partial scope nor successful output");
  auto control=std::make_shared<SearchControl>();PartialModule partial((std::filesystem::path(root)/"data/text").string().c_str(),control);
  SearchControlScope progress(control);auto stopped=capture_module_search(manager,partial,{"Text","Original"},{},control,previous.get(),&staged);progress.finish(stopped.cancelled);
  check(stopped.executed&&stopped.cancelled&&staged&&staged->info().element_count==1&&stopped.references==std::vector<std::string>{"John 1:3"}&&!partial.terminateSearch,"Returned partial scope stages despite cancellation and restores SDK stop flag");
  previous=staged;partial.fail=true;rejects<std::runtime_error>([&]{capture_module_search(manager,partial,{"Text","Original"},{},{},previous.get(),&staged);});
  check(!staged&&previous->info().element_count==1,"SDK failure preserves prior immutable scope");
 }
 std::cout<<"Private complete last-search scopes, tab isolation, publication, capacity and interval ownership passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
