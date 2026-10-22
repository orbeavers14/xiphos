// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include <libxml/HTMLparser.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <memory>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::size_t count(const std::string &text,const std::string &part){std::size_t n=0,p=0;while((p=text.find(part,p))!=std::string::npos){++n;p+=part.size();}return n;}
int main(int argc,char **argv){try{
 check(argc==3||argc==4,"Original attribute and lexical fixtures required");SearchHtmlExport held;
 {
  Reader reader(argv[1]);StudySession session(reader);SearchBatchLimits limits;limits.total.max_results=1;
  auto search=session.search_batch({{"Text","Original"},{"Commentary","Original"},{"Text","Original"}},limits);
  SearchHtmlExportRequest request;request.generation=search.generation.id;request.selection=SearchExportSelection::all_retained;
  request.include_scripture=true;request.title="Original <query> & café </title><script>not-code</script>";
  auto invalid_request=request;invalid_request.selection=SearchExportSelection::visible;invalid_request.rows.clear();
  bool rejected=false;try{session.export_search_html(invalid_request);}catch(const std::invalid_argument &){rejected=true;}check(rejected,"Empty visible HTML selection is not an all-results alias");
  invalid_request=request;invalid_request.max_rows=0;rejected=false;try{session.export_search_html(invalid_request);}catch(const std::invalid_argument &){rejected=true;}check(rejected,"Invalid HTML aggregate row limit");
  const auto before=session.module_presentation_options("Text");
  held=session.export_search_html(request);
  check(held.status==SearchExportStatus::ok&&held.row_count==12&&held.selection==SearchExportSelection::all_retained&&held.html.find("&lt;script&gt;not-code&lt;/script&gt;")!=std::string::npos&&held.html.find("<script>")==std::string::npos,"Complete HTML includes all retained groups with escaped title and first/duplicate rows");
  check(count(held.html,"John 1:1 Text")==2&&held.html.find("Original")!=std::string::npos&&held.html.find("default-src 'none'")!=std::string::npos,"Duplicate identity and passive document policy are preserved");
  std::unique_ptr<xmlDoc,decltype(&xmlFreeDoc)> document(htmlReadMemory(held.html.data(),static_cast<int>(held.html.size()),nullptr,"UTF-8",HTML_PARSE_NONET|HTML_PARSE_NODEFDTD),xmlFreeDoc);
  check(document!=nullptr,"Owned export is a parseable complete HTML document");
  check(!held.plain_text,"Existing HTML-only calls do not silently acquire another representation");
  request.plain_text_max_bytes=8192;
  auto rich=session.export_search_html(request);
  SearchAllTextExportRequest plain;plain.generation=search.generation.id;plain.title=request.title;plain.include_scripture=true;plain.max_bytes=8192;
  const auto expected=session.export_search_all_text(plain);
  check(rich.status==SearchExportStatus::ok&&rich.html==held.html&&rich.plain_text==expected.text&&rich.row_count==expected.row_count,
    "Rich HTML reuses exact all-retained text semantics on the same resolved selection");
  request.plain_text_max_bytes=expected.text.size();
  check(session.export_search_html(request).plain_text==expected.text,"Exact companion byte limit succeeds");
  request.plain_text_max_bytes=expected.text.size()-1;
  auto short_text=session.export_search_html(request);
  check(short_text.status==SearchExportStatus::output_limit&&short_text.html.empty()&&!short_text.plain_text&&!short_text.row_count&&!short_text.images.count,
    "One-byte-short companion discards HTML, text, counters and rows together");
  request.plain_text_max_bytes=0;rejected=false;
  try{session.export_search_html(request);}catch(const std::invalid_argument &){rejected=true;}
  check(rejected,"An enabled C++ companion cannot have zero budget");
  request.plain_text_max_bytes.reset();
  request.include_search_context=true;request.plain_text_max_bytes=16384;request.max_bytes=16384;
  auto contextual=session.export_search_html(request);
  check(contextual.status==SearchExportStatus::ok&&contextual.row_count==12&&
    count(contextual.html,"class=\"xiphos-search-context\"")==3&&count(contextual.html,"John 1:1 Text")==2&&
    contextual.html.find("Query: Original")!=std::string::npos&&contextual.html.find("Mode: Exact phrase")!=std::string::npos&&
    contextual.plain_text->find("Search group 3\nModule: Text")!=std::string::npos,
    "Opted-in headers use committed group metadata, keep every first row and distinguish repeated modules");
  check(count(contextual.html,"Source SDK result elements: 4")==3&&count(*contextual.plain_text,"Source SDK result elements: 4")==3&&
    contextual.html.find("Source visible references: 0")!=std::string::npos&&contextual.plain_text->find("Source visible references: 1")!=std::string::npos,
    "HTML and text companion disclose the same source result and preview counts for every retained group");
  request.max_bytes=contextual.html.size();request.plain_text_max_bytes=contextual.plain_text->size();
  check(session.export_search_html(request).html==contextual.html,"Exact context budgets include both complete representations");
  --request.max_bytes;auto short_context=session.export_search_html(request);
  check(short_context.status==SearchExportStatus::output_limit&&short_context.html.empty()&&!short_context.plain_text,
    "HTML header bytes count against the same complete-document budget");
  request.include_search_context=false;request.plain_text_max_bytes.reset();
  request.max_bytes=held.html.size();check(session.export_search_html(request).html==held.html,"Exact complete HTML byte cap accepted");
  --request.max_bytes;auto clipped=session.export_search_html(request);check(clipped.status==SearchExportStatus::output_limit&&clipped.html.empty()&&!clipped.row_count,"HTML output cap never returns an unlabeled partial page");
  request.max_bytes=8192;request.max_rows=11;auto row_limited=session.export_search_html(request);
  check(row_limited.status==SearchExportStatus::output_limit&&row_limited.html.empty(),"Complete HTML never treats a clipped row prefix as all retained output");request.max_rows=maximum_search_export_rows;
  request.selection=SearchExportSelection::visible;request.rows={{0,0},{0,0}};auto selected=session.export_search_html(request);
  check(selected.status==SearchExportStatus::ok&&selected.row_count==2&&count(selected.html,"John 1:1 Text")==2&&selected.html.find("Selected search passages.")!=std::string::npos,"Explicit visible duplicates preserve request order/identity");
  request.plain_text_max_bytes=8192;
  SearchTextExportRequest visible;visible.generation=request.generation;visible.rows=request.rows;visible.title=request.title;visible.include_scripture=true;visible.max_bytes=8192;
  check(session.export_search_html(request).plain_text==session.export_search_visible_text(visible).text,
    "Visible rich text preserves duplicate order and the existing text formatter");
  request.rows={{1,0}};check(session.export_search_html(request).status==SearchExportStatus::invalid_selection,"Omitted group cannot be synthesized from complete keys in visible mode");
  request.rows={{0,0}};request.generation++;check(session.export_search_html(request).status==SearchExportStatus::stale,"Stale HTML request cannot substitute latest generation");request.generation--;
  auto stopped=std::make_shared<SearchControl>();stopped->request_cancel();auto stop=session.export_search_html(request,stopped);
  check(stop.status==SearchExportStatus::cancelled&&stop.html.empty()&&!stop.plain_text&&stopped->phase()==SearchPhase::cancelled,"Pre-cancel HTML has no payload");
  check(session.history().entries().empty()&&session.search_generation().id==search.generation.id&&session.module_presentation_options("Text").image_content==before.image_content,"HTML reads preserve logical profile/history/search state");
  auto incomplete=session.search_batch({{"Text","Original"},{"Missing","Original"}},limits);request={};request.generation=incomplete.generation.id;request.selection=SearchExportSelection::all_retained;
  check(session.export_search_html(request).status==SearchExportStatus::source_incomplete,"HTML shares incomplete-source default refusal");request.allow_partial_source=true;request.plain_text_max_bytes=8192;
  auto partial=session.export_search_html(request);check(partial.status==SearchExportStatus::ok&&partial.generation.failed_groups==1&&partial.plain_text.has_value()&&partial.html.find("only returned matches are included")!=std::string::npos,"Explicit partial HTML labels its source disposition in the document as well as metadata");
 }
 {
  Reader reader(argv[2]);StudySession session(reader);session.enable_module_preferences();session.set_lexical_layout_enabled(true);
  EngineOptions on;on.strongs=true;session.set_module_engine_options("Text",on);session.set_module_engine_options("ThMLNotes",EngineOptions{});
  auto search=session.search_batch({{"Text","Original"},{"ThMLNotes","Original"}});
  SearchHtmlExportRequest request;request.generation=search.generation.id;request.rows={{0,0}};request.include_scripture=true;
  // Search result ordering begins at earlier plain verses; explicitly locate a
  // rich source row from copied references rather than assuming a fixed ordinal.
  const auto &refs=search.groups[0].result->references;auto rich=std::find(refs.begin(),refs.end(),"John 4:1");check(rich!=refs.end(),"Rich original source must be in search results");request.rows={{0,static_cast<std::size_t>(rich-refs.begin())}};
  const auto before=session.module_presentation_options("Text");auto unsupported=session.export_search_html(request);
  check(unsupported.status==SearchExportStatus::unsupported_html&&unsupported.issue==HtmlExportIssue::resources&&unsupported.html.empty()&&!unsupported.row_count,"Missing/resource-dependent source rejects HTML instead of dropping the image");
  session.set_image_content("Text",false);auto supported=session.export_search_html(request);
  check(supported.status==SearchExportStatus::ok&&supported.html.find(".xiphos-export-style-2 .word")!=std::string::npos&&supported.html.find("class=\"xiphos-export-style-2\"")!=std::string::npos&&supported.html.find("<img")==std::string::npos,"Core HTML/CSS stays matched and isolated after explicit existing image preference");
  const auto &comment_refs=search.groups[1].result->references;const auto plain_row=std::find(comment_refs.begin(),comment_refs.end(),"John 4:2");
  check(plain_row!=comment_refs.end(),"Original plain commentary row required for mixed CSS document");
  request.rows.push_back({1,static_cast<std::size_t>(plain_row-comment_refs.begin())});auto mixed=session.export_search_html(request);
  check(mixed.status==SearchExportStatus::ok&&mixed.row_count==2&&mixed.html.find("class=\"xiphos-export-style-0\"")!=std::string::npos&&
   count(mixed.html,"<style>")==1,"Mixed configured modules share one scoped stylesheet without applying annotation rules to the plain row");
  if(argc==4){std::ofstream output(argv[3],std::ios::binary);output<<mixed.html;check(static_cast<bool>(output),"Write original-prose local visual fixture");}
  check(before.image_content==ContentPreference::unknown&&session.module_presentation_options("Text").respect_font_faces==ContentPreference::unknown,"HTML attempts do not learn/publish font choices");
 }
 {
  Reader reader(argv[1]);StudySession session(reader);
  auto ordered=session.search_batch({{"Text","Original"},{"Commentary","Original"}});
  SearchHtmlExportRequest mixed; mixed.generation=ordered.generation.id; mixed.rows={{0,1},{1,0},{0,0}};
  mixed.include_search_context=true;mixed.plain_text_max_bytes=8192;
  const auto reordered=session.export_search_html(mixed);
  check(reordered.status==SearchExportStatus::ok&&reordered.row_count==3&&count(reordered.html,"class=\"xiphos-search-context\"")==3&&
    reordered.html.find("John 1:2 Text")<reordered.html.find("John 1:1 Commentary")&&
    reordered.html.find("John 1:1 Commentary")<reordered.html.find("John 1:1 Text"),
    "Visible group reentry repeats its committed header without reordering selected rows");
  SearchQuery query{"Text","Original|<script>not-code</script>"};query.mode=SearchMode::regular_expression;
  query.case_sensitive=true;query.range=SearchRange{"Text","John1:1-2"};query.text_options=SearchTextOptions{true,false,true};
  auto result=session.search_module(query);check(!result.references.empty(),"Context query has original matches");
  session.clear_search_text_options();
  SearchHtmlExportRequest request;request.generation=result.generation.id;request.rows={{0,0},{0,0}};
  request.include_search_context=true;request.plain_text_max_bytes=8192;
  const auto context=session.export_search_html(request);
  check(context.status==SearchExportStatus::ok&&context.row_count==2&&count(context.html,"class=\"xiphos-search-context\"")==1&&
    context.html.find("Query: Original|&lt;script&gt;not-code&lt;/script&gt;")!=std::string::npos&&context.html.find("<script>")==std::string::npos&&
    context.plain_text->find("Query: "+query.expression)!=std::string::npos&&context.plain_text->find("Case sensitive: on\nScope: range (Text): John1:1-2")!=std::string::npos&&
    context.plain_text->find("Search annotation policy: Strong's on, morphology off, footnotes on")!=std::string::npos,
    "Headers escape committed query metadata and retain search-time annotations after settings change");
  check(!session.search_text_options().options&&session.history().entries().empty()&&session.search_generation().id==result.generation.id,
    "Header preparation does not restore old annotations or change history/generation");
  query={"Text","Original"};query.mode=SearchMode::multiword;query.within_last_results=true;
  result=session.search_module(query);request.generation=result.generation.id;
  auto last=session.export_search_html(request);
  check(last.status==SearchExportStatus::ok&&last.plain_text->find("Mode: Multiword")!=std::string::npos&&
    last.plain_text->find("Scope: previous results")!=std::string::npos,"Last-scope provenance belongs to committed request, not latest UI scope");
  query={"Text","Original"};query.mode=SearchMode::indexed;
  result=session.search_module(query);request.generation=result.generation.id;
  auto indexed=session.export_search_html(request);
  check(indexed.status==SearchExportStatus::ok&&indexed.plain_text->find("Mode: Raw indexed query")!=std::string::npos&&
    (indexed.plain_text->find("used multiword fallback")!=std::string::npos)==result.index_fallback,
    "Explicit indexed provenance reports actual captured fallback without claiming usable indexing");
 }
 std::future<SearchHtmlExport> drained;
 {StudyWorker worker(argv[1]);const auto tab=worker.create_tab().get();auto search=worker.search_module(tab,{"Text","Original"}).get();
  SearchHtmlExportRequest request;request.generation=search.generation.id;request.rows={{0,0}};request.include_scripture=true;
  drained=worker.export_search_html(tab,request);request.rows.clear();worker.close_tab(tab).get();}
 check(drained.get().status==SearchExportStatus::ok&&held.html.find("Original")!=std::string::npos,"Accepted copied HTML request drains; complete owned payload outlives worker");
 std::cout<<"Selected/all HTML, scoped CSS, unsupported assets, escaping, strict budgets and owner lifetime passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
