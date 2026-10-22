// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/export_item.hpp"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char **argv){try{
 check(argc==2,"Original attribute fixture required");Reader reader(argv[1]);StudySession session(reader);
 session.visit("Text","John1:3");const auto history=session.history().entries().size();
 SearchBatchLimits limits;limits.total.max_results=1;
 auto batch=session.search_batch({{"Text","Original"},{"Commentary","Original"},{"Text","Original"}},limits);
 SearchAllTextExportRequest request;request.generation=batch.generation.id;request.title="Complete returned matches";
 auto all=session.export_search_all_text(request);
 std::string expected=request.title+"\n\n";
 for(const auto &module:{"Text","Commentary","Text"})for(const auto &key:{"John 1:1","John 1:2","John 1:3","John 1:4"})
  expected+=format_export_item(std::string(key)+" "+module,module,key,"",false,false);
 expected+='\n';
 check(all.status==SearchExportStatus::ok&&all.selection==SearchExportSelection::all_retained&&all.text==expected&&all.row_count==12&&all.generation.visible_results==1&&all.generation.retained_elements==12,"Complete export uses every retained ordered group beyond visible prefixes and final scope");
 request.include_search_context=true;auto contextual=session.export_search_all_text(request);
 check(contextual.status==SearchExportStatus::ok&&contextual.row_count==12&&
  contextual.text.find("Search group 1\nModule: Text\nQuery: Original\nMode: Exact phrase")!=std::string::npos&&
  contextual.text.find("Search group 2\nModule: Commentary")!=std::string::npos&&
  contextual.text.find("Search group 3\nModule: Text")!=std::string::npos&&contextual.text.find("John 1:1 Text")!=std::string::npos,
  "Optional all-retained headers preserve group identity and every first match");
 check(contextual.text.find("Source SDK result elements: 4\nSource visible references: 1\nSource visible references were limited;")!=std::string::npos&&
  contextual.text.find("Source SDK result elements: 4\nSource visible references: 0\nSource visible references were limited;")!=std::string::npos,
  "All-retained provenance distinguishes source SDK counts from bounded visible prefixes, including zero-visible groups");
 request.max_bytes=contextual.text.size();check(session.export_search_all_text(request).text==contextual.text,"Exact text context byte budget");
 --request.max_bytes;auto short_context=session.export_search_all_text(request);
 check(short_context.status==SearchExportStatus::output_limit&&short_context.text.empty()&&!short_context.row_count,"One-byte-short context cannot leak partial text");
 request.include_search_context=false;
 request.max_rows=12;request.max_bytes=expected.size();check(session.export_search_all_text(request).text==expected,"Exact aggregate rows and payload bytes accepted");
 request.max_rows=11;auto rows=session.export_search_all_text(request);
 check(rows.status==SearchExportStatus::output_limit&&rows.text.empty()&&!rows.row_count,"Clipped final group rejects all document");
 request.max_rows=12;--request.max_bytes;auto bytes=session.export_search_all_text(request);
 check(bytes.status==SearchExportStatus::output_limit&&bytes.text.empty()&&!bytes.row_count,"Exact whole-document cap is all-or-nothing");
 request.max_bytes=8192;request.include_scripture=true;auto scripture=session.export_search_all_text(request);
 check(scripture.status==SearchExportStatus::ok&&scripture.row_count==12&&scripture.text.find("Original")!=std::string::npos,"Complete service shares configured text assembly");
 auto failed=session.search_batch({{"Text","Original"},{"Missing","Original"},{"Commentary","Original"}},limits);
 request.generation=failed.generation.id;auto refused=session.export_search_all_text(request);
 check(refused.status==SearchExportStatus::source_incomplete&&refused.text.empty()&&refused.generation.failed_groups==1,"Complete retained keys do not imply every requested module succeeded");
 request.allow_partial_source=true;request.include_search_context=true;auto partial=session.export_search_all_text(request);
 check(partial.status==SearchExportStatus::ok&&partial.row_count==8&&partial.generation.failed_groups==1&&partial.generation.requested_count==3&&partial.text.find("only returned matches are included")!=std::string::npos,"Explicit partial-source choice retains failure metadata and exports returned successful groups");
 auto errors=session.search_batch({{"Missing","Original"}},limits);request.generation=errors.generation.id;
 auto no_rows=session.export_search_all_text(request);
 check(no_rows.status==SearchExportStatus::ok&&!no_rows.row_count&&no_rows.text.find("only returned matches are included")!=std::string::npos&&no_rows.generation.failed_groups==1,"Explicit all-failed source acceptance returns labelled empty document, not old results");
 request.include_search_context=false;
 auto default_empty=session.export_search_all_text(request);
 check(default_empty.status==SearchExportStatus::ok&&default_empty.text==request.title+"\n\n\n",
  "Opting out preserves the original exact empty-document bytes even for accepted failed sources");
 auto exact=session.search_batch({{"Text","Original"},{"Commentary","NoMatchOriginalFixture"}},limits);
 request={};request.generation=exact.generation.id;request.max_rows=4;
 check(session.export_search_all_text(request).status==SearchExportStatus::ok&&session.export_search_all_text(request).row_count==4,"Empty later group at exhausted exact row budget is accepted");
 auto empty=session.search_module({"Text","NoMatchOriginalFixture"});request.generation=empty.generation.id;
 check(session.export_search_all_text(request).status==SearchExportStatus::ok&&session.export_search_all_text(request).row_count==0,"Successfully empty search has complete empty export");
 const auto scope=session.last_search_scope();session.clear_search_text_options();
 check(session.history().entries().size()==history&&session.search_generation().id==empty.generation.id&&session.last_search_scope().element_count==scope.element_count&&!session.book_selection(),"Complete export changes no history, selection or search state");
 session.set_search_retention_limits({1,1024});auto overflow=session.search_module({"Text","Original"});request.generation=overflow.generation.id;
 check(session.export_search_all_text(request).status==SearchExportStatus::capacity_exceeded,"Unavailable retention cannot export visible prefix as complete");
 check(session.export_search_visible_text({overflow.generation.id,{{0,0}},"Visible",false,1024}).status==SearchExportStatus::ok,"Visible fallback remains explicitly selectable");
 request.generation=empty.generation.id;check(session.export_search_all_text(request).status==SearchExportStatus::stale,"Stale complete source rejected");request.generation=overflow.generation.id;
 auto control=std::make_shared<SearchControl>();control->request_cancel();auto stopped=session.export_search_all_text(request,control);
 check(stopped.status==SearchExportStatus::cancelled&&stopped.text.empty()&&control->phase()==SearchPhase::cancelled,"Pre-cancel prevents retention/read/document work");
 for(auto count:{std::size_t{0},maximum_search_export_rows+1}){request.max_rows=count;bool rejected=false;try{session.export_search_all_text(request);}catch(const std::invalid_argument &){rejected=true;}check(rejected,"Invalid complete row budget");}
 std::future<SearchTextExport> drained;SearchTextExport held;
 {StudyWorker worker(argv[1]);const auto tab=worker.create_tab().get();auto seed=worker.search_batch(tab,{{"Text","Original"},{"Commentary","Original"}},limits).get();
  SearchAllTextExportRequest copied;copied.generation=seed.generation.id;
  auto task=worker.export_search_all_text_task(tab,copied);copied.max_rows=1;held=task.result.get();
  check(held.status==SearchExportStatus::ok&&held.row_count==8&&task.control->phase()==SearchPhase::complete,"Worker copies all-export options and serializes retained SDK traversal");
  drained=worker.export_search_all_text(tab,{seed.generation.id,"Drained complete",true,8192,8,false});
 }
 check(drained.get().status==SearchExportStatus::ok&&held.row_count==8&&!held.text.empty(),"Worker destruction drains accepted complete export; owned output survives");
 std::cout<<"Complete retained text export, partial-source choice, aggregate caps, empty groups and owner lifetime passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
