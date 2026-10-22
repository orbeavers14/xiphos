// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
xc_status wait(xc_request *r){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);xc_status status;
 while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"C HTML deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==3,"Original attribute/lexical fixtures required");xc_worker *worker=nullptr;xc_request *request=nullptr;
 auto open=[&](const char *root){check(xc_worker_open(root,&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Open HTML worker/tab");const auto id=xc_request_tab(request);xc_request_release(request);return id;};
 auto tab=open(argv[1]);xc_search_query query;xc_search_query_default(&query);query.expression="Original";
 const char *modules[]={"Text","Commentary"};xc_search_batch_limits budget;xc_search_batch_limits_default(&budget);budget.max_results=1;
 check(xc_worker_search_modules(worker,tab,modules,2,&query,&budget,&request)==XC_OK&&wait(request)==XC_OK,"Seed retained HTML groups");auto *batch=xc_request_take_search_batch(request);xc_request_release(request);
 xc_search_generation_info generation{};check(xc_search_batch_get_generation(batch,&generation)==XC_OK&&generation.visible_results==1,"Source prefix deliberately clipped");xc_search_batch_release(batch);
 xc_search_html_export_request input;xc_search_html_export_request_default(&input);
 check(input.selection==0&&!input.rows&&!input.row_count&&input.max_bytes==1024*1024&&input.max_rows==10000&&!input.allow_partial_source,"HTML defaults explicit and bounded");
 input.generation=generation.id;input.selection=1;input.include_scripture=1;input.max_rows=8;std::string title="Original <query> café";input.title=title.c_str();
 check(xc_worker_export_search_html(worker,tab,&input,&request)==XC_OK&&!xc_request_take_search_html_export(request)&&!xc_request_take_search_text_export(request),"Accepted HTML observer is typed and requires poll");
 input.max_rows=1;title="Changed";
 check(wait(request)==XC_OK,"Complete copied HTML request");auto *owned=xc_request_take_search_html_export(request);
 check(owned&&!xc_request_take_search_html_export(request)&&!xc_request_take_search_text_export(request),"HTML transfer is take-once and distinct from text");xc_request_release(request);
 xc_search_html_export_info info{};check(xc_search_html_export_get_info(owned,&info)==XC_OK&&info.disposition==XC_SEARCH_EXPORT_OK&&info.row_count==8&&info.selection==1&&info.includes_scripture&&info.issue==XC_HTML_EXPORT_ISSUE_NONE,"Copied all-result options recover every retained row");
 const std::string html=xc_search_html_export_html(owned);check(html.find("Original &lt;query&gt; café")!=std::string::npos&&xc_search_html_export_size(owned)==html.size(),"Owned UTF-8 HTML title and exact byte size");
 xc_search_generation_info copied{};check(xc_search_html_export_get_generation(owned,&copied)==XC_OK&&copied.id==generation.id,"Source generation metadata copied");
 check(!xc_search_html_export_plain_text(owned)&&!xc_search_html_export_plain_text_size(owned)&&
  !xc_search_html_export_plain_text(nullptr)&&!xc_search_html_export_plain_text_size(nullptr),"Absent/null C companion is distinct from an empty string");
 xc_search_html_image_options images;xc_search_html_image_options_default(&images);
 input.title="Original <query> café";input.max_rows=8;
 auto run_rich=[&](size_t cap){check(xc_worker_export_search_rich_html(worker,tab,&input,&images,cap,&request)==XC_OK&&wait(request)==XC_OK,"Rich C job completes");
  auto *r=xc_request_take_search_html_export(request);check(r&&!xc_request_take_search_html_export(request),"Rich C ownership transfers once");xc_request_release(request);return r;};
 auto *rich_payload=run_rich(8192);const std::string text=xc_search_html_export_plain_text(rich_payload);
 check(std::string(xc_search_html_export_html(rich_payload))==html&&text.find("Original <query> café")!=std::string::npos&&
  text.size()==xc_search_html_export_plain_text_size(rich_payload),"C rich representation owns complete UTF-8 text beside unchanged HTML");
 auto *rich_exact=run_rich(text.size());check(std::string(xc_search_html_export_plain_text(rich_exact))==text,"C exact companion cap");xc_search_html_export_release(rich_exact);
 auto *rich_short=run_rich(text.size()-1);xc_search_html_export_info rich_info{};
 check(xc_search_html_export_get_info(rich_short,&rich_info)==XC_OK&&rich_info.disposition==XC_SEARCH_EXPORT_OUTPUT_LIMIT&&
  !xc_search_html_export_size(rich_short)&&!xc_search_html_export_plain_text(rich_short)&&!rich_info.row_count,"C companion exhaustion cannot leak HTML-only success");xc_search_html_export_release(rich_short);
 request=reinterpret_cast<xc_request *>(1);
 check(xc_worker_export_search_rich_html(worker,tab,&input,&images,8*1024*1024+1,&request)==XC_INVALID_ARGUMENT&&!request,"C rejects oversized companion cap before enqueue");
 check(xc_worker_export_search_rich_html_with_context(worker,tab,&input,&images,8192,1,&request)==XC_OK&&wait(request)==XC_OK,"C context HTML queues on existing owner");
 auto *context=xc_request_take_search_html_export(request);xc_request_release(request);
 check(context&&std::string(xc_search_html_export_html(context)).find("class=\"xiphos-search-context\"")!=std::string::npos&&
  std::string(xc_search_html_export_plain_text(context)).find("Query: Original\nMode: Exact phrase")!=std::string::npos,
  "C carries committed shared headers in both representations without changing result layouts");
 xc_search_html_export_release(context);
 xc_search_visible_row context_row{0,0};xc_search_text_export_request text_input;xc_search_text_export_request_default(&text_input);
 text_input.generation=generation.id;text_input.rows=&context_row;text_input.row_count=1;
 check(xc_worker_export_search_visible_text_with_context(worker,tab,&text_input,1,&request)==XC_OK&&wait(request)==XC_OK,"C selected context accepted");
 auto *text_context=xc_request_take_search_text_export(request);xc_request_release(request);
 check(text_context&&std::string(xc_search_text_export_text(text_context)).find("Search group 1\nModule: Text")!=std::string::npos,"C selected context is core text");xc_search_text_export_release(text_context);
 xc_search_all_text_export_request all_input;xc_search_all_text_export_request_default(&all_input);all_input.generation=generation.id;
 check(xc_worker_export_search_all_text_with_context(worker,tab,&all_input,1,&request)==XC_OK&&wait(request)==XC_OK,"C complete context accepted");
 text_context=xc_request_take_search_text_export(request);xc_request_release(request);
 check(text_context&&std::string(xc_search_text_export_text(text_context)).find("Search group 2\nModule: Commentary")!=std::string::npos,"C complete context includes groups beyond visible prefix");xc_search_text_export_release(text_context);
 for(int flag:{-1,2}){
  request=reinterpret_cast<xc_request *>(1);check(xc_worker_export_search_rich_html_with_context(worker,tab,&input,&images,8192,flag,&request)==XC_INVALID_ARGUMENT&&!request,"C rejects invalid HTML context flags and clears observer");
  request=reinterpret_cast<xc_request *>(1);check(xc_worker_export_search_visible_text_with_context(worker,tab,&text_input,flag,&request)==XC_INVALID_ARGUMENT&&!request,"C rejects invalid selected context flags");
  request=reinterpret_cast<xc_request *>(1);check(xc_worker_export_search_all_text_with_context(worker,tab,&all_input,flag,&request)==XC_INVALID_ARGUMENT&&!request,"C rejects invalid complete context flags");
 }
 input.title="Original <query> café";input.max_rows=8;input.max_bytes=html.size();
 auto run=[&]{check(xc_worker_export_search_html(worker,tab,&input,&request)==XC_OK&&wait(request)==XC_OK,"HTML outcome request");auto *r=xc_request_take_search_html_export(request);xc_request_release(request);check(r&&xc_search_html_export_get_info(r,&info)==XC_OK,"Owned HTML outcome");return r;};
 auto *exact=run();check(std::string(xc_search_html_export_html(exact))==html,"Exact C HTML budget accepted");xc_search_html_export_release(exact);
 --input.max_bytes;auto *limited=run();check(info.disposition==XC_SEARCH_EXPORT_OUTPUT_LIMIT&&!info.row_count&&!xc_search_html_export_size(limited),"One-byte-short C HTML never exposes partial page");xc_search_html_export_release(limited);
 input.max_bytes=8192;input.generation++;auto *stale=run();check(info.disposition==XC_SEARCH_EXPORT_STALE&&xc_search_html_export_get_generation(stale,&copied)==XC_OK&&copied.id==generation.id&&!xc_search_html_export_size(stale),"Stale outcome is not a newer export");xc_search_html_export_release(stale);input.generation--;
 xc_search_visible_row rows[]={{0,0},{0,0}};input.selection=0;input.rows=rows;input.row_count=2;
 check(xc_worker_export_search_html(worker,tab,&input,&request)==XC_OK,"Accept copied visible rows");rows[0]={99,99};
 check(wait(request)==XC_OK,"Copied ordinals complete");auto *selected=xc_request_take_search_html_export(request);xc_request_release(request);
 check(xc_search_html_export_get_info(selected,&info)==XC_OK&&info.disposition==XC_SEARCH_EXPORT_OK&&info.row_count==2&&info.selection==0,"Copied duplicate visible rows preserve identity");xc_search_html_export_release(selected);rows[0]={0,0};
 auto rejects=[&]{request=reinterpret_cast<xc_request *>(1);check(xc_worker_export_search_html(worker,tab,&input,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid HTML input clears observer");};
 input.selection=2;rejects();input.selection=0;input.row_count=0;rejects();input.row_count=10001;rejects();input.row_count=2;input.rows=nullptr;rejects();input.rows=rows;input.selection=1;rejects();input.rows=nullptr;input.row_count=0;
 input.include_scripture=2;rejects();input.include_scripture=1;input.allow_partial_source=2;rejects();input.allow_partial_source=0;input.max_rows=0;rejects();input.max_rows=10001;rejects();input.max_rows=8;input.max_bytes=0;rejects();input.max_bytes=8*1024*1024+1;rejects();input.max_bytes=8192;
 check(xc_worker_export_search_html(nullptr,tab,&input,&request)==XC_INVALID_ARGUMENT&&!request&&xc_worker_export_search_html(worker,tab,nullptr,&request)==XC_INVALID_ARGUMENT&&!request&&xc_worker_export_search_html(worker,tab,&input,nullptr)==XC_INVALID_ARGUMENT,"NULL HTML request shapes");
 info.row_count=99;copied.id=99;
 check(xc_search_html_export_get_info(nullptr,&info)==XC_INVALID_ARGUMENT&&!info.row_count&&xc_search_html_export_get_generation(nullptr,&copied)==XC_INVALID_ARGUMENT&&!copied.id&&
  xc_search_html_export_get_info(owned,nullptr)==XC_INVALID_ARGUMENT&&xc_search_html_export_get_generation(owned,nullptr)==XC_INVALID_ARGUMENT&&!xc_search_html_export_html(nullptr)&&!xc_search_html_export_size(nullptr)&&!xc_request_take_search_html_export(nullptr),"NULL HTML getters clear stale data");
 check(xc_worker_export_search_html(worker,999,&input,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_search_html_export(request),"Unknown tab fails request");xc_request_release(request);
 check(xc_worker_export_search_html(worker,tab,&input,&request)==XC_OK&&xc_request_cancel_search(request)==XC_OK&&wait(request)==XC_OK,"HTML cooperative cancellation accepted");auto *race=xc_request_take_search_html_export(request);xc_search_progress progress{};
 check(xc_search_html_export_get_info(race,&info)==XC_OK&&xc_request_search_progress(request,&progress)==XC_OK&&
  ((info.disposition==XC_SEARCH_EXPORT_OK&&progress.phase==XC_SEARCH_COMPLETE)||(info.disposition==XC_SEARCH_EXPORT_CANCELLED&&progress.phase==XC_SEARCH_CANCELLED&&!xc_search_html_export_size(race))),"HTML cancellation/completion outcome agrees with job control");xc_request_release(request);xc_search_html_export_release(race);
 check(xc_worker_export_search_html(worker,tab,&input,&request)==XC_OK,"Detached HTML observer accepted");xc_request_release(request);
 check(xc_worker_export_search_html(worker,tab,&input,&request)==XC_OK,"HTML accepted before shutdown");xc_worker_close(worker);worker=nullptr;
 check(wait(request)==XC_OK,"Shutdown drains HTML");auto *drained=xc_request_take_search_html_export(request);xc_request_release(request);check(xc_search_html_export_get_info(drained,&info)==XC_OK&&info.disposition==XC_SEARCH_EXPORT_OK,"Drained HTML stays owned");xc_search_html_export_release(drained);
 tab=open(argv[2]);query.module="Text";
 check(xc_worker_search(worker,tab,&query,&request)==XC_OK&&wait(request)==XC_OK,"Seed real resource-dependent HTML source");auto *search=xc_request_take_search(request);xc_request_release(request);xc_search_info source{};
 check(xc_search_get_info(search,&source)==XC_OK&&xc_search_get_generation(search,&generation)==XC_OK,"Rich source metadata");size_t rich=source.result_count;
 for(size_t i=0;i<source.result_count;++i)if(std::string(xc_search_reference(search,i))=="John 4:1")rich=i;
 check(rich<source.result_count,"Rich original row exists");xc_search_release(search);
 rows[0]={0,rich};input={generation.id,0,rows,1,nullptr,1,8192,10000,0};auto *unsupported=run();
 check(info.disposition==XC_SEARCH_EXPORT_UNSUPPORTED_HTML&&info.issue==XC_HTML_EXPORT_ISSUE_RESOURCES&&info.failed_row_present&&!info.failed_row&&!xc_search_html_export_size(unsupported),"Unsupported asset outcome is typed, empty and identifies the selected row");xc_search_html_export_release(unsupported);xc_worker_close(worker);
 check(std::string(xc_search_html_export_html(owned))==html,"HTML copied value survives unrelated later worker lifetime");check(std::string(xc_search_html_export_plain_text(rich_payload))==text,"Owned rich companion survives worker shutdown and later engine use");
 xc_search_html_export_release(rich_payload);xc_search_html_export_release(owned);xc_search_html_export_release(nullptr);
 std::cout<<"C copied selected/all HTML, exact limits, typed unsupported outcomes, cancellation and lifetime passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
