// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/export_item.hpp"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F action){try{action();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected invalid export request");}
int main(int argc,char **argv){try{
 check(argc==2,"Original attribute fixture required");Reader reader(argv[1]);StudySession session(reader),other(reader);
 session.visit("Text","John1:3");const auto history=session.history().entries();const auto cursor=session.history().cursor();
 SearchTextExportRequest request;request.rows={{0,0}};
 check(session.export_search_visible_text(request).status==SearchExportStatus::stale,"No generation rejects instead of reading current key");
 SearchBatchLimits limits;limits.total.max_results=3;
 auto batch=session.search_batch({{"Text","Original"},{"Missing","Original"},{"Commentary","Original"}},limits);
 const auto generation=batch.generation;request.generation=generation.id;
 const auto last=session.last_search_scope();const auto text=session.search_text_options();
 request.rows={{0,2},{0,0},{0,2}};request.title="Original selected results";
 auto plain=session.export_search_visible_text(request);
 const auto expected=request.title+"\n\n"+format_export_item("John 1:3 Text","Text","John 1:3","",false,false)+
  format_export_item("John 1:1 Text","Text","John 1:1","",false,false)+format_export_item("John 1:3 Text","Text","John 1:3","",false,false)+"\n";
 check(plain.status==SearchExportStatus::ok&&plain.text==expected&&plain.row_count==3&&!plain.includes_scripture&&
  plain.generation.id==generation.id,"Text payload preserves first selected row, order and duplicates with exact shared formatting");
 request.include_search_context=true;request.max_bytes=4096;
 const auto contextual=session.export_search_visible_text(request);
 check(contextual.status==SearchExportStatus::ok&&contextual.row_count==3&&
  contextual.text.find("Source SDK result elements: 4\nSource visible references: 3\nSource visible references were limited;")!=std::string::npos,
  "Selected rows and duplicates do not replace the captured source match/preview counts");
 request.include_search_context=false;
 request.max_bytes=expected.size();check(session.export_search_visible_text(request).text==expected,"Exact byte cap accepted");
 --request.max_bytes;auto overflow=session.export_search_visible_text(request);
 check(overflow.status==SearchExportStatus::output_limit&&overflow.text.empty()&&!overflow.row_count,"One byte below cap has no partial payload");
 request.max_bytes=1;check(session.export_search_visible_text(request).status==SearchExportStatus::output_limit,"Header budget checked before append");
 request.max_bytes=1024;request.rows={{0,0},{2,0}};
 auto invalid=session.export_search_visible_text(request);
 check(invalid.status==SearchExportStatus::invalid_selection&&invalid.failed_row==1&&invalid.text.empty(),"Omitted visible group cannot silently recover all retained results");
 request.rows={{1,0}};check(session.export_search_visible_text(request).status==SearchExportStatus::invalid_selection,"Failed group has no selectable row");
 request.rows={{0,3}};check(session.export_search_visible_text(request).status==SearchExportStatus::invalid_selection,"Visible row bound enforced");
 request.rows={{0,0}};check(other.export_search_visible_text(request).status==SearchExportStatus::stale,"Origin tab required");
 request.include_scripture=true;auto with=session.export_search_visible_text(request);
 const auto stripped=reader.read_stripped("Text","John 1:1",session.module_engine_options("Text"),1024);
 check(with.status==SearchExportStatus::ok&&with.includes_scripture&&with.text==request.title+"\n\n"+
  format_export_item("John 1:1 Text","Text","John 1:1",stripped,false)+"\n"&&stripped.find("Original")!=std::string::npos&&stripped.find("<w")==std::string::npos,"Scripture uses SDK stripped text, not HTML removal approximation");
 request.max_bytes=with.text.size();check(session.export_search_visible_text(request).text==with.text,"Exact scripture cap accepted");
 --request.max_bytes;check(session.export_search_visible_text(request).status==SearchExportStatus::output_limit&&session.export_search_visible_text(request).text.empty(),"Scripture over cap never publishes truncated text");
 request.max_bytes=1024;
 EngineOptions configured;configured.strongs=true;configured.footnotes=true;session.set_engine_options(configured);
 const auto before=other.read_entry("Text","John 1:1").html;
 const auto configured_text=session.export_search_visible_text(request);
 check(configured_text.status==SearchExportStatus::ok&&configured_text.text.find(reader.read_stripped("Text","John 1:1",session.module_engine_options("Text"),1024))!=std::string::npos&&
  other.read_entry("Text","John 1:1").html==before,"Configured reading engine options apply and physical filters restore across sessions");
 auto stopped=std::make_shared<SearchControl>();stopped->request_cancel();auto cancelled=session.export_search_visible_text(request,stopped);
 check(cancelled.status==SearchExportStatus::cancelled&&cancelled.text.empty()&&!cancelled.row_count&&stopped->phase()==SearchPhase::cancelled,"Pre-cancel yields no document");
 check(session.history().entries().size()==history.size()&&session.history().cursor()==cursor&&session.history().entries()[0].reference==history[0].reference&&
  session.search_generation().id==generation.id&&session.last_search_scope().element_count==last.element_count&&
  session.search_text_options().options.has_value()==text.options.has_value()&&!session.book_selection(),"Export success/failures do not navigate, replace search or select book");
 request.rows.clear();rejects([&]{session.export_search_visible_text(request);});request.rows={{0,0}};
 request.max_bytes=0;rejects([&]{session.export_search_visible_text(request);});request.max_bytes=maximum_search_export_bytes+1;rejects([&]{session.export_search_visible_text(request);});request.max_bytes=1024;
 request.title=std::string("bad\0tail",8);rejects([&]{session.export_search_visible_text(request);});request.title=std::string(1,static_cast<char>(0xff));rejects([&]{session.export_search_visible_text(request);});request.title=std::string(32769,'x');rejects([&]{session.export_search_visible_text(request);});request.title="Results";
 request.rows.resize(maximum_search_export_rows+1);rejects([&]{session.export_search_visible_text(request);});request.rows={{0,0}};
 session.clear_search_scope();session.clear_search_text_options();
 check(session.export_search_visible_text(request).status==SearchExportStatus::ok,"Scope/context reset independent from visible export");
 session.set_search_retention_limits({1,1024});auto next=session.search_module({"Text","Original"});
 check(session.export_search_visible_text(request).status==SearchExportStatus::stale,"Old selection cannot export new generation");
 request.generation=next.generation.id;
 check(next.generation.retention==SearchRetentionStatus::capacity_exceeded&&session.export_search_visible_text(request).status==SearchExportStatus::ok,"Visible export available after complete-retention overflow");
 auto worker=std::make_unique<StudyWorker>(argv[1]);const auto tab=worker->create_tab().get();auto searched=worker->search_module(tab,{"Commentary","Original"}).get();
 SearchTextExportRequest copied;copied.generation=searched.generation.id;copied.rows={{0,1},{0,0}};copied.include_scripture=true;
 auto queued=worker->export_search_visible_text(tab,copied);copied.rows.clear();copied.title="Changed";
 auto owned=queued.get();check(owned.status==SearchExportStatus::ok&&owned.row_count==2&&owned.text.find("Commentary John 1:2")<owned.text.find("Commentary John 1:1"),"Worker copies ordered selection and reads commentary on owner");
 auto task=worker->export_search_visible_text_task(tab,{searched.generation.id,{{0,0}},"Race",true,1024});task.control->request_cancel();
 auto race=task.result.get();check((race.status==SearchExportStatus::ok||race.status==SearchExportStatus::cancelled)&&
  (race.status==SearchExportStatus::ok?task.control->phase()==SearchPhase::complete:race.text.empty()&&task.control->phase()==SearchPhase::cancelled),"Cooperative cancel/completion race returns authoritative disposition");
 auto drained=worker->export_search_visible_text(tab,{searched.generation.id,{{0,0}},"Drained",false,1024});worker->close_tab(tab).get();worker.reset();
 check(drained.get().status==SearchExportStatus::ok&&owned.text.find("Original")!=std::string::npos,"Accepted export drains before tab/worker destruction and owns payload");
 std::cout<<"Selected visible search text export, exact budgets, SDK scripture, cancellation and state neutrality passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
