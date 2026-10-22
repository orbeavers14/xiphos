// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "search_generation.hpp"
#include "search_source.hpp"
#include "search_control.hpp"
#include <markupfiltmgr.h>
#include <rawtext.h>
#include <swfilter.h>
#include <versekey.h>
#include <filesystem>
#include <limits>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
using namespace xiphos::core::detail;
using namespace sword;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class E,class F>void rejects(F call){try{call();}catch(const E &){return;}throw std::runtime_error("Expected retained-generation rejection");}
struct ExpandingFilter:SWFilter {
 std::string text;
 explicit ExpandingFilter(std::string value):text(std::move(value)){}
 char processText(SWBuf &buffer,const SWKey *,const SWModule *)override{buffer.setSize(text.size());for(std::size_t i=0;i<text.size();++i)buffer[static_cast<unsigned long>(i)]=text[i];return 0;}
};
const std::vector<std::string> all{"John 1:1","John 1:2","John 1:3","John 1:4"};
struct OrderedModule:RawText {
 ListKey result;bool indexed=true,stop=false,fail=false;std::shared_ptr<SearchControl> control;
 OrderedModule(const char *path):RawText(path){VerseKey third("John1:3"),first("John1:1");result.add(third);result.add(first);}
 bool hasSearchFramework()override{return indexed;}
 bool isSearchOptimallySupported(const char *,int,int,SWKey *)override{return true;}
 ListKey &search(const char *,int,int,SWKey *,bool *,void(*percent)(char,void *),void *user)override{
  if(fail)throw std::runtime_error("Original generation SDK failure");
  if(stop){control->request_cancel();percent(50,user);}return result;
 }
};
int main(int argc,char **argv){try{
 check(argc==2,"Original generation fixture required");const std::string root=argv[1];
 Reader reader(root);StudySession session(reader),other(reader);session.visit("Text","John1:2");
 check(!session.search_generation().id&&session.search_generation().retention==SearchRetentionStatus::none&&session.read_search_generation(0,0).status==SearchGenerationReadStatus::stale,"Absent generation has no valid ID or invented keys");
 rejects<std::invalid_argument>([&]{session.read_search_generation(0,0,SearchReferenceSource::complete,{0,10});});
 auto seed=session.search_module({"Text","Original"},{1,1024});const auto id=seed.generation.id;
 check(id==1&&seed.generation_updated&&seed.generation.retention==SearchRetentionStatus::complete&&seed.generation.retained_elements==4&&seed.generation.matched_elements==4&&seed.generation.visible_results==1&&seed.generation.accounted_key_bytes>0,"Single returned prefix identifies complete retained keys with independent accounting");
 const auto held=session.read_search_generation(id,0);check(held.status==SearchGenerationReadStatus::ok&&held.result->references==all&&!held.result->limited&&held.requested->expression=="Original","Complete re-read recovers omitted keys without rerunning SDK search");
 auto visible=session.read_search_generation(id,0,SearchReferenceSource::visible);check(visible.result->references==std::vector<std::string>{"John 1:1"}&&visible.result->limited,"Visible read cannot relabel its truncated original prefix complete");
 auto clipped=session.read_search_generation(id,0,SearchReferenceSource::complete,{10,7});check(clipped.result->references.empty()&&clipped.result->limited,"Re-read byte budget omits whole reference rather than splitting UTF-8/key bytes");
 check(session.read_search_generation(id,99).status==SearchGenerationReadStatus::invalid_group,"Invalid requested group is explicit");
 session.clear_search_scope();session.clear_search_text_options();check(session.read_search_generation(id,0).result->references==all&&!session.last_search_scope().present&&session.history().entries().size()==1&&!other.search_generation().id,"Scope/context reset and history/other tabs are independent of retained results");
 session.set_search_retention_limits({1,1});check(session.read_search_generation(id,0).result->references==all,"Policy changes affect future generations only");
 auto small=session.search_module({"Commentary","N-ASF",SearchMode::morphology},{1,1024});
 check(small.generation.id==2&&small.generation.retention==SearchRetentionStatus::capacity_exceeded&&!small.generation.retained_elements&&!small.generation.accounted_key_bytes&&small.scope.element_count==2&&small.references.size()==1,"Capacity overflow preserves usable results and full last scope while publishing explicit new unavailable generation");
 check(session.read_search_generation(id,0).status==SearchGenerationReadStatus::stale&&session.read_search_generation(id,0).generation.id==2&&session.read_search_generation(2,0).status==SearchGenerationReadStatus::capacity_exceeded&&session.read_search_generation(2,0,SearchReferenceSource::visible).result->references.size()==1,"Stale and unavailable complete reads cannot masquerade as old/new complete results");
 const auto before=session.search_generation().id;auto invalid=SearchQuery{"Text","Original"};invalid.expression=std::string("bad\0query",9);
 rejects<std::invalid_argument>([&]{session.search_module(invalid);});rejects<std::out_of_range>([&]{session.search_module({"Missing","Original"});});
 auto stop=std::make_shared<SearchControl>();stop->request_cancel();auto cancelled=session.search_module({"Text","Original"},{},stop);
 check(!cancelled.executed&&cancelled.cancelled&&!cancelled.generation_updated&&cancelled.generation.id==before&&stop->phase()==SearchPhase::cancelled&&session.search_generation().id==before,"Errors and pre-execution cancellation retain committed generation without stranding phase");
 for(const auto limits:{SearchRetentionLimits{0,1},SearchRetentionLimits{1,0},SearchRetentionLimits{maximum_search_retained_elements+1,1},SearchRetentionLimits{1,maximum_search_retained_key_bytes+1}})
  rejects<std::invalid_argument>([&]{session.set_search_retention_limits(limits);});
 check(session.search_retention_limits().max_elements==1,"Invalid policy cannot overwrite previous limits");
 session.set_search_retention_limits({});session.clear_search_scope();SearchQuery last{"Text","Original"};last.within_last_results=true;
 auto empty=session.search_module(last);check(!empty.executed&&!empty.cancelled&&empty.generation_updated&&empty.generation.id==3&&empty.generation.retention==SearchRetentionStatus::complete&&session.read_search_generation(3,0).result->references.empty(),"Successful empty last scope publishes explicit empty generation");
 StudySession copied=session;auto copied_result=copied.search_module({"Text","Original"});check(copied_result.generation.id==4&&session.search_generation().id==3&&session.read_search_generation(3,0).result->references.empty(),"Session copies share only immutable old generation and replace independently");
 std::vector<SearchQuery> queries{{"Text","Original"},{"Missing","Original"},{"Commentary","N-ASF",SearchMode::morphology},{"Text","G140",SearchMode::lemma}};
 auto batch=session.search_batch(queries,{{1,1024},{1,1024}});const auto batch_id=batch.generation.id;
 check(batch_id==4&&batch.generation_updated&&batch.generation.requested_count==4&&batch.generation.group_count==4&&batch.generation.failed_groups==1&&batch.generation.visible_results==1&&batch.generation.retained_elements==8&&batch.generation.matched_elements==8&&batch.scope.element_count==2,"Batch retains all module groups independently of total displayed prefix and final last scope");
 check(session.read_search_generation(batch_id,0).result->references==all&&session.read_search_generation(batch_id,2).result->references==std::vector<std::string>{"John 1:1","John 1:3"}&&session.read_search_generation(batch_id,3).result->references==std::vector<std::string>{"John 1:1","John 1:4"},"Earlier complete result groups survive later SDK searches and narrowed final scope");
 const auto failed=session.read_search_generation(batch_id,1);check(failed.status==SearchGenerationReadStatus::failed_group&&failed.failure==SearchBatchFailure::module_not_found&&failed.requested->module=="Missing"&&!failed.result,"Failed group retains original owned query and failure rather than false empty success");
 check(batch.groups[0].result->generation.id==batch_id&&batch.groups[2].result->generation_updated,"All successful public batch children identify the same publication");
 const auto bytes=seed.generation.accounted_key_bytes;
 session.set_search_retention_limits({4,bytes});auto exact=session.search_batch({{"Text","Original"},{"Commentary","AbsentToken"}},{{1,1024},{1,1024}});
 check(exact.generation.retention==SearchRetentionStatus::complete&&exact.generation.retained_elements==4&&exact.generation.accounted_key_bytes==bytes&&exact.scope.element_count==0&&session.read_search_generation(exact.generation.id,0).result->references==all&&session.read_search_generation(exact.generation.id,1).result->references.empty(),"Exact exhausted count/text budget still admits later empty group and retains earlier keys after empty final scope");
 session.set_search_retention_limits({4,bytes});auto overflow=session.search_batch({{"Text","Original"},{"Commentary","Original"}},{{1,1024},{1,1024}});
 check(overflow.generation.retention==SearchRetentionStatus::capacity_exceeded&&!overflow.generation.retained_elements&&!overflow.generation.accounted_key_bytes&&overflow.generation.matched_elements==8&&overflow.scope.element_count==4,"Aggregate count cap across duplicate-sized groups is not multiplied by module count");
 check(session.read_search_generation(overflow.generation.id,0).status==SearchGenerationReadStatus::capacity_exceeded&&session.read_search_generation(overflow.generation.id,1,SearchReferenceSource::visible).result->references.empty(),"Overflow clears earlier SDK result snapshots while keeping bounded visible prefixes available");
 session.set_search_retention_limits({4,bytes-1});auto byte_overflow=session.search_module({"Text","Original"},{1,1024});check(byte_overflow.generation.retention==SearchRetentionStatus::capacity_exceeded&&byte_overflow.scope.element_count==4,"Accounted SDK text capacity is independent of element count");
 auto failed_only=session.search_batch({{"Missing","Original"},{"Missing","Original"}});check(failed_only.generation_updated&&failed_only.generation.failed_groups==2&&failed_only.generation.retention==SearchRetentionStatus::complete&&!failed_only.generation.matched_elements&&session.read_search_generation(failed_only.generation.id,0).status==SearchGenerationReadStatus::failed_group,"Completed all-error batch replaces old generation with explicit failed groups");
 const auto unchanged=session.search_generation().id;auto batch_stop=std::make_shared<SearchControl>();batch_stop->request_cancel();auto stopped=session.search_batch(queries,{},batch_stop);
 check(!stopped.generation_updated&&stopped.generation.id==unchanged&&!stopped.groups.size(),"Pre-cancelled batch preserves previous generation");
 rejects<std::invalid_argument>([&]{session.search_batch({{"Text","Original"},invalid});});check(session.search_generation().id==unchanged,"Malformed later query is rejected before partial publication");
 rejects<std::invalid_argument>([&]{session.read_search_generation(unchanged,0,static_cast<SearchReferenceSource>(7));});
 check(next_search_generation_id(0)==1,"Generation zero is reserved");rejects<std::overflow_error>([]{next_search_generation_id(std::numeric_limits<std::uint64_t>::max());});
 // SDK-controlled sorting, lifetime and interval semantics are tested without
 // pretending the optional CLucene backend ran on this host.
 EngineManager manager(root.c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);
 std::shared_ptr<const SearchGeneration> sorted;
 {
  OrderedModule module((std::filesystem::path(root)/"data/text").string().c_str());std::shared_ptr<const SearchScope> scope;SearchRetainedKeys retained;
  auto result=capture_module_search(manager,module,{"Text","Original",SearchMode::words},{},{},nullptr,&scope,{}, {},0,100,false,&retained);
  check(std::string(scope->keys().getElement(0)->getText())=="John 1:3"&&std::string(retained.keys->keys().getElement(0)->getText())=="John 1:1","Scope remains pre-sort while complete retained result order is post-sort");
  SearchGenerationBuilder builder(1,{{"Text","Original",SearchMode::words},{"Commentary","Original"}},{});builder.append(0,result,SearchBatchFailure::none,std::move(retained));sorted=builder.finish(true);
  module.result.clear();
 }
 check(sorted->read(1,0,SearchReferenceSource::complete,{}).result->references==std::vector<std::string>{"John 1:1","John 1:3"}&&sorted->read(1,1,SearchReferenceSource::complete,{}).status==SearchGenerationReadStatus::not_attempted&&sorted->read(1,1,SearchReferenceSource::complete,{}).requested->module=="Commentary"&&sorted->info().cancelled,"Retained owned order survives SDK destruction and cancelled unattempted requests retain identity");
 {
  VerseKey parser;auto interval=parser.parseVerseList("John1:1-John1:3","",true);auto keys=retain_search_keys(interval,{},{});
  check(keys.keys->info().element_count==1&&dynamic_cast<const VerseKey *>(keys.keys->keys().getElement(0)),"Retention preserves typed interval rather than reparsing strings");
  SearchResult value;value.requested={"Text","Original"};value.total_results=1;value.executed=true;
  SearchGenerationBuilder builder(1,{value.requested},{});builder.append(0,value,SearchBatchFailure::none,std::move(keys));const auto generation=builder.finish(false);
  check(generation->read(1,0,SearchReferenceSource::complete,{}).result->references==std::vector<std::string>{"John 1:1","John 1:2","John 1:3"}&&generation->info().retained_elements==1,"One retained SDK interval expands to several copied references without losing element semantics");
 }
 {
  auto control=std::make_shared<SearchControl>();SearchControlScope progress(control);OrderedModule module((std::filesystem::path(root)/"data/text").string().c_str());module.indexed=false;module.stop=true;module.control=control;SearchRetainedKeys retained;std::shared_ptr<const SearchScope> scope;
  auto partial=capture_module_search(manager,module,{"Text","Original"},{},control,nullptr,&scope,{}, {},0,100,false,&retained);
  SearchGenerationBuilder builder(1,{partial.requested},{});builder.append(0,partial,SearchBatchFailure::none,std::move(retained));auto generation=builder.finish(partial.cancelled);progress.finish(partial.cancelled);
  check(partial.executed&&partial.cancelled&&generation->info().cancelled&&generation->read(1,0,SearchReferenceSource::complete,{}).result->references==std::vector<std::string>{"John 1:3","John 1:1"}&&!module.terminateSearch&&control->phase()==SearchPhase::cancelled,"Returned stopped SDK groups retain their partial keys and owner state restores");
 }
 for(const auto &text:std::vector<std::string>{std::string(maximum_search_effective_bytes+1,'q'),std::string("bad\0query",9),std::string(1,static_cast<char>(0xff))}){
  ExpandingFilter filter(text);OrderedModule module((std::filesystem::path(root)/"data/text").string().c_str());module.addStripFilter(&filter);auto control=std::make_shared<SearchControl>();
  auto call=[&]{SearchControlScope progress(control);(void)capture_module_search(manager,module,{"Text","Original"},{},control);};
  if(text.size()>maximum_search_effective_bytes)rejects<std::length_error>(call);else rejects<std::runtime_error>(call);
  check(control->phase()==SearchPhase::failed&&!module.terminateSearch,"Oversized or invalid prepared SDK bytes fail before result retention and restore owner state");
 }
 {
  ExpandingFilter filter(std::string(maximum_search_effective_bytes,'q'));OrderedModule module((std::filesystem::path(root)/"data/text").string().c_str());module.addStripFilter(&filter);
  const auto exact_query=capture_module_search(manager,module,{"Text","Original"},{});
  check(exact_query.executed&&exact_query.effective_expression.size()==maximum_search_effective_bytes,"Exact effective-query metadata byte budget is accepted");
 }
 std::future<SearchGenerationRead> drained;
 {
  StudyWorker worker(root);auto a=worker.create_tab().get(),b=worker.create_tab().get();
  auto task=worker.search_module_task(a,{"Text","Original"},{1,1024});auto result=task.result.get();
  check(task.control->phase()==SearchPhase::complete&&worker.search_generation(a).get().id==result.generation.id&&!worker.search_generation(b).get().id,"One FIFO owner publishes generation before terminal success and isolates tabs");
  check(worker.read_search_generation(a,result.generation.id,0).get().result->references==all,"Worker rereads complete copied results on owner");
  (void)worker.set_search_retention_limits(a,{1,1}).get();auto limited=worker.search_module(a,{"Text","Original"}).get();check(limited.generation.retention==SearchRetentionStatus::capacity_exceeded&&worker.search_retention_limits(a).get().max_elements==1&&worker.search_retention_limits(b).get().max_elements==maximum_search_retained_elements,"Future retention policy is FIFO and per tab");
  auto absent=worker.search_generation(999);rejects<std::invalid_argument>([&]{(void)absent.get();});
  worker.close_tab(a).get();auto closed=worker.read_search_generation(a,1,0);rejects<std::invalid_argument>([&]{(void)closed.get();});
  auto saved=worker.search_module(b,{"Text","Original"}).get();drained=worker.read_search_generation(b,saved.generation.id,0);
 }
 check(drained.get().result->references==all&&held.result->references==all,"Copied generation results drain and survive worker/SDK lifetime");
 {
  std::vector<SearchQuery> queries{{"Missing","one"},{"Invalid","two"},{"Engine","three"},{"Empty","four"},{"Stopped","five"},{"Unattempted","six"}};
  SearchGenerationBuilder builder(1,queries,{});
  builder.append(0,std::nullopt,SearchBatchFailure::module_not_found,{});
  builder.append(1,std::nullopt,SearchBatchFailure::invalid_request,{});
  builder.append(2,std::nullopt,SearchBatchFailure::engine_error,{});
  SearchResult empty;empty.requested=queries[3];builder.append(3,empty,SearchBatchFailure::none,{});
  SearchResult stopped;stopped.requested=queries[4];stopped.cancelled=true;builder.append(4,stopped,SearchBatchFailure::none,{});
  const auto summary=builder.finish(true)->export_source_summary();
  check(summary.find("Status: failed (module not found)")!=std::string::npos&&summary.find("Status: failed (invalid request)")!=std::string::npos&&
   summary.find("Status: failed (engine error)")!=std::string::npos&&summary.find("Status: not executed (empty scope)")!=std::string::npos&&
   summary.find("Status: stopped")!=std::string::npos&&summary.find("Group 6\nModule: Unattempted\nQuery: six\nStatus: not attempted")!=std::string::npos,
   "Copied source summary distinguishes every no-row disposition, ordered request identity and unattempted suffix without SDK access");
 }
 std::cout<<"Retained complete search generations, aggregate caps, stale reads, ordering, cancellation and FIFO ownership passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
