// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/engine_manager.hpp"
#include "search_source.hpp"
#include "search_control.hpp"
#include <rawtext.h>
#include <markupfiltmgr.h>
#include <listkey.h>
#include <future>
#include <thread>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
using namespace sword;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
struct ControlledModule:RawText {
 bool fail;ListKey result;std::shared_ptr<SearchControl> foreign_control;
 ControlledModule(const char *path,bool error,std::shared_ptr<SearchControl> foreign={}):RawText(path),fail(error),foreign_control(std::move(foreign)){}
 ListKey &search(const char *,int,int,SWKey *,bool *,void(*percent)(char,void *),void *user)override{
  terminateSearch=false;
  if(foreign_control){
   bool timely=true;
   std::thread emitter([&]{percent(1,user);const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(!foreign_control->cancel_requested()){if(std::chrono::steady_clock::now()>=end){timely=false;return;}std::this_thread::sleep_for(std::chrono::milliseconds(1));}percent(2,user);});
   emitter.join();check(timely&&!terminateSearch,"Foreign SDK callback may report atomic progress but cannot write the stop flag");
   percent(3,user);check(terminateSearch,"Owner callback applies the pending stop request");return result;
  }
  percent(1,user);
  if(fail)throw std::runtime_error("Original injected engine failure");
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);unsigned tick=0;
  while(!terminateSearch){check(std::chrono::steady_clock::now()<end,"Owner callback observes cancellation");std::this_thread::sleep_for(std::chrono::milliseconds(1));percent(static_cast<char>(2+(tick++%90)),user);}
  return result;
 }
};
xc_status wait(xc_request *request){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Controlled C request deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Original search fixture required");const std::string root=argv[1];
 {Reader reader(root);auto control=std::make_shared<SearchControl>();control->request_cancel();const auto stopped=reader.search_module({"Text","Original"},{},control);
  check(stopped.cancelled&&!stopped.executed&&stopped.references.empty()&&control->phase()==SearchPhase::cancelled&&!control->percent(),"Pre-dispatch stop returns owned cancelled result without SDK execution");
  try{(void)reader.search_module({"Text","Original"},{},control);throw std::logic_error("Expected reused control rejection");}catch(const std::invalid_argument &){}
 }
 auto control=std::make_shared<SearchControl>();
 auto engine=std::async(std::launch::async,[root,control]{
  EngineManager manager(root.c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);
  ControlledModule module((std::filesystem::path(root)/"data/text").string().c_str(),false);
  detail::SearchControlScope scope(control);auto value=detail::capture_module_search(manager,module,{"Text","Original"},{},control);scope.finish(value.cancelled);return value;
 });
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
 while(control->percent()<1){check(std::chrono::steady_clock::now()<deadline,"Progress begins on owner thread");std::this_thread::sleep_for(std::chrono::milliseconds(1));}
 check(control->phase()==SearchPhase::running,"Running progress precedes stop request");control->request_cancel();const auto stopped=engine.get();
 check(stopped.cancelled&&stopped.executed&&control->phase()==SearchPhase::cancelled&&control->percent()>=1&&control->percent()<=100,"Atomic cancellation reaches only the SDK owner callback and records cancelled execution");
 auto foreign=std::make_shared<SearchControl>();
 auto delegated=std::async(std::launch::async,[root,foreign]{EngineManager manager(root.c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);ControlledModule module((std::filesystem::path(root)/"data/text").string().c_str(),false,foreign);detail::SearchControlScope scope(foreign);auto value=detail::capture_module_search(manager,module,{"Text","Original"},{},foreign);scope.finish(value.cancelled);return value;});
 const auto foreign_end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
 while(foreign->percent()<1){check(std::chrono::steady_clock::now()<foreign_end,"Foreign callback publishes progress");std::this_thread::sleep_for(std::chrono::milliseconds(1));}
 foreign->request_cancel();check(delegated.get().cancelled&&foreign->phase()==SearchPhase::cancelled&&foreign->percent()==3,"Foreign progress is safe and owner callback retains interruption authority");
 auto failed=std::make_shared<SearchControl>();auto fault=std::async(std::launch::async,[root,failed]{EngineManager manager(root.c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);ControlledModule module((std::filesystem::path(root)/"data/text").string().c_str(),true);detail::SearchControlScope scope(failed);auto value=detail::capture_module_search(manager,module,{"Text","Original"},{},failed);scope.finish(value.cancelled);return value;});
 try{(void)fault.get();throw std::logic_error("Expected controlled engine failure");}catch(const std::runtime_error &){}
 check(failed->phase()==SearchPhase::failed&&failed->percent()==1,"Exception marks control failed without a result");
 {StudyWorker worker(root+"/absent-owned-search-control");auto task=worker.search_module_task(1,{"Text","Original"});try{(void)task.result.get();throw std::logic_error("Expected initialization failure");}catch(const std::invalid_argument &){}check(task.control->phase()==SearchPhase::failed,"Worker initialization failure cannot strand queued progress");}
 {StudyWorker worker(root);auto task=worker.search_module_task(999,{"Text","Original"});try{(void)task.result.get();throw std::logic_error("Expected missing tab failure");}catch(const std::invalid_argument &){}check(task.control->phase()==SearchPhase::failed,"Unknown-tab failure marks queued control terminal");}
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open controlled C worker");
 check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create controlled C tab");const auto tab=xc_request_tab(request);xc_search_progress progress{9,9,9};check(xc_request_cancel_search(request)==XC_INVALID_ARGUMENT&&xc_request_search_progress(request,&progress)==XC_INVALID_ARGUMENT&&!progress.percent&&!progress.cancel_requested,"Non-search requests do not expose search control");xc_request_release(request);
 check(xc_worker_search_module(worker,tab,"Text","Original",nullptr,&request)==XC_OK,"Queue controlled search");check(xc_request_cancel_search(request)==XC_OK&&xc_request_cancel_search(request)==XC_OK,"Repeated stop request is idempotent");check(wait(request)==XC_OK&&xc_request_search_progress(request,&progress)==XC_OK,"Cancelled/completed race still returns an owned result");auto *result=xc_request_take_search(request);xc_search_info info{};check(xc_search_get_info(result,&info)==XC_OK&&progress.percent<=100&&progress.phase==(info.cancelled?XC_SEARCH_CANCELLED:XC_SEARCH_COMPLETE),"C control terminal phase agrees with authoritative result disposition");xc_request_release(request);xc_search_release(result);
 check(xc_worker_search_module(worker,tab,"Text","Original",nullptr,&request)==XC_OK&&wait(request)==XC_OK&&xc_request_search_progress(request,&progress)==XC_OK&&progress.phase==XC_SEARCH_COMPLETE&&progress.percent==100,"Fresh search succeeds after cancellation");result=xc_request_take_search(request);check(xc_search_get_info(result,&info)==XC_OK&&!info.cancelled&&info.executed&&info.result_count==3,"Cancellation does not leak SDK stop state into next search");xc_request_release(request);xc_search_release(result);
 check(xc_worker_search_module(worker,tab,"Missing","Original",nullptr,&request)==XC_OK&&wait(request)==XC_MODULE_NOT_FOUND&&xc_request_search_progress(request,&progress)==XC_OK&&progress.phase==XC_SEARCH_FAILED,"Queued source error exposes failed progress without data");xc_request_release(request);
 check(xc_request_cancel_search(nullptr)==XC_INVALID_ARGUMENT&&xc_request_search_progress(nullptr,&progress)==XC_INVALID_ARGUMENT&&!progress.percent&&xc_request_search_progress(nullptr,nullptr)==XC_INVALID_ARGUMENT,"NULL control getter/cancel contracts");xc_worker_close(worker);
 std::cout<<"Owned search control, owner-only SDK interruption, cancellation races/recovery and terminal progress passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
