// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "image_source_test_support.hpp"
#include <glib.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void empty_failure(const SearchHtmlExport &out,SearchExportStatus status){
 check(out.status==status&&out.html.empty()&&!out.row_count&&!out.images.count&&!out.images.unique_count&&!out.images.original_bytes&&!out.images.pixels,"Failure publishes neither partial document nor asset statistics");
}
std::size_t ordinal(const SearchBatchResult &search,std::size_t group,const char *reference){
 const auto &refs=search.groups.at(group).result->references;const auto found=std::find(refs.begin(),refs.end(),reference);
 check(found!=refs.end(),"Original image verse must be present in the copied search prefix");return static_cast<std::size_t>(found-refs.begin());
}
int main(int argc,char **argv){try{
 check(argc==2,"Original image-source fixture required");
 const auto root=(std::filesystem::path(argv[1])/std::filesystem::u8path("library %20 café")).string();
 SearchHtmlExport held;
 {
  Reader reader(root);StudySession session(reader);session.enable_module_preferences();session.visit("ThMLText","John1:2");
  const auto search=session.search_batch({{"ThMLText","Original"},{"OSISNotes","Original"},{"OSISText","Original"},{"ThMLNotes","Original"}});
  check(search.groups.size()==4,"All four actual SDK image families searched");
  const auto history=session.history().entries().size();const auto before=session.module_presentation_options("ThMLText");
  SearchHtmlExportRequest request;request.generation=search.generation.id;request.include_scripture=true;
  const auto a=ordinal(search,0,"John 1:8"),b=ordinal(search,1,"John 1:8");request.rows={{0,a},{1,b},{0,a}};
  auto rejected=session.export_search_html(request);empty_failure(rejected,SearchExportStatus::unsupported_html);
  check(rejected.issue==HtmlExportIssue::resources,"Default HTML policy still rejects image-dependent sources");
  request.images.policy=SearchHtmlImagePolicy::embedded_static_png;request.images.max_images=3;
  request.images.max_original_bytes=150;request.images.max_pixels=128;
  held=session.export_search_html(request);
  check(held.status==SearchExportStatus::ok&&held.row_count==3&&held.images.count==3&&held.images.unique_count==2&&held.images.original_bytes==150&&held.images.pixels==128,"Shared document includes ordered duplicate occurrences with unique module-owned resource accounting");
  check(held.html.find("img-src data:")!=std::string::npos&&held.html.find("default-src 'none'")!=std::string::npos,"Opt-in document permits embedded data images while retaining default resource denial");
  const auto urls=image_test::images(held.html);check(urls.size()==3&&urls[0]==urls[1]&&urls[1]==urls[2],"Each original valid PNG becomes the same self-contained payload");
  for(const auto &url:urls){
   const std::string prefix="data:image/png;base64,";check(url.rfind(prefix,0)==0,"Only the fixed validated PNG data URL is embedded");
   gsize length=0;std::unique_ptr<guchar,decltype(&g_free)> decoded(g_base64_decode(url.substr(prefix.size()).c_str(),&length),g_free);
   check(std::string(reinterpret_cast<const char *>(decoded.get()),length)==image_test::preview_png(),"Encoded document preserves complete original PNG bytes");
  }
  request.max_bytes=held.html.size();check(session.export_search_html(request).html==held.html,"Exact assembled HTML image document byte limit accepted");
  --request.max_bytes;empty_failure(session.export_search_html(request),SearchExportStatus::output_limit);request.max_bytes=1024*1024;
  for(unsigned budget=0;budget<3;++budget){
   auto limited=request;if(budget==0)--limited.images.max_images;if(budget==1)--limited.images.max_original_bytes;if(budget==2)--limited.images.max_pixels;
   const auto out=session.export_search_html(limited);empty_failure(out,SearchExportStatus::output_limit);
   check(out.failed_row==std::make_optional<std::size_t>(budget==0?2:1),"Image budget reports the selected row that exhausted its aggregate limit");
  }
  for(std::size_t group=0;group<4;++group){
   request.rows={{group,ordinal(search,group,"John 1:8")}};
   check(session.export_search_html(request).status==SearchExportStatus::ok,"Actual ThML/OSIS Bible/commentary inline image sources load through the shared reader");
   for(const char *reference:{"John 1:1","John 1:5","John 1:6","John 1:7"}){
    request.rows={{group,ordinal(search,group,reference)}};const auto out=session.export_search_html(request);
    empty_failure(out,SearchExportStatus::unsupported_html);check(out.issue==HtmlExportIssue::resources&&out.failed_row==0,"Invalid PNG, missing, parent and external image source fail explicitly without an artifact");
   }
  }
  request.rows={{0,a},{0,ordinal(search,0,"John 1:5")}};
  empty_failure(session.export_search_html(request),SearchExportStatus::unsupported_html);
  request.rows={{0,a}};
  auto stopped=std::make_shared<SearchControl>();stopped->request_cancel();empty_failure(session.export_search_html(request,stopped),SearchExportStatus::cancelled);
  auto stale=request;++stale.generation;empty_failure(session.export_search_html(stale),SearchExportStatus::stale);
  for(unsigned shape=0;shape<5;++shape){
   auto invalid=request;if(shape==0)invalid.images.policy=static_cast<SearchHtmlImagePolicy>(99);if(shape==1)invalid.images.max_images=0;
   if(shape==2)invalid.images.max_images=maximum_search_export_images+1;if(shape==3)invalid.images.max_original_bytes=0;if(shape==4)invalid.images.max_pixels=maximum_search_export_image_pixels+1;
   bool threw=false;try{session.export_search_html(invalid);}catch(const std::invalid_argument &){threw=true;}check(threw,"Invalid image policy/aggregate budgets reject before work");
  }
  request.include_scripture=false;const auto references=session.export_search_html(request);
  check(references.status==SearchExportStatus::ok&&!references.images.count&&image_test::images(references.html).empty(),"Reference-only export does not load scripture resources");request.include_scripture=true;
  check(session.history().entries().size()==history&&session.search_generation().id==search.generation.id&&session.module_presentation_options("ThMLText").image_content==before.image_content&&!session.book_selection(),"Resource export preserves logical history/generation/profile/book state");
  session.set_image_content("ThMLText",false);const auto off=session.export_search_html(request);
  check(off.status==SearchExportStatus::ok&&!off.images.count&&image_test::images(off.html).empty(),"Explicit existing image-off preference takes precedence over export opt-in");
  session.set_image_content("ThMLText",true);
  request.selection=SearchExportSelection::all_retained;request.rows.clear();empty_failure(session.export_search_html(request),SearchExportStatus::unsupported_html);
  const auto valid=session.search_batch({{"ThMLText","PNG"},{"OSISNotes","PNG"}});
  request.generation=valid.generation.id;const auto all=session.export_search_html(request);
  check(all.status==SearchExportStatus::ok&&all.row_count==2&&all.images.count==2&&all.images.unique_count==2&&all.images.original_bytes==150,"All-retained mode uses the same aggregate image policy without dropping source rows");
  const auto partial=session.search_batch({{"ThMLText","PNG"},{"Missing","PNG"}});request.generation=partial.generation.id;
  empty_failure(session.export_search_html(request),SearchExportStatus::source_incomplete);request.allow_partial_source=true;
  const auto accepted=session.export_search_html(request);check(accepted.status==SearchExportStatus::ok&&accepted.row_count==1&&accepted.images.count==1&&accepted.html.find("only returned matches are included")!=std::string::npos,"Explicit partial-source image document remains labelled with its original search disposition");
 }
 std::future<SearchHtmlExport> pending;
 {StudyWorker worker(root);const auto tab=worker.create_tab().get();const auto search=worker.search_module(tab,{"ThMLText","Original"}).get();
  const auto &refs=search.references;const auto found=std::find(refs.begin(),refs.end(),"John 1:8");check(found!=refs.end(),"Worker search retains original PNG row");
  SearchHtmlExportRequest request;request.generation=search.generation.id;request.include_scripture=true;
  request.rows={{0,static_cast<std::size_t>(found-refs.begin())}};request.images.policy=SearchHtmlImagePolicy::embedded_static_png;
  pending=worker.export_search_html(tab,request);request.images.max_pixels=1;request.rows.clear();worker.close_tab(tab).get();}
 const auto drained=pending.get();check(drained.status==SearchExportStatus::ok&&drained.images.count==1&&!held.html.empty(),"Accepted copied image export drains and owned payloads survive worker shutdown");
 std::cout<<"Real module-owned PNG document export, defaults, aggregate limits, failure atomicity and copied worker lifetime passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
