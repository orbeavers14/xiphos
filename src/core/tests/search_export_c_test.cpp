// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
xc_status wait(xc_request *request){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);xc_status status;
 while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"C export deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Original attribute fixture required");xc_worker *worker=nullptr;xc_request *request=nullptr;
 check(xc_worker_open(argv[1],&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Open worker/tab");
 const auto tab=xc_request_tab(request);xc_request_release(request);
 xc_search_query query;xc_search_query_default(&query);query.module="Text";query.expression="Original";
 check(xc_worker_search(worker,tab,&query,&request)==XC_OK&&wait(request)==XC_OK,"Seed generation");auto *search=xc_request_take_search(request);xc_request_release(request);
 xc_search_generation_info generation{};check(xc_search_get_generation(search,&generation)==XC_OK,"Seed metadata");xc_search_release(search);
 xc_search_text_export_request input;xc_search_text_export_request_default(&input);
 check(input.max_bytes==1024*1024&&!input.include_scripture&&!input.rows&&!input.title,"C bounded defaults");
 xc_search_visible_row rows[]={{0,2},{0,0},{0,2}};std::string title="Original selected text";
 input.generation=generation.id;input.rows=rows;input.row_count=3;input.title=title.c_str();input.include_scripture=1;
 check(xc_worker_export_search_visible_text(worker,tab,&input,&request)==XC_OK,"Accept copied export");
 title="Changed";rows[0]={99,99};input.max_bytes=1;input.include_scripture=0;
 check(!xc_request_take_search_text_export(request)&&!xc_request_take_search(request),"Pending/wrong-kind take rejects");
 check(wait(request)==XC_OK,"Complete export");xc_search_progress progress{};
 check(xc_request_search_progress(request,&progress)==XC_OK&&progress.phase==XC_SEARCH_COMPLETE&&progress.percent==100,"Export uses job progress without pretending to search");
 auto *owned=xc_request_take_search_text_export(request);
 check(owned&&!xc_request_take_search_text_export(request)&&!xc_request_take_search(request),"Export result take-once");xc_request_release(request);
 xc_search_text_export_info info{};
 check(xc_search_text_export_get_info(owned,&info)==XC_OK&&info.disposition==XC_SEARCH_EXPORT_OK&&info.row_count==3&&info.includes_scripture&&!info.failed_row_present,"Copied selection/options produce complete document");
 const std::string text=xc_search_text_export_text(owned);
 check(text.find("Original selected text\n\n")==0&&text.find("John 1:3 Text")<text.find("John 1:1 Text")&&
  text.rfind("John 1:3 Text")>text.find("John 1:1 Text")&&text.find("Original")!=std::string::npos&&xc_search_text_export_size(owned)==text.size(),"Owned payload preserves ordered duplicates and UTF-8 byte size");
 xc_search_generation_info copied{};check(xc_search_text_export_get_generation(owned,&copied)==XC_OK&&copied.id==generation.id,"Export origin metadata");
 rows[0]={0,0};input.row_count=1;input.title=nullptr;input.max_bytes=1;
 auto run=[&]{check(xc_worker_export_search_visible_text(worker,tab,&input,&request)==XC_OK&&wait(request)==XC_OK,"Export disposition request");
  auto *result=xc_request_take_search_text_export(request);xc_request_release(request);check(result&&xc_search_text_export_get_info(result,&info)==XC_OK,"Owned disposition");return result;};
 auto *limited=run();check(info.disposition==XC_SEARCH_EXPORT_OUTPUT_LIMIT&&!info.row_count&&xc_search_text_export_size(limited)==0&&std::string(xc_search_text_export_text(limited)).empty(),"Output failure never publishes partial text");xc_search_text_export_release(limited);
 input.max_bytes=1024;rows[0]={0,999};auto *invalid=run();check(info.disposition==XC_SEARCH_EXPORT_INVALID_SELECTION&&info.failed_row_present&&!info.failed_row,"Failed ordinal copied");xc_search_text_export_release(invalid);
 rows[0]={0,0};input.generation=generation.id+1;auto *stale=run();check(info.disposition==XC_SEARCH_EXPORT_STALE&&xc_search_text_export_get_generation(stale,&copied)==XC_OK&&copied.id==generation.id,"Stale request exposes current metadata without exporting it");xc_search_text_export_release(stale);input.generation=generation.id;
 auto rejects=[&]{request=reinterpret_cast<xc_request *>(1);check(xc_worker_export_search_visible_text(worker,tab,&input,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid export clears observer");};
 input.row_count=0;rejects();input.row_count=10001;rejects();input.row_count=1;input.rows=nullptr;rejects();input.rows=rows;input.include_scripture=2;rejects();input.include_scripture=0;input.max_bytes=0;rejects();input.max_bytes=8*1024*1024+1;rejects();input.max_bytes=1024;
 std::string large(32769,'x');input.title=large.c_str();rejects();input.title=nullptr;
 check(xc_worker_export_search_visible_text(nullptr,tab,&input,&request)==XC_INVALID_ARGUMENT&&!request&&
  xc_worker_export_search_visible_text(worker,tab,nullptr,&request)==XC_INVALID_ARGUMENT&&!request&&
  xc_worker_export_search_visible_text(worker,tab,&input,nullptr)==XC_INVALID_ARGUMENT,"NULL request inputs");
 info.row_count=99;copied.id=99;
 check(xc_search_text_export_get_info(nullptr,&info)==XC_INVALID_ARGUMENT&&!info.row_count&&
  xc_search_text_export_get_generation(nullptr,&copied)==XC_INVALID_ARGUMENT&&!copied.id&&
  xc_search_text_export_get_info(owned,nullptr)==XC_INVALID_ARGUMENT&&xc_search_text_export_get_generation(owned,nullptr)==XC_INVALID_ARGUMENT&&
  !xc_search_text_export_text(nullptr)&&!xc_search_text_export_size(nullptr)&&!xc_request_take_search_text_export(nullptr),"NULL getters clear stale values");
 check(xc_worker_export_search_visible_text(worker,999,&input,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_search_text_export(request),"Unknown tab fails at poll");xc_request_release(request);
 std::string bad(1,static_cast<char>(0xff));input.title=bad.c_str();check(xc_worker_export_search_visible_text(worker,tab,&input,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_search_text_export(request),"Queued invalid UTF-8 has no payload");xc_request_release(request);input.title=nullptr;
 check(xc_worker_export_search_visible_text(worker,tab,&input,&request)==XC_OK&&xc_request_cancel_search(request)==XC_OK&&wait(request)==XC_OK,"Cooperative cancel accepted");
 auto *race=xc_request_take_search_text_export(request);check(xc_search_text_export_get_info(race,&info)==XC_OK&&xc_request_search_progress(request,&progress)==XC_OK&&
  ((info.disposition==XC_SEARCH_EXPORT_OK&&progress.phase==XC_SEARCH_COMPLETE)||(info.disposition==XC_SEARCH_EXPORT_CANCELLED&&progress.phase==XC_SEARCH_CANCELLED&&!xc_search_text_export_size(race))),"Cancel/completion disposition agrees with control");xc_request_release(request);xc_search_text_export_release(race);
 check(xc_worker_export_search_visible_text(worker,tab,&input,&request)==XC_OK,"Detached observer export");xc_request_release(request);
 // All-retained transport recovers keys omitted from a one-row display prefix.
 xc_search_options prefix;xc_search_options_default(&prefix);prefix.max_results=1;query.options=&prefix;
 check(xc_worker_search(worker,tab,&query,&request)==XC_OK&&wait(request)==XC_OK,"Seed clipped all-export source");
 search=xc_request_take_search(request);xc_request_release(request);
 check(xc_search_get_generation(search,&generation)==XC_OK,"Clipped generation metadata");xc_search_release(search);
 xc_search_all_text_export_request complete;xc_search_all_text_export_request_default(&complete);
 check(complete.max_rows==10000&&complete.max_bytes==1024*1024&&!complete.allow_partial_source,"Complete defaults bounded and reject partial sources");
 complete.generation=generation.id;complete.include_scripture=1;std::string label="All original";complete.title=label.c_str();
 check(xc_worker_export_search_all_text(worker,tab,&complete,&request)==XC_OK,"Accept copied complete request");
 complete.max_rows=1;label="Changed";
 check(wait(request)==XC_OK,"Complete copied request");auto *all=xc_request_take_search_text_export(request);xc_request_release(request);
 check(xc_search_text_export_get_info(all,&info)==XC_OK&&info.disposition==XC_SEARCH_EXPORT_OK&&info.row_count==4&&
  xc_search_text_export_selection(all)==1&&xc_search_text_export_selection(owned)==0&&xc_search_text_export_selection(nullptr)==-1&&
  std::string(xc_search_text_export_text(all)).find("All original\n\n")==0,"Complete selection kind, copied title/cap and all retained rows");
 complete.title=nullptr;complete.max_rows=3;
 auto runAll=[&]{check(xc_worker_export_search_all_text(worker,tab,&complete,&request)==XC_OK&&wait(request)==XC_OK,"Complete disposition request");
  auto *result=xc_request_take_search_text_export(request);xc_request_release(request);check(result&&xc_search_text_export_get_info(result,&info)==XC_OK,"Complete owned outcome");return result;};
 auto *clipped=runAll();check(info.disposition==XC_SEARCH_EXPORT_OUTPUT_LIMIT&&!xc_search_text_export_size(clipped),"Complete clipped output is never success");xc_search_text_export_release(clipped);
 complete.max_rows=4;query.module=nullptr;const char *pair[]={"Text","Missing"};
 check(xc_worker_search_modules(worker,tab,pair,2,&query,nullptr,&request)==XC_OK&&wait(request)==XC_OK,"Seed incomplete source");
 auto *batch=xc_request_take_search_batch(request);xc_request_release(request);check(xc_search_batch_get_generation(batch,&generation)==XC_OK,"Incomplete generation");xc_search_batch_release(batch);complete.generation=generation.id;
 auto *incomplete=runAll();check(info.disposition==XC_SEARCH_EXPORT_SOURCE_INCOMPLETE&&!xc_search_text_export_size(incomplete),"Default complete export rejects failed module source");xc_search_text_export_release(incomplete);
 complete.allow_partial_source=1;auto *partial=runAll();
 check(info.disposition==XC_SEARCH_EXPORT_OK&&info.row_count==4&&xc_search_text_export_get_generation(partial,&copied)==XC_OK&&copied.failed_groups==1,"Explicit partial source retains failure counts");xc_search_text_export_release(partial);
 auto rejectAll=[&]{request=reinterpret_cast<xc_request *>(1);check(xc_worker_export_search_all_text(worker,tab,&complete,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid complete request clears observer");};
 complete.allow_partial_source=2;rejectAll();complete.allow_partial_source=0;complete.include_scripture=2;rejectAll();complete.include_scripture=0;complete.max_rows=0;rejectAll();complete.max_rows=10001;rejectAll();complete.max_rows=4;complete.max_bytes=0;rejectAll();complete.max_bytes=8*1024*1024+1;rejectAll();complete.max_bytes=1024;
 check(xc_worker_export_search_all_text(worker,tab,nullptr,&request)==XC_INVALID_ARGUMENT&&!request&&
  xc_worker_export_search_all_text(nullptr,tab,&complete,&request)==XC_INVALID_ARGUMENT&&!request&&
  xc_worker_export_search_all_text(worker,tab,&complete,nullptr)==XC_INVALID_ARGUMENT,"NULL complete inputs");
 complete.allow_partial_source=1;
 check(xc_worker_export_search_all_text(worker,tab,&complete,&request)==XC_OK,"Complete shutdown drain");
 xc_worker_close(worker);worker=nullptr;
 check(wait(request)==XC_OK,"Shutdown drains complete export");auto *allDrained=xc_request_take_search_text_export(request);xc_request_release(request);
 check(xc_search_text_export_get_info(allDrained,&info)==XC_OK&&info.disposition==XC_SEARCH_EXPORT_OK&&
  std::string(xc_search_text_export_text(all)).find("John 1:4")!=std::string::npos,"Complete copied payload survives worker lifetime");
 xc_search_text_export_release(all);xc_search_text_export_release(allDrained);
 // The earlier selected payload is also independent of the destroyed worker.
 check(std::string(xc_search_text_export_text(owned))==text,"Selected payload remains owned after all-export jobs");
 xc_search_text_export_release(owned);xc_search_text_export_release(nullptr);
 std::cout<<"C selected/complete text export, typed failures, copied inputs and owner lifetime passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
