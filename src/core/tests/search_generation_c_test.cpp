// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
xc_status wait(xc_request *request){
 const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);
 xc_status status;
 while((status=xc_request_poll(request))==XC_PENDING){
  check(std::chrono::steady_clock::now()<end,"C generation deadline");
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
 }
 return status;
}
}
int main(int argc,char **argv){try{
 check(argc==2,"Original attribute fixture required");
 xc_worker *worker=nullptr;xc_request *request=nullptr;
 check(xc_worker_open(argv[1],&worker)==XC_OK,"Open worker");
 auto tab=[&]{check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create tab");
  const auto id=xc_request_tab(request);xc_request_release(request);return id;};
 const auto a=tab(),b=tab();
 xc_search_generation_info generation{};
 auto current=[&](uint64_t target){
  check(xc_worker_search_generation(worker,target,&request)==XC_OK&&wait(request)==XC_OK,"Query current generation");
  check(xc_request_search_generation(request,&generation)==XC_OK&&!xc_request_take_search(request),"Typed generation metadata");
  xc_request_release(request);return generation.id;
 };
 check(current(a)==0&&generation.retention==XC_SEARCH_RETENTION_NONE,"New tab has no generation");
 xc_search_retention_limits policy{};xc_search_retention_limits_default(&policy);
 check(policy.max_elements==100000&&policy.max_key_bytes==16*1024*1024,"Finite retention defaults");
 check(xc_worker_search_retention_limits(worker,a,&request)==XC_OK&&wait(request)==XC_OK&&
  xc_request_search_retention_limits(request,&policy)==XC_OK,"Read policy");
 generation.id=99;check(xc_request_search_generation(request,&generation)==XC_INVALID_ARGUMENT&&!generation.id,"Wrong kind clears metadata");
 xc_request_release(request);
 xc_search_query query;xc_search_query_default(&query);
 xc_search_options options;xc_search_options_default(&options);options.mode=XC_SEARCH_MULTIWORD;
 xc_search_text_options text{1,0,1};query.options=&options;query.text_options=&text;query.expression="Original";
 const char *modules[]={"Text","Missing","Commentary","Text"};
 xc_search_batch_limits budget;xc_search_batch_limits_default(&budget);budget.max_results=1;
 check(xc_worker_search_modules(worker,a,modules,4,&query,&budget,&request)==XC_OK&&wait(request)==XC_OK,"Create ordered generation");
 auto *batch=xc_request_take_search_batch(request);xc_request_release(request);
 check(batch&&xc_search_batch_generation_updated(batch)==1&&xc_search_batch_get_generation(batch,&generation)==XC_OK,"Owned batch identifies generation");
 const auto first=generation.id;
 check(first&&generation.retention==XC_SEARCH_RETENTION_COMPLETE&&generation.requested_count==4&&generation.group_count==4&&
  generation.failed_groups==1&&generation.visible_results==1&&generation.matched_elements==12&&generation.retained_elements==12,"Aggregate complete keys independent of visible prefix");
 xc_search_generation_info child{};
 check(xc_search_get_generation(xc_search_batch_group_result(batch,3),&child)==XC_OK&&child.id==first,"Each child names batch generation");
 xc_search_generation_read_options read;xc_search_generation_read_options_default(&read);
 check(read.source==XC_SEARCH_REFERENCES_COMPLETE&&read.max_results==1000&&read.max_reference_bytes==1024*1024,"Bounded complete read defaults");
 xc_search_generation_read_info disposition{};xc_search_query_metadata metadata{};
 auto readGroup=[&](uint64_t id,size_t index,const xc_search_generation_read_options *input=nullptr){
  check(xc_worker_read_search_generation(worker,a,id,index,input,&request)==XC_OK,"Queue read");
  disposition={99,XC_ENGINE_ERROR,1};
  check(xc_request_search_generation_read(request,&disposition)==XC_INVALID_ARGUMENT&&!disposition.result_present,"Getter requires explicit successful poll");
  check(wait(request)==XC_OK&&xc_request_search_generation_read(request,&disposition)==XC_OK,"Read disposition is separate from request status");
 };
 read.max_results=2;readGroup(first,0,&read);
 check(disposition.disposition==XC_SEARCH_READ_OK&&disposition.result_present&&xc_request_search_generation_query(request,&metadata)==XC_OK&&
  std::string(metadata.module)=="Text"&&std::string(metadata.expression)=="Original"&&metadata.mode==XC_SEARCH_MULTIWORD&&
  !metadata.within_last_results&&!metadata.range_module&&metadata.text_options.present&&metadata.text_options.strongs&&metadata.text_options.footnotes,"Original query metadata");
 check(xc_request_cancel_search(request)==XC_INVALID_ARGUMENT,"Read cannot cancel a different search");
 auto *owned=xc_request_take_search(request);xc_search_info info{};
 check(owned&&!xc_request_take_search(request)&&xc_search_get_info(owned,&info)==XC_OK&&info.result_count==2&&info.limited,"Bounded take-once full read");
 check(xc_request_search_generation_query(request,&metadata)==XC_OK&&xc_request_search_generation_read(request,&disposition)==XC_OK&&disposition.result_present,"Metadata survives take");
 xc_request_release(request);
 readGroup(first,2);auto *later=xc_request_take_search(request);xc_request_release(request);
 check(xc_search_get_info(later,&info)==XC_OK&&info.result_count==4&&!info.limited,"Later zero-prefix group retains full result");
 read.source=XC_SEARCH_REFERENCES_VISIBLE;readGroup(first,2,&read);
 auto *visible=xc_request_take_search(request);xc_request_release(request);
 check(xc_search_get_info(visible,&info)==XC_OK&&!info.result_count&&info.limited,"Visible read preserves original limitation");xc_search_release(visible);
 readGroup(first,1);
 check(disposition.disposition==XC_SEARCH_READ_FAILED_GROUP&&disposition.failure==XC_MODULE_NOT_FOUND&&!disposition.result_present&&
  !xc_request_take_search(request)&&xc_request_search_generation_query(request,&metadata)==XC_OK&&std::string(metadata.module)=="Missing","Failed group still owns requested query");xc_request_release(request);
 readGroup(first,4);metadata.module="stale";
 check(disposition.disposition==XC_SEARCH_READ_INVALID_GROUP&&xc_request_search_generation_query(request,&metadata)==XC_INVALID_ARGUMENT&&!metadata.module,"Invalid group has no invented query");xc_request_release(request);
 readGroup(first+1,0);check(disposition.disposition==XC_SEARCH_READ_STALE&&!xc_request_take_search(request),"Unknown ID never substitutes latest");xc_request_release(request);
 check(current(b)==0,"Tab generations isolated");
 policy={1,1024};
 check(xc_worker_set_search_retention_limits(worker,a,&policy,&request)==XC_OK,"Accept copied future policy");policy={999,999};
 check(wait(request)==XC_OK&&xc_request_search_retention_limits(request,&policy)==XC_OK&&policy.max_elements==1&&policy.max_key_bytes==1024,"Policy copied before return");xc_request_release(request);
 check(current(a)==first&&generation.retained_elements==12,"Policy update preserves current generation");
 query.module="Text";
 check(xc_worker_search(worker,a,&query,&request)==XC_OK&&wait(request)==XC_OK,"New search exceeds future policy");
 auto *single=xc_request_take_search(request);xc_request_release(request);
 check(xc_search_generation_updated(single)==1&&xc_search_get_generation(single,&generation)==XC_OK&&generation.id>first&&
  generation.retention==XC_SEARCH_RETENTION_CAPACITY_EXCEEDED&&!generation.retained_elements&&!generation.accounted_key_bytes,"Overflow invalidates full retention without failing search");
 const auto overflow=generation.id;
 readGroup(overflow,0);check(disposition.disposition==XC_SEARCH_READ_CAPACITY_EXCEEDED&&!xc_request_take_search(request),"Full overflow read explicitly unavailable");xc_request_release(request);
 read.max_results=1;read.source=XC_SEARCH_REFERENCES_VISIBLE;
 check(xc_worker_read_search_generation(worker,a,overflow,0,&read,&request)==XC_OK,"Accept copied read limits");read.source=99;read.max_results=0;
 check(wait(request)==XC_OK&&xc_request_search_generation_read(request,&disposition)==XC_OK&&disposition.disposition==XC_SEARCH_READ_OK,"Read options copied before return");visible=xc_request_take_search(request);xc_request_release(request);
 check(xc_search_get_info(visible,&info)==XC_OK&&info.result_count==1&&info.limited,"Visible fallback remains bounded");xc_search_release(visible);
 readGroup(first,0);check(disposition.disposition==XC_SEARCH_READ_STALE,"Replacement invalidates old generation");xc_request_release(request);
 xc_search_retention_limits_default(&policy);
 check(xc_worker_set_search_retention_limits(worker,a,&policy,&request)==XC_OK&&wait(request)==XC_OK,"Restore policy");xc_request_release(request);
 query.scope=XC_SEARCH_SCOPE_BOUNDS;query.range_module="Text";query.range_expression="John1:2-John1:3";options.case_sensitive=1;
 check(xc_worker_search(worker,a,&query,&request)==XC_OK&&wait(request)==XC_OK,"Bounded origin query");auto *bounded=xc_request_take_search(request);xc_request_release(request);
 check(xc_search_get_generation(bounded,&generation)==XC_OK,"Bounded generation");
 readGroup(generation.id,0);
 check(xc_request_search_generation_query(request,&metadata)==XC_OK&&metadata.case_sensitive&&std::string(metadata.range_module)=="Text"&&
  std::string(metadata.range_expression)=="John1:2-John1:3","Raw requested bounds copied independently from normalized result");xc_request_release(request);
 // Immediate validation clears observer outputs; metadata rejects NULL and wrong types.
 auto reject=[&]{request=reinterpret_cast<xc_request *>(1);check(xc_worker_read_search_generation(worker,a,generation.id,0,&read,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid read clears observer");};
 xc_search_generation_read_options_default(&read);read.source=2;reject();read.source=XC_SEARCH_REFERENCES_COMPLETE;read.max_results=0;reject();read.max_results=10001;reject();read.max_results=1;read.max_reference_bytes=8*1024*1024+1;reject();
 policy={0,1};request=reinterpret_cast<xc_request *>(1);
 check(xc_worker_set_search_retention_limits(worker,a,&policy,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid policy clears observer");
 check(xc_worker_set_search_retention_limits(worker,a,nullptr,&request)==XC_INVALID_ARGUMENT&&!request,"NULL policy rejects");
 generation.id=99;metadata.module="stale";disposition.result_present=1;policy.max_elements=99;
 check(xc_request_search_generation(nullptr,&generation)==XC_INVALID_ARGUMENT&&!generation.id&&
  xc_request_search_generation_query(nullptr,&metadata)==XC_INVALID_ARGUMENT&&!metadata.module&&
  xc_request_search_generation_read(nullptr,&disposition)==XC_INVALID_ARGUMENT&&!disposition.result_present&&
  xc_request_search_retention_limits(nullptr,&policy)==XC_INVALID_ARGUMENT&&!policy.max_elements,"NULL getters clear stale values");
 check(xc_search_generation_updated(nullptr)==-1&&xc_search_batch_generation_updated(nullptr)==-1&&
  xc_search_get_generation(nullptr,&generation)==XC_INVALID_ARGUMENT&&xc_search_batch_get_generation(nullptr,&generation)==XC_INVALID_ARGUMENT&&
  xc_request_search_generation(nullptr,nullptr)==XC_INVALID_ARGUMENT&&xc_request_search_generation_query(nullptr,nullptr)==XC_INVALID_ARGUMENT&&
  xc_request_search_generation_read(nullptr,nullptr)==XC_INVALID_ARGUMENT&&xc_request_search_retention_limits(nullptr,nullptr)==XC_INVALID_ARGUMENT,"NULL contracts");
 check(xc_worker_read_search_generation(worker,999,1,0,nullptr,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_search(request),"Unknown tab fails request");xc_request_release(request);
 const auto active=current(a);
 check(xc_worker_read_search_generation(worker,a,active,0,nullptr,&request)==XC_OK,"Accept read before worker shutdown");
 xc_worker_close(worker);check(wait(request)==XC_OK,"Shutdown drains retained read");auto *drained=xc_request_take_search(request);xc_request_release(request);
 check(std::string(xc_search_reference(owned,0))=="John 1:1"&&std::string(xc_search_reference(later,3))=="John 1:4"&&
  xc_search_get_info(drained,&info)==XC_OK&&info.result_count==2,"Copied results survive later searches and worker destruction");
 xc_search_release(owned);xc_search_release(later);xc_search_release(single);xc_search_release(bounded);xc_search_release(drained);xc_search_batch_release(batch);
 std::cout<<"C retained generations, stale reads, aggregate bounds, copied policy/query and ownership passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
