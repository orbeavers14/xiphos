// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
xc_status wait(xc_request *r){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Scope request deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Original scope-worker fixture required");const std::string root=argv[1];
 std::future<SearchScopeInfo> drained;
 {
  StudyWorker worker(root);const auto a=worker.create_tab().get(),b=worker.create_tab().get();
  const auto catalog=worker.modules().get();for(const auto &module:catalog)check(module.verse_search==(module.id=="Text"||module.id=="Commentary"),"Shared catalog capability matches actual initial verse-search gate");
  auto seed=worker.search_module(a,{"Text","Original"},{1,1024});auto info=worker.last_search_scope(a);auto other=worker.last_search_scope(b);
  check(seed.get().references.size()==1&&info.get().element_count==3&&!other.get().present,"FIFO complete scope does not inherit display truncation or another tab's state");
  SearchQuery last{"Commentary","beta"};last.within_last_results=true;
  auto next=worker.search_module(a,last).get();check(next.scope_updated&&next.scope.element_count==2&&next.references.size()==2,"Worker executes shared complete last scope");
  check(!worker.clear_search_scope(a).get().present&&!worker.search_module(a,last).get().executed,"Clear and next query are FIFO serialized");
  auto invalid=worker.last_search_scope(999);try{invalid.get();throw std::logic_error("Expected invalid tab");}catch(const std::invalid_argument &){}
  worker.close_tab(a).get();try{worker.clear_search_scope(a).get();throw std::logic_error("Expected closed tab");}catch(const std::invalid_argument &){}
  worker.search_module(b,{"Text","Original"}).get();drained=worker.last_search_scope(b);
 }
 check(drained.get().element_count==3,"Copied scope metadata drains and survives worker shutdown");
 {
  xc_reader *reader=nullptr;check(xc_open(root.c_str(),&reader)==XC_OK,"Open sync capability reader");
  for(std::size_t i=0;i<xc_module_count(reader);++i){const std::string id=xc_module_id(reader,i);check(xc_module_has_verse_search(reader,i)==(id=="Text"||id=="Commentary"),"Sync C catalog exposes shared capability");}
  check(!xc_module_has_verse_search(nullptr,0)&&!xc_module_has_verse_search(reader,999),"Invalid sync capability getters are safe");xc_close(reader);
 }
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open C scope worker");
 auto tab=[&]{check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create C tab");const auto id=xc_request_tab(request);xc_request_release(request);return id;};const auto a=tab(),b=tab();
 check(xc_worker_modules(worker,&request)==XC_OK&&wait(request)==XC_OK,"Queue copied capability catalog");auto *catalog=xc_request_take_catalog(request);xc_request_release(request);
 for(std::size_t i=0;i<xc_catalog_count(catalog);++i){const std::string id=xc_catalog_id(catalog,i);check(xc_catalog_has_verse_search(catalog,i)==(id=="Text"||id=="Commentary"),"Queued C catalog preserves shared capability");}
 check(!xc_catalog_has_verse_search(nullptr,0)&&!xc_catalog_has_verse_search(catalog,999),"Invalid copied capability getters are safe");
 xc_search_scope_info info{9,9};
 check(xc_worker_last_search_scope(worker,a,&request)==XC_OK&&xc_request_search_scope(request,&info)==XC_INVALID_ARGUMENT&&!info.present&&!info.element_count,"Pending scope metadata cannot escape before poll");
 check(wait(request)==XC_OK&&xc_request_search_scope(request,&info)==XC_OK&&!info.present&&!info.element_count&&!xc_request_take_search(request),"Empty scope metadata has a distinct, non-consuming request kind");
 check(xc_request_search_scope(request,&info)==XC_OK,"Repeated metadata reads are safe");xc_request_release(request);
 xc_search_options options;xc_search_options_default(&options);options.max_results=1;
 check(xc_worker_search_module(worker,a,"Text","Original",&options,&request)==XC_OK&&wait(request)==XC_OK,"Seed complete C scope with truncated display");auto *seed=xc_request_take_search(request);
 check(xc_request_search_scope(request,&info)==XC_INVALID_ARGUMENT&&!info.present&&xc_search_get_scope(seed,&info)==XC_OK&&info.present&&info.element_count==3&&xc_search_scope_updated(seed)==1&&xc_search_within_last_results(seed)==0,"Owned result exposes complete committed scope metadata without wrong-kind request aliasing");xc_request_release(request);
 std::string query="beta";check(xc_worker_search_module_in_last_results(worker,a,"Commentary",query.c_str(),nullptr,&request)==XC_OK,"Queue copied last-scope query");query="unrelated";
 check(wait(request)==XC_OK,"Complete C last-scope query");auto *last=xc_request_take_search(request);xc_request_release(request);xc_search_info result{};
 check(xc_search_get_info(last,&result)==XC_OK&&result.result_count==2&&result.executed&&xc_search_within_last_results(last)==1&&xc_search_get_scope(last,&info)==XC_OK&&info.element_count==2&&std::string(xc_search_expression(last))=="beta"&&!xc_search_range_module(last),"C copies requested scope/query and full narrowed ownership");
 check(xc_worker_search_module_in_last_results(worker,b,"Text","Original",nullptr,&request)==XC_OK&&wait(request)==XC_OK,"Other C tab remains empty");auto *empty=xc_request_take_search(request);xc_request_release(request);check(xc_search_get_info(empty,&result)==XC_OK&&!result.executed&&!result.result_count&&xc_search_scope_updated(empty)==0,"Absent last scope skips rather than searches all");xc_search_release(empty);
 check(xc_worker_clear_search_scope(worker,a,&request)==XC_OK&&wait(request)==XC_OK&&xc_request_search_scope(request,&info)==XC_OK&&!info.present,"C clear resets retained scope");xc_request_release(request);
 check(xc_worker_last_search_scope(worker,999,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&xc_request_search_scope(request,&info)==XC_INVALID_ARGUMENT&&!info.present,"Unknown tab is a queued error with no metadata");xc_request_release(request);
 request=reinterpret_cast<xc_request *>(1);check(xc_worker_last_search_scope(nullptr,a,&request)==XC_INVALID_ARGUMENT&&!request&&xc_worker_clear_search_scope(worker,a,nullptr)==XC_INVALID_ARGUMENT,"Invalid worker/output clears stale request");
 check(xc_worker_search_module_in_last_results(worker,a,nullptr,"Original",nullptr,&request)==XC_INVALID_ARGUMENT&&!request,"NULL last-scope query source rejects");
 check(xc_search_get_scope(nullptr,&info)==XC_INVALID_ARGUMENT&&!info.present&&!info.element_count&&xc_search_get_scope(seed,nullptr)==XC_INVALID_ARGUMENT&&xc_request_search_scope(nullptr,&info)==XC_INVALID_ARGUMENT&&xc_search_scope_updated(nullptr)==-1&&xc_search_within_last_results(nullptr)==-1,"Null metadata getter contracts");
 check(xc_worker_last_search_scope(worker,a,&request)==XC_OK,"Queue final metadata before close");xc_worker_close(worker);check(wait(request)==XC_OK&&xc_request_search_scope(request,&info)==XC_OK&&!info.present,"Metadata request survives worker shutdown");xc_request_release(request);
 check(xc_search_get_scope(seed,&info)==XC_OK&&info.element_count==3&&xc_search_get_scope(last,&info)==XC_OK&&info.element_count==2,"Earlier result scope copies survive subsequent clear and worker destruction");check(xc_catalog_count(catalog)==4,"Copied capability catalog survives worker destruction");
 for(std::size_t i=0;i<xc_catalog_count(catalog);++i){const std::string id=xc_catalog_id(catalog,i);check(xc_catalog_has_verse_search(catalog,i)==(id=="Text"||id=="Commentary"),"Capability values retain their owned catalog after shutdown");}xc_catalog_release(catalog);
 xc_search_release(seed);xc_search_release(last);
 std::cout<<"Queued complete scope, copied C metadata, clear, tab isolation and shutdown passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
