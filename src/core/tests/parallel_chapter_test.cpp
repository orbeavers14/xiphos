// SPDX-License-Identifier: GPL-2.0-or-later
#include "parallel_interpolation_host.hpp"
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
using namespace xiphos::core;
template<class E,class F>void rejects(F f){try{f();}catch(const E &){return;}throw std::runtime_error("Expected parallel rejection");}
static xc_status wait(xc_request *request){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(30);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Parallel deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Original parallel fixture required");const std::string root=argv[1];std::size_t comparisons=0;ParallelChapter owned;
 {Reader reader(root);SWMgr baseline(root.c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);BackEnd old(baseline);
  for(const auto &ids:std::vector<std::vector<std::string>>{{"TextA","TextAlt"},{"TextAlt","TextA"},{"RtlText","TextA"},{"TextA","Missing","TextA"},{"Notes","TextA"},{"TextA","RtlText","Notes"}})
  for(const auto *reference:{"John3:16","Psalms3:2","Genesis1:1"}){
   const auto expected=run(frozen::interpolate_parallel_display,old,ids,reference,true,false,true,nullptr);
   owned=reader.read_parallel(ids,reference,{},true);std::size_t at=0;
   check(owned.columns.size()==ids.size()&&owned.selected_verse==static_cast<unsigned>(expected.selected),"Native parallel shape/anchor agrees with full frozen interpolation");
   for(const auto &row:owned.rows)for(std::size_t col=0;col<ids.size();++col){
    const auto &cell=row.cells[col];if(owned.columns[col].status!=ParallelColumnStatus::ready){check(cell.html.empty(),"Missing columns preserve empty owned slot");continue;}
    const auto &source=expected.cells.at(at++);check(cell.html==source.html&&cell.reference==source.matched&&row.reference==source.requested&&owned.columns[col].module==source.module,"Native cell content/references agree with complete retained SDK renderer");
   }
   check(at==expected.cells.size(),"All retained cells consumed exactly");++comparisons;
  }
  const auto aliasSource=baseline.getModule("TextA")->getConfigEntry("Abbreviation");
  const auto resolvedAlias=reader.resolve_module_name("TextAAlias");
  check(aliasSource&&std::string(aliasSource)=="TextAAlias"&&resolvedAlias.module=="TextA","Fresh disposable catalog exposes configured abbreviation");
  auto alias=reader.read_parallel({"TextAAlias","Missing","Dictionary","TextAAlias",""},"John3:16",{},true);
  check(alias.columns.size()==5&&alias.columns[0].requested_module=="TextAAlias"&&alias.columns[0].module=="TextA"&&alias.columns[0].via_abbreviation&&alias.columns[1].status==ParallelColumnStatus::missing&&alias.columns[2].status==ParallelColumnStatus::unsupported&&alias.columns[3].module=="TextA"&&alias.columns[4].status==ParallelColumnStatus::missing,"Copied alias/missing/unsupported/duplicate/empty column provenance");
  check(!alias.rows.at(15).cells[0].headings.empty()&&alias.rows.at(15).cells[0].headings[0].find("Original TextA heading")!=std::string::npos,"Alias uses actual owned module for heading source, correcting retained raw-token title loss");
  check(reader.read_parallel({"RtlText","TextA"},"Psalms3:2",{},false).columns[0].right_to_left,"Copied RTL module capability");
  (void)reader.read("TextA","John3:23");const auto before=reader.navigation_target("TextA","24").reference;
  const auto relative=reader.read_parallel({"TextA","TextAlt"},"24",{},false);
  const auto relativeTarget=reader.navigation_target("TextA","24");
  check(relative.reference==relativeTarget.reference&&relative.selected_verse>0&&relative.rows.size()>=relative.selected_verse,"Bare numeric input follows existing SDK navigation interpretation and a consistent control chapter");
  rejects<std::invalid_argument>([&]{(void)reader.read_parallel({"TextA","TextAlt"},"John3:16",{},true,1);});
  check(reader.navigation_target("TextA","24").reference==before,"Limit failure preserves reading module key");
  auto held=reader.read_parallel({"TextA","TextAlt"},"John3:16",{},true);check(held.snapshot_bytes>0,"Account owned string bytes");
  check(reader.read_parallel({"TextA","TextAlt"},"John3:16",{},true,held.snapshot_bytes).snapshot_bytes==held.snapshot_bytes,"Exact aggregate byte limit succeeds");
  rejects<std::invalid_argument>([&]{(void)reader.read_parallel({"TextA","TextAlt"},"John3:16",{},true,held.snapshot_bytes-1);});
  check(reader.navigation_target("TextA","24").reference==before,"Successful and late overflow reads preserve live module key");
  const auto wide=reader.read_parallel(std::vector<std::string>(maximum_parallel_columns,"TextA"),"John3:16",{},false);
  check(wide.columns.size()==maximum_parallel_columns&&wide.rows.at(15).cells.front().html==wide.rows.at(15).cells.back().html,"Exact column bound preserves all duplicate slots without truncation");
  rejects<std::out_of_range>([&]{(void)reader.read_parallel({std::string(maximum_parallel_module_bytes,'m')},"John3:16",{},false);});
  rejects<std::out_of_range>([&]{(void)reader.read_parallel({"Missing","TextA"},"John3:16",{},true);});
  rejects<std::invalid_argument>([&]{(void)reader.read_parallel({"Dictionary","TextA"},"John3:16",{},true);});
  rejects<std::invalid_argument>([&]{(void)reader.read_parallel({"TextA"},"not-a-reference",{},true);});
 }
 check(!owned.rows.empty()&&!owned.rows[0].cells.empty(),"Owned chapter survives Reader close");
 std::future<ParallelChapter> accepted;
 {StudyWorker worker(root);const auto tab=worker.create_tab().get(),other=worker.create_tab().get();
  (void)worker.visit_passage(tab,"TextA","Notes","John3:17").get();const auto history=worker.history(tab).get();const auto generation=worker.search_generation(tab).get().id;
  EngineOptions mainOptions;mainOptions.strongs=true;worker.set_engine_options(tab,mainOptions).get();
  const auto chapter=worker.read_parallel(tab,{"TextA","TextAlt"},"John3:16",{},true).get();check(chapter.rows.size()==36,"Worker-owned full chapter");
  auto bad=worker.read_parallel(tab,std::vector<std::string>(maximum_parallel_columns+1,"TextA"),"John3:16",{},true);check(bad.wait_for(std::chrono::seconds(0))==std::future_status::ready,"Oversized columns rejected before FIFO");rejects<std::invalid_argument>([&]{(void)bad.get();});
  auto nul=worker.read_parallel(tab,{"TextA"},std::string("John3:16\0tail",13),{},true);check(nul.wait_for(std::chrono::seconds(0))==std::future_status::ready,"NUL reference rejected before FIFO");rejects<std::invalid_argument>([&]{(void)nul.get();});
  rejects<std::invalid_argument>([&]{(void)worker.read_parallel(tab,{"TextA"},"John3:16",{},true,1).get();});
  check(worker.module_engine_options(tab,"TextA").get()->strongs,"Explicit parallel options do not replace main tab options");
  check(worker.history(tab).get().entries.size()==history.entries.size()&&worker.history(other).get().entries.empty()&&worker.dictionary_history(tab).get().entries.empty()&&worker.search_generation(tab).get().id==generation,"Parallel reads preserve history/search/sibling state");
  check(!worker.parallel_selection(tab).get(),"Read-only parallel snapshots do not establish selection");
  (void)worker.visit_parallel(tab,{"TextAAlias","TextAlt"},"John3:16",{},true).get();
  auto selection=worker.parallel_selection(tab).get();check(selection&&selection->modules.front()=="TextAAlias"&&selection->control_module=="TextA"&&selection->reference=="John 3:16"&&selection->headings&&!selection->options.strongs,"Committed parallel selection owns raw intent and actual control independently of main options");
  (void)worker.read_parallel(tab,{"RtlText"},"Psalms3:2",{},false).get();check(worker.parallel_selection(tab).get()->reference==selection->reference,"Read-only previews preserve committed selection");
  rejects<std::out_of_range>([&]{(void)worker.visit_parallel(tab,{"Missing","TextA"},"John3:17",{},true).get();});
  rejects<std::invalid_argument>([&]{(void)worker.visit_parallel(tab,{"TextA"},"John3:17",{},true,1).get();});
  check(worker.parallel_selection(tab).get()->reference==selection->reference,"Failed visits retain last committed parallel location");
  check(worker.navigate_parallel_chapter(tab,1).get().reference=="John 4:1"&&worker.navigate_parallel_chapter(tab,-1).get().reference=="John 3:1","Parallel adjacent chapters use committed control and existing chapter policy");
  (void)worker.visit_parallel(tab,{"TextA"},"Genesis1:1",{},false).get();
  rejects<std::invalid_argument>([&]{(void)worker.navigate_parallel_chapter(tab,-1).get();});
  check(worker.parallel_selection(tab).get()->reference=="Genesis 1:1","Unavailable chapter boundary preserves prior selection");
  (void)worker.visit_parallel(other,{"RtlText"},"Psalms3:2",{},false).get();worker.clear_parallel_selection(tab).get();
  check(!worker.parallel_selection(tab).get()&&worker.parallel_selection(other).get()->control_module=="RtlText"&&worker.history(tab).get().entries.size()==history.entries.size()&&worker.search_generation(tab).get().id==generation,"Parallel state and clearing are independent across tabs and main/search history");
  rejects<std::invalid_argument>([&]{(void)worker.navigate_parallel_chapter(tab,1).get();});
  std::vector<std::string> ids={"TextAAlias","TextAlt"};accepted=worker.read_parallel(tab,ids,"John3:16",{},true);ids.clear();worker.close_tab(tab).get();
 }
 owned=accepted.get();check(owned.columns[0].module=="TextA"&&!owned.rows[15].cells[0].html.empty(),"Accepted parallel copies drain after tab/worker close");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C parallel worker");const auto tab=xc_request_tab(request);xc_request_release(request);
 const char *ids[]={"TextAAlias","Missing","Dictionary","RtlText","TextAAlias"};
 check(xc_worker_read_parallel(worker,tab,ids,5,"John3:17",0,0,XC_PARALLEL_MAX_SNAPSHOT_BYTES,&request)==XC_OK&&wait(request)==XC_OK,"C read-only parallel preview");xc_parallel_chapter_release(xc_request_take_parallel_chapter(request));xc_request_release(request);
 check(xc_worker_parallel_selection(worker,tab,&request)==XC_OK&&wait(request)==XC_OK,"C absent parallel selection");auto *emptySelection=xc_request_take_parallel_selection(request);check(emptySelection&&!xc_parallel_selection_present(emptySelection),"C read-only operation leaves selection absent");xc_parallel_selection_release(emptySelection);xc_request_release(request);
 check(xc_worker_visit_parallel(worker,tab,ids,5,"John3:17",XC_ENGINE_STRONGS,0,XC_PARALLEL_MAX_SNAPSHOT_BYTES,&request)==XC_OK&&wait(request)==XC_OK,"C initial committed visit");xc_parallel_chapter_release(xc_request_take_parallel_chapter(request));xc_request_release(request);
 check(xc_worker_navigate_parallel_chapter(worker,tab,1,&request)==XC_OK&&wait(request)==XC_OK,"C adjacent parallel chapter");auto *next=xc_request_take_parallel_chapter(request);check(std::string(xc_parallel_reference(next))=="John 4:1","C chapter navigation result");xc_parallel_chapter_release(next);xc_request_release(request);
 const char *missing[]={"Missing","TextA"};check(xc_worker_visit_parallel(worker,tab,missing,2,"John3:16",0,1,XC_PARALLEL_MAX_SNAPSHOT_BYTES,&request)==XC_OK&&wait(request)==XC_MODULE_NOT_FOUND,"C failed visit status");check(!xc_request_take_parallel_chapter(request),"C failed visit publishes no partial snapshot");xc_request_release(request);
 check(xc_worker_parallel_selection(worker,tab,&request)==XC_OK&&wait(request)==XC_OK,"C state after failed visit");auto *retained=xc_request_take_parallel_selection(request);check(std::string(xc_parallel_selection_reference(retained))=="John 4:1"&&xc_parallel_selection_engine_flags(retained)==XC_ENGINE_STRONGS&&!xc_parallel_selection_headings(retained),"C failure retains prior committed reference/options");xc_parallel_selection_release(retained);xc_request_release(request);
 check(xc_worker_clear_parallel_selection(worker,tab,&request)==XC_OK&&wait(request)==XC_OK,"C clear parallel state");xc_request_release(request);
 check(xc_worker_parallel_selection(worker,tab,&request)==XC_OK&&wait(request)==XC_OK,"C inspect cleared state");auto *cleared=xc_request_take_parallel_selection(request);check(!xc_parallel_selection_present(cleared),"C cleared state absent");xc_parallel_selection_release(cleared);xc_request_release(request);
 check(xc_worker_visit_parallel(worker,tab,ids,5,"John3:16",0,1,XC_PARALLEL_MAX_SNAPSHOT_BYTES,&request)==XC_OK,"C parallel visit accepts");ids[0]="Dictionary";
 xc_request *selectionRequest=nullptr;check(xc_worker_parallel_selection(worker,tab,&selectionRequest)==XC_OK,"C selection queued behind visit");xc_worker_close(worker);check(wait(request)==XC_OK&&wait(selectionRequest)==XC_OK,"C parallel visit/state drain");auto *chapter=xc_request_take_parallel_chapter(request);
 auto *saved=xc_request_take_parallel_selection(selectionRequest);check(saved&&!xc_request_take_parallel_selection(selectionRequest),"C selection takes once");xc_request_release(selectionRequest);
 check(xc_parallel_selection_present(saved)&&xc_parallel_selection_module_count(saved)==5&&std::string(xc_parallel_selection_module(saved,0))=="TextAAlias"&&std::string(xc_parallel_selection_control(saved))=="TextA"&&std::string(xc_parallel_selection_reference(saved))=="John 3:16"&&xc_parallel_selection_headings(saved)&&!xc_parallel_selection_engine_flags(saved)&&xc_parallel_selection_max_bytes(saved)==XC_PARALLEL_MAX_SNAPSHOT_BYTES,"C owned committed state survives request/owner and retains copied intent");xc_parallel_selection_release(saved);
 check(chapter&&!xc_request_take_parallel_chapter(request)&&!xc_request_take_entry(request),"Distinct C parallel handle takes once");xc_request_release(request);
 check(std::string(xc_parallel_control_module(chapter))=="TextA"&&std::string(xc_parallel_reference(chapter))=="John 3:16"&&xc_parallel_selected_verse(chapter)==16&&xc_parallel_column_count(chapter)==5&&xc_parallel_row_count(chapter)==36&&xc_parallel_snapshot_bytes(chapter)>0,"C chapter metadata survives owner");
 check(std::string(xc_parallel_column_requested(chapter,0))=="TextAAlias"&&std::string(xc_parallel_column_module(chapter,0))=="TextA"&&xc_parallel_column_via_abbreviation(chapter,0)&&xc_parallel_column_is_bible(chapter,0)&&xc_parallel_column_is_rtl(chapter,3)&&xc_parallel_column_state(chapter,1)==XC_PARALLEL_MISSING&&xc_parallel_column_state(chapter,2)==XC_PARALLEL_UNSUPPORTED,"C ordered column provenance/capabilities");
 check(xc_parallel_row_ordinal(chapter,15)==16&&std::string(xc_parallel_row_reference(chapter,15))=="John 3:16"&&std::string(xc_parallel_cell_reference(chapter,15,0))=="John 3:16"&&std::string(xc_parallel_cell_html(chapter,15,0)).find("Original TextA")!=std::string::npos&&xc_parallel_cell_heading_count(chapter,15,0)>0&&xc_parallel_cell_heading(chapter,15,0,0),"C owned row/cell/heading values");
 check(!xc_parallel_cell_html(chapter,15,1)&&!xc_parallel_cell_html(chapter,15,2)&&!xc_parallel_cell_html(chapter,999,0)&&!xc_parallel_cell_heading(chapter,15,0,999)&&xc_parallel_column_state(chapter,99)==XC_PARALLEL_INVALID,"C missing/unsupported/bounds access");xc_parallel_chapter_release(chapter);
 check(!xc_parallel_control_module(nullptr)&&!xc_parallel_reference(nullptr)&&!xc_parallel_title(nullptr)&&!xc_parallel_row_count(nullptr)&&!xc_parallel_column_count(nullptr)&&!xc_parallel_selected_verse(nullptr)&&!xc_parallel_cell_heading_count(nullptr,0,0)&&xc_parallel_column_state(nullptr,0)==XC_PARALLEL_INVALID,"C null safety");xc_parallel_chapter_release(nullptr);
 check(!xc_parallel_selection_present(nullptr)&&!xc_parallel_selection_module_count(nullptr)&&!xc_parallel_selection_module(nullptr,0)&&!xc_parallel_selection_control(nullptr)&&!xc_parallel_selection_reference(nullptr)&&!xc_parallel_selection_headings(nullptr)&&!xc_parallel_selection_engine_flags(nullptr)&&!xc_parallel_selection_max_bytes(nullptr),"C null selection getters");xc_parallel_selection_release(nullptr);
 std::cout<<"Owned parallel chapters: "<<comparisons<<" complete retained matrix comparisons, aliases/headings, missing slots, bounded snapshots, isolation and C lifetime passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
