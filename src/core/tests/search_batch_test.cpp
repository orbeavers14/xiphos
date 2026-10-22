// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "search_source.hpp"
#include "search_results.hpp"
#include "search_control.hpp"
#include "../sdk_search.hpp"
#include <markupfiltmgr.h>
#include <rawtext.h>
#include <versekey.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
using namespace sword;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class E,class F>void rejects(F call){try{call();}catch(const E &){return;}throw std::runtime_error("Expected batch rejection");}
void main_dialog_search_percent_update(char,void *){}
void main_sidebar_search_percent_update(char,void *){}
struct BackEnd {EngineManager *main_mgr;SWModule *search_mod=nullptr;ListKey results,search_scope_list;SWKey *current_scope=nullptr;int do_module_search(char *,const char *,int,int,int);};
#include "legacy_search_backend_actual.inc"
std::vector<std::string> refs(ListKey &list){std::vector<std::string> out;list.setPosition(TOP);while(list.getCount()&&!list.popError()){out.emplace_back(list.getText());list++;}return out;}
struct ProgressModule:RawText {
 std::shared_ptr<SearchControl> control;ListKey result;bool cancel;
 ProgressModule(const char *path,std::shared_ptr<SearchControl> c,bool stop):RawText(path),control(std::move(c)),cancel(stop){VerseKey key("John1:1");result.add(key);}
 ListKey &search(const char *,int,int,SWKey *,bool *,void(*percent)(char,void *),void *user)override {
  percent(50,user);check(control->percent()==75,"Nested SDK progress maps 50 percent into the second half of a batch");
  if(cancel){control->request_cancel();percent(51,user);check(terminateSearch,"Owner callback still applies shared batch cancellation");}
  return result;
 }
};
int main(int argc,char **argv){try{
 check(argc==2,"Original attribute fixture required");const std::string root=argv[1];
 Reader reader(root);StudySession session(reader),other(reader);session.visit("Text","John1:2");
 SearchQuery first{"Text","N-ASF",SearchMode::morphology};first.text_options=SearchTextOptions(true,false,true);
 SearchQuery second{"Commentary","G140",SearchMode::lemma};second.within_last_results=true;
 SearchQuery third{"Text","Original",SearchMode::phrase};third.within_last_results=true;
 const std::vector<SearchQuery> chain{first,second,third};
 const auto batch=session.search_batch(chain,{{1,1024},{1,1024}});
 check(batch.groups.size()==3&&!batch.failures&&batch.copied_results==1&&batch.copied_reference_bytes==8&&batch.limited,"Whole-batch copied prefix is bounded independently of per-module limits");
 check(batch.groups[0].result->references==std::vector<std::string>{"John 1:1"}&&batch.groups[1].result->references.empty()&&batch.groups[2].result->references.empty(),"Exhausted display budget preserves group order without allocating more references");
 check(batch.groups[1].result->executed&&batch.groups[2].result->executed&&batch.scope.element_count==1&&batch.scope_updated&&session.last_search_scope().element_count==1,"Later modules still execute and publish complete final scope after copied output fills");
 check(batch.text_options_updated&&batch.text_options.options&&batch.text_options.options->strongs&&!batch.text_options.options->morphology&&batch.text_options.options->footnotes&&batch.groups[2].result->effective_text_options->strongs,"Batch phrase inherits prior successfully prepared annotation context");
 check(session.history().entries().size()==1&&!other.last_search_scope().present&&!other.search_text_options().options,"Batch changes no reading history or another tab");
 // Compare full last-result chaining with the actual retained backend, not a
 // reconstructed union/intersection over copied prefixes.
 const auto complete=session.search_batch(chain);
 EngineManager manager(root.c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);BackEnd legacy{&manager};
 for(std::size_t i=0;i<chain.size();++i){
  const auto &result=*complete.groups[i].result;legacy.current_scope=i?&legacy.search_scope_list:nullptr;
  const int flags=XIPHOS_SWORD_SEARCH_ICASE_FLAG|(i<2?xiphos::core::detail::sdk_search_whole_entry:0);
  check(legacy.do_module_search(const_cast<char *>(chain[i].module.c_str()),result.effective_expression.c_str(),result.engine_type,flags,1)>=0,"Retained ordered search completes");
  check(result.references==refs(legacy.results),"Every batch group matches actual legacy typed last-scope chaining");
 }
 check(complete.groups[0].result->references.size()==2&&complete.groups[1].result->references.size()==1&&complete.groups[2].result->references.size()==1,"Final scope follows last module rather than the union");
 auto bytes=session.search_batch({first,first},{{1000,1024},{10,7}});
 check(bytes.copied_results==0&&bytes.copied_reference_bytes==0&&bytes.limited&&bytes.groups[1].result->executed&&bytes.scope.element_count==2,"Whole UTF-8 keys exceeding remaining byte budget are omitted while final full scope survives");
 auto failure=session.search_batch({{"Missing","Original"},first,{"Commentary","Original"}});
 check(failure.failures==1&&failure.groups[0].failure==SearchBatchFailure::module_not_found&&!failure.groups[0].result&&failure.groups[1].request_index==1&&failure.groups[2].result->references.size()==4,"Missing module is an owned ordered failure and does not block later modules");
 auto bad_range=first;bad_range.range=SearchRange{"Missing","John1:1"};
 failure=session.search_batch({bad_range,first});check(failure.failures==1&&failure.groups[0].failure==SearchBatchFailure::module_not_found&&failure.groups[1].result,"Missing range provider is scoped failure and later group recovers");
 auto invalid=first;invalid.expression=std::string("bad\0query",9);
 const auto before=session.last_search_scope().element_count;
 rejects<std::invalid_argument>([&]{session.search_batch({first,invalid});});
 check(session.last_search_scope().element_count==before,"Malformed later input is rejected before any scope publication");
 invalid=first;invalid.mode=static_cast<SearchMode>(8);rejects<std::invalid_argument>([&]{session.search_batch({invalid});});
 invalid=first;invalid.range=SearchRange{"Text","John1:1"};invalid.within_last_results=true;rejects<std::invalid_argument>([&]{session.search_batch({invalid});});
 invalid=first;invalid.module=std::string(4097,'x');rejects<std::invalid_argument>([&]{session.search_batch({invalid});});
 rejects<std::invalid_argument>([&]{session.search_batch({});});rejects<std::invalid_argument>([&]{session.search_batch(std::vector<SearchQuery>(33,first));});
 rejects<std::invalid_argument>([&]{session.search_batch({first},{{0,1024},{10,1024}});});rejects<std::invalid_argument>([&]{session.search_batch({first},{{1,1024},{10,0}});});
 auto maximum=session.search_batch(std::vector<SearchQuery>(32,first),{{1,1024},{1,1024}});check(maximum.groups.size()==32&&maximum.copied_results==1&&maximum.scope.element_count==2,"Maximum bounded module count keeps duplicates and executes beyond output cap");
 auto stop=std::make_shared<SearchControl>();stop->request_cancel();const auto stopped=session.search_batch(chain,{},stop);
 check(stopped.cancelled&&stopped.groups.empty()&&stopped.requested.size()==3&&!stopped.scope_updated&&!stopped.text_options_updated&&stopped.scope.element_count==2&&stop->phase()==SearchPhase::cancelled&&stop->percent()==0,"Pre-cancelled batch keeps original state and no attempted groups");
 rejects<std::invalid_argument>([&]{session.search_batch(chain,{},stop);});
 for(bool cancel:{false,true}){
  auto control=std::make_shared<SearchControl>();detail::SearchControlScope progress(control);
  ProgressModule module((std::filesystem::path(root)/"data/text").string().c_str(),control,cancel);
  auto captured=detail::capture_module_search(manager,module,{"Text","Original"},{},control,nullptr,nullptr,{}, {},50,100);
  progress.finish(captured.cancelled);check(captured.executed&&captured.cancelled==cancel&&control->phase()==(cancel?SearchPhase::cancelled:SearchPhase::complete)&&!module.terminateSearch,"Nested callback cancellation/restoration and terminal disposition agree");
 }
 ListKey keys;keys.add(SWKey("whole"));check(detail::copy_search_references(keys,{0,0},true).limited,"Internal zero-budget copy records omission without output");
 rejects<std::invalid_argument>([&]{detail::copy_search_references(keys,{0,0});});
 std::future<SearchBatchResult> drained;
 {
  StudyWorker worker(root);const auto a=worker.create_tab().get(),b=worker.create_tab().get();
  auto copied=chain;auto queued=worker.search_batch_task(a,copied,{{1,1024},{1,1024}});copied[0].expression="unrelated";
  const auto owned=queued.result.get();check(owned.requested[0].expression=="N-ASF"&&owned.scope.element_count==1&&queued.control->phase()==SearchPhase::complete&&queued.control->percent()==100,"Queued batch copies input, owns one terminal control and preserves scope");
  check(worker.last_search_scope(a).get().element_count==1&&!worker.last_search_scope(b).get().present,"Whole batch runs before the next FIFO metadata command and isolates tabs");
  auto unknown=worker.search_batch_task(999,chain);rejects<std::invalid_argument>([&]{(void)unknown.result.get();});check(unknown.control->phase()==SearchPhase::failed,"Unknown-tab batch marks queued control failed");
  auto malformed=worker.search_batch_task(a,{invalid});rejects<std::invalid_argument>([&]{(void)malformed.result.get();});check(malformed.control->phase()==SearchPhase::failed&&worker.last_search_scope(a).get().element_count==1,"Bad batch fails control without changing last committed state");
  auto race=worker.search_batch_task(a,std::vector<SearchQuery>(32,first));race.control->request_cancel();const auto value=race.result.get();
  check(value.groups.size()<=32&&value.cancelled==(race.control->phase()==SearchPhase::cancelled),"Immediate worker cancellation returns an authoritative completed prefix or wins completion race");
  check(worker.search_batch(a,chain).get().scope.element_count==1,"Fresh batch recovers after cancellation");
  drained=worker.search_batch(b,chain);
 }
 const auto held=drained.get();check(held.groups.size()==3&&held.groups[2].result->references==std::vector<std::string>{"John 1:1"},"All owned group results survive draining worker shutdown");
 std::cout<<"Ordered bounded batch search, legacy scope chaining, staged state, errors, progress, cancellation and FIFO ownership passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
