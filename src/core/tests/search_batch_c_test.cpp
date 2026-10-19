// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <string>
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
xc_status wait(xc_request *request){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Batch C deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Original batch C fixture required");xc_worker *worker=nullptr;xc_request *request=nullptr;
 check(xc_worker_open(argv[1],&worker)==XC_OK,"Open batch C worker");
 auto tab=[&]{check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create batch tab");auto id=xc_request_tab(request);xc_request_release(request);return id;};const auto a=tab(),b=tab();
 xc_search_query query;xc_search_query_default(&query);xc_search_options options;xc_search_options_default(&options);options.mode=XC_SEARCH_MULTIWORD;
 xc_search_text_options text{1,0,1};query.options=&options;query.text_options=&text;
 std::string first="Text",expression="Original";const char *modules[]={first.c_str(),"Missing","Commentary","Text"};query.expression=expression.c_str();
 xc_search_batch_limits budget;xc_search_batch_limits_default(&budget);check(budget.max_results==10000&&budget.max_reference_bytes==8*1024*1024,"Batch defaults are bounded across all modules");budget.max_results=1;
 check(xc_worker_search_modules(worker,a,modules,4,&query,&budget,&request)==XC_OK,"Accept copied batch");
 first="changed";expression="unrelated";modules[1]="Text";options.mode=XC_SEARCH_FOOTNOTE;options.max_results=7;text={0,1,0};budget.max_results=8;query.scope=XC_SEARCH_SCOPE_LAST;
 check(!xc_request_take_search_batch(request)&&!xc_request_take_search(request),"Batch take requires successful poll and has its own request kind");
 check(wait(request)==XC_OK,"Group failure still completes an owned batch");xc_search_progress progress{};
 check(xc_request_search_progress(request,&progress)==XC_OK&&progress.phase==XC_SEARCH_COMPLETE&&progress.percent==100,"One terminal progress control spans all groups");
 auto *batch=xc_request_take_search_batch(request);check(batch&&!xc_request_take_search_batch(request)&&!xc_request_take_search(request),"Batch is take once and cannot be taken as single search");xc_request_release(request);
 xc_search_batch_info info{};check(xc_search_batch_get_info(batch,&info)==XC_OK&&info.requested_count==4&&info.group_count==4&&info.failure_count==1&&info.result_count==1&&info.reference_bytes==8&&info.limited&&!info.cancelled&&info.scope_updated&&info.text_options_updated,"Owned batch summary copies aggregate budgets, errors and publication");
 check(std::string(xc_search_batch_requested_module(batch,0))=="Text"&&std::string(xc_search_batch_requested_module(batch,1))=="Missing"&&!xc_search_batch_requested_module(batch,4),"All requested module IDs are owned in order");
 for(size_t i=0;i<4;++i){xc_search_batch_group_info group{};check(xc_search_batch_get_group(batch,i,&group)==XC_OK&&group.request_index==i&&group.status==(i==1?XC_MODULE_NOT_FOUND:XC_OK)&&group.result_present==(i!=1),"Group index and status preserve missing module position");
  const auto *result=xc_search_batch_group_result(batch,i);if(i==1){check(!result,"Failure has no false result");continue;}
  xc_search_info single{};check(xc_search_get_info(result,&single)==XC_OK&&single.mode==XC_SEARCH_MULTIWORD&&single.executed&&single.result_count==(i?0:1)&&std::string(xc_search_expression(result))=="Original","Borrowed const result supports existing getters with copied query/options");
 }
 xc_search_scope_info scope{};xc_search_text_info annotations{};
 check(xc_search_batch_get_scope(batch,&scope)==XC_OK&&scope.present&&scope.element_count==4&&xc_search_batch_get_text_options(batch,&annotations)==XC_OK&&annotations.present&&annotations.strongs&&!annotations.morphology&&annotations.footnotes,"Final full scope and annotation context are independent of displayed prefix");
 query={};xc_search_query_default(&query);query.expression="N-ASF";options={};xc_search_options_default(&options);options.mode=XC_SEARCH_MORPHOLOGY;query.options=&options;const char *pair[]={"Text","Commentary"};
 check(xc_worker_search_modules(worker,a,pair,2,&query,nullptr,&request)==XC_OK&&wait(request)==XC_OK,"Seed complete scope through batch");auto *seed=xc_request_take_search_batch(request);xc_request_release(request);xc_search_batch_release(seed);
 query.expression="G140";options.mode=XC_SEARCH_LEMMA;query.scope=XC_SEARCH_SCOPE_LAST;
 check(xc_worker_search_modules(worker,a,pair,2,&query,nullptr,&request)==XC_OK&&wait(request)==XC_OK,"Common last-results query chains through modules");auto *last=xc_request_take_search_batch(request);xc_request_release(request);
 check(xc_search_batch_get_scope(last,&scope)==XC_OK&&scope.element_count==1&&std::string(xc_search_reference(xc_search_batch_group_result(last,1),0))=="John 1:1","Last-results batch narrows full typed scope in order");
 query.scope=XC_SEARCH_SCOPE_BOUNDS;query.range_module="Text";std::string bounds="John1:2-John1:3";query.range_expression=bounds.c_str();query.expression="Original";options.mode=XC_SEARCH_PHRASE;
 check(xc_worker_search_modules(worker,b,pair,2,&query,nullptr,&request)==XC_OK,"Queue copied batch bounds");bounds="bad";query.range_module="Missing";
 check(wait(request)==XC_OK,"Copied range provider survives changed descriptor");auto *bounded=xc_request_take_search_batch(request);xc_request_release(request);
 check(xc_search_batch_get_scope(bounded,&scope)==XC_OK&&scope.element_count==2&&std::string(xc_search_range_module(xc_search_batch_group_result(bounded,0)))=="Text","Range applies to each module through common shared service");
 // Null/shape validation clears stale observer outputs before accepting anything.
 query.scope=XC_SEARCH_SCOPE_ALL;query.range_module=nullptr;query.range_expression=nullptr;
 auto rejects=[&](const char *const *ids,size_t count,const xc_search_query *input,const xc_search_batch_limits *total){request=reinterpret_cast<xc_request *>(1);check(xc_worker_search_modules(worker,a,ids,count,input,total,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid batch shape clears request output");};
 rejects(nullptr,2,&query,nullptr);rejects(pair,0,&query,nullptr);rejects(pair,33,&query,nullptr);rejects(pair,2,nullptr,nullptr);
 const char *missing[]={"Text",nullptr};rejects(missing,2,&query,nullptr);
 query.module="Text";rejects(pair,2,&query,nullptr);query.module=nullptr;query.scope=static_cast<xc_search_scope_kind>(3);rejects(pair,2,&query,nullptr);
 query.scope=XC_SEARCH_SCOPE_BOUNDS;rejects(pair,2,&query,nullptr);query.scope=XC_SEARCH_SCOPE_ALL;query.range_module="Text";rejects(pair,2,&query,nullptr);query.range_module=nullptr;
 options.mode=8;rejects(pair,2,&query,nullptr);options.mode=XC_SEARCH_PHRASE;options.case_sensitive=2;rejects(pair,2,&query,nullptr);options.case_sensitive=0;
 text={2,0,0};query.text_options=&text;rejects(pair,2,&query,nullptr);query.text_options=nullptr;
 budget={0,1024};rejects(pair,2,&query,&budget);budget={1,8*1024*1024+1};rejects(pair,2,&query,&budget);
 check(xc_worker_search_modules(nullptr,a,pair,2,&query,nullptr,&request)==XC_INVALID_ARGUMENT&&!request&&xc_worker_search_modules(worker,a,pair,2,&query,nullptr,nullptr)==XC_INVALID_ARGUMENT,"NULL worker/output contracts");
 xc_search_batch_group_info group{99,XC_ENGINE_ERROR,1};info={};info.result_count=99;scope={1,99};annotations={1,1,1,1};
 check(xc_search_batch_get_info(nullptr,&info)==XC_INVALID_ARGUMENT&&!info.result_count&&xc_search_batch_get_group(batch,99,&group)==XC_INVALID_ARGUMENT&&!group.result_present&&!group.request_index&&xc_search_batch_get_scope(nullptr,&scope)==XC_INVALID_ARGUMENT&&!scope.present&&xc_search_batch_get_text_options(nullptr,&annotations)==XC_INVALID_ARGUMENT&&!annotations.present,"Invalid scalar getters clear stale data");
 check(xc_search_batch_get_info(batch,nullptr)==XC_INVALID_ARGUMENT&&xc_search_batch_get_group(batch,0,nullptr)==XC_INVALID_ARGUMENT&&xc_search_batch_get_scope(batch,nullptr)==XC_INVALID_ARGUMENT&&xc_search_batch_get_text_options(batch,nullptr)==XC_INVALID_ARGUMENT&&!xc_search_batch_group_result(nullptr,0)&&!xc_search_batch_group_result(batch,99)&&!xc_search_batch_requested_module(nullptr,0)&&!xc_request_take_search_batch(nullptr),"NULL/bounds getter contracts");
 query.expression="Original";
 check(xc_worker_search_modules(worker,999,pair,2,&query,nullptr,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_search_batch(request),"Unknown tab fails at poll without result");xc_request_release(request);
 query.expression="";check(xc_worker_search_modules(worker,a,pair,2,&query,nullptr,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_search_batch(request),"Queued malformed query fails whole batch");xc_request_release(request);query.expression="Original";
 check(xc_worker_search_modules(worker,a,pair,2,&query,nullptr,&request)==XC_OK&&xc_request_cancel_search(request)==XC_OK&&wait(request)==XC_OK,"Cancelled or completion-race batch returns owned disposition");auto *race=xc_request_take_search_batch(request);check(xc_search_batch_get_info(race,&info)==XC_OK&&xc_request_search_progress(request,&progress)==XC_OK&&info.cancelled==(progress.phase==XC_SEARCH_CANCELLED),"Cancellation summary agrees with batch control");xc_request_release(request);xc_search_batch_release(race);
 check(xc_worker_search_modules(worker,a,pair,2,&query,nullptr,&request)==XC_OK,"Accept detached observer batch");xc_request_release(request);
 check(xc_worker_last_search_scope(worker,a,&request)==XC_OK&&wait(request)==XC_OK&&xc_request_search_scope(request,&scope)==XC_OK&&scope.element_count==4&&!xc_request_take_search_batch(request),"Releasing observer neither cancels accepted work nor changes FIFO ownership");xc_request_release(request);
 check(xc_worker_search_modules(worker,a,pair,2,&query,nullptr,&request)==XC_OK,"Accept batch before shutdown");xc_worker_close(worker);check(wait(request)==XC_OK,"Shutdown drains accepted batch");auto *drained=xc_request_take_search_batch(request);xc_request_release(request);
 check(std::string(xc_search_reference(xc_search_batch_group_result(batch,0),0))=="John 1:1"&&std::string(xc_search_expression(xc_search_batch_group_result(drained,1)))=="Original"&&xc_search_batch_get_scope(last,&scope)==XC_OK&&scope.element_count==1,"Batch metadata and borrowed group strings survive later jobs and worker destruction");
 xc_search_batch_release(batch);xc_search_batch_release(last);xc_search_batch_release(bounded);xc_search_batch_release(drained);xc_search_batch_release(nullptr);
 std::cout<<"Copied C multi-module queries, bounded groups, progress, validation, take-once and borrowed result lifetime passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
