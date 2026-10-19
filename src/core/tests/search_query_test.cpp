// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
xc_status wait(xc_request *r){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Composable query deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Original composable-query fixture required");const std::string root=argv[1];
 {StudyWorker worker(root);const auto a=worker.create_tab().get(),b=worker.create_tab().get();SearchQuery query{"Text","G140",SearchMode::lemma};query.text_options=SearchTextOptions(true,false,true);
  auto job=worker.search_module(a,query);auto state=worker.search_text_options(a);check(job.get().text_options_updated&&state.get().options->strongs&&!worker.search_text_options(b).get().options,"Worker state snapshots are FIFO and per-tab");
  worker.clear_search_scope(a).get();check(worker.search_text_options(a).get().options->footnotes,"Scope clear is independent of text state");
  check(!worker.clear_search_text_options(a).get().options,"Explicit reset returns unconfigured state");
 }
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open composable C worker");
 auto tab=[&]{check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create C query tab");const auto id=xc_request_tab(request);xc_request_release(request);return id;};const auto a=tab(),b=tab();
 xc_search_query query;xc_search_query_default(&query);xc_search_query_default(nullptr);xc_search_text_options text;xc_search_text_options_default(&text);xc_search_text_options_default(nullptr);xc_search_options options;xc_search_options_default(&options);
 check(!query.module&&!query.expression&&query.scope==XC_SEARCH_SCOPE_ALL&&!query.options&&!query.text_options&&!text.strongs&&!text.morphology&&!text.footnotes,"Composable/default optional inputs are explicit");
 xc_search_text_info info{9,9,9,9};check(xc_worker_search_text_options(worker,a,&request)==XC_OK&&xc_request_search_text_options(request,&info)==XC_INVALID_ARGUMENT&&!info.present,"Pending metadata cannot escape");
 check(wait(request)==XC_OK&&xc_request_search_text_options(request,&info)==XC_OK&&!info.present&&!xc_request_take_search(request),"Unconfigured text state is distinct from wrong request kind");xc_request_release(request);
 std::string module="Text",expression="G140";query.module=module.c_str();query.expression=expression.c_str();query.options=&options;query.text_options=&text;options.mode=XC_SEARCH_LEMMA;text={1,0,1};
 check(xc_worker_search(worker,a,&query,&request)==XC_OK,"Queue composable copied request");module="changed";expression="changed";text={0,1,0};options.mode=XC_SEARCH_PHRASE;query.scope=XC_SEARCH_SCOPE_LAST;
 check(wait(request)==XC_OK,"Complete copied text context");auto *seed=xc_request_take_search(request);xc_request_release(request);xc_search_info result{};
 check(xc_search_get_info(seed,&result)==XC_OK&&result.mode==XC_SEARCH_LEMMA&&result.result_count==2&&xc_search_requested_text_options(seed,&info)==XC_OK&&info.present&&info.strongs&&!info.morphology&&info.footnotes&&xc_search_text_options_updated(seed)==1,"Caller descriptor/buffers/flags/options are owned before return");
 check(xc_search_effective_text_options(seed,&info)==XC_OK&&info.strongs&&!info.morphology&&info.footnotes,"Effective prepared context is copied in result");
 query.module="Text";query.expression="G140";query.scope=XC_SEARCH_SCOPE_ALL;
 check(xc_worker_search(worker,a,&query,&request)==XC_OK&&wait(request)==XC_OK,"Phrase with changed checkbox input");auto *phrase=xc_request_take_search(request);xc_request_release(request);
 check(xc_search_requested_text_options(phrase,&info)==XC_OK&&!info.strongs&&info.morphology&&!info.footnotes&&xc_search_effective_text_options(phrase,&info)==XC_OK&&info.strongs&&!info.morphology&&info.footnotes&&xc_search_text_options_updated(phrase)==0,"Phrase ignores requested changes and materializes prior context");
 check(xc_worker_search_text_options(worker,b,&request)==XC_OK&&wait(request)==XC_OK&&xc_request_search_text_options(request,&info)==XC_OK&&!info.present,"C query contexts are isolated across tabs");xc_request_release(request);
 query.scope=XC_SEARCH_SCOPE_BOUNDS;query.range_module="Text";query.range_expression="John1:2-John1:3";options.mode=XC_SEARCH_MORPHOLOGY;query.expression="N-ASF";
 check(xc_worker_search(worker,a,&query,&request)==XC_OK&&wait(request)==XC_OK,"Composable bounds plus attribute/text options");auto *bounded=xc_request_take_search(request);xc_request_release(request);
 check(xc_search_get_info(bounded,&result)==XC_OK&&result.result_count==1&&std::string(xc_search_reference(bounded,0))=="John 1:3"&&xc_search_text_options_updated(bounded)==1,"Bounds and advanced flags share one engine path");
 query.scope=XC_SEARCH_SCOPE_LAST;query.range_module=nullptr;query.range_expression=nullptr;options.mode=XC_SEARCH_FOOTNOTE;query.expression="Original";query.text_options=nullptr;
 check(xc_worker_search(worker,a,&query,&request)==XC_OK&&wait(request)==XC_OK,"Composable last scope with retained settings");auto *last=xc_request_take_search(request);xc_request_release(request);
 check(xc_search_get_info(last,&result)==XC_OK&&result.result_count==1&&xc_search_within_last_results(last)==1&&xc_search_requested_text_options(last,&info)==XC_OK&&!info.present&&xc_search_effective_text_options(last,&info)==XC_OK&&info.present&&info.morphology,"Absent annotation input preserves private context");
 xc_search_query invalid=query;invalid.scope=static_cast<xc_search_scope_kind>(99);request=reinterpret_cast<xc_request *>(1);
 check(xc_worker_search(worker,a,&invalid,&request)==XC_INVALID_ARGUMENT&&!request,"Unknown scope clears stale output");
 invalid=query;invalid.range_module="Text";check(xc_worker_search(worker,a,&invalid,&request)==XC_INVALID_ARGUMENT&&!request,"Stray range data contradicting last scope rejects");
 invalid=query;invalid.scope=XC_SEARCH_SCOPE_BOUNDS;invalid.range_module="Text";check(xc_worker_search(worker,a,&invalid,&request)==XC_INVALID_ARGUMENT&&!request,"Missing bounds input rejects");
 invalid=query;invalid.text_options=&text;
 for(int field=0;field<3;++field){text={0,0,0};if(field==0)text.strongs=2;else if(field==1)text.morphology=-1;else text.footnotes=2;check(xc_worker_search(worker,a,&invalid,&request)==XC_INVALID_ARGUMENT&&!request,"Strict annotation boolean validation");}
 check(xc_worker_search(worker,a,nullptr,&request)==XC_INVALID_ARGUMENT&&!request&&xc_worker_search(nullptr,a,&query,&request)==XC_INVALID_ARGUMENT&&!request&&xc_worker_search(worker,a,&query,nullptr)==XC_INVALID_ARGUMENT,"NULL composable request contracts");
 check(xc_search_effective_text_options(nullptr,&info)==XC_INVALID_ARGUMENT&&!info.present&&xc_search_requested_text_options(seed,nullptr)==XC_INVALID_ARGUMENT&&xc_search_text_options_updated(nullptr)==-1,"Invalid result text getters clear output");
 check(xc_worker_clear_search_text_options(worker,a,&request)==XC_OK&&wait(request)==XC_OK&&xc_request_search_text_options(request,&info)==XC_OK&&!info.present&&xc_request_search_text_options(request,&info)==XC_OK,"Reset state metadata is non-consuming and unconfigured");xc_request_release(request);
 check(xc_worker_search_text_options(worker,999,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&xc_request_search_text_options(request,&info)==XC_INVALID_ARGUMENT&&!info.present,"Unknown tab errors cannot expose metadata");xc_request_release(request);
 check(xc_worker_search_text_options(worker,a,&request)==XC_OK,"Queue text metadata before close");xc_worker_close(worker);
 check(wait(request)==XC_OK&&xc_request_search_text_options(request,&info)==XC_OK&&!info.present,"Text state snapshot drains through shutdown");xc_request_release(request);
 check(xc_search_requested_text_options(seed,&info)==XC_OK&&info.strongs&&!info.morphology&&info.footnotes,"Owned requested context outlives later reset and worker close");
 xc_search_release(seed);xc_search_release(phrase);xc_search_release(bounded);xc_search_release(last);
 std::cout<<"Composable C search inputs, sticky context metadata, bounds, last scope, validation and lifetime passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
