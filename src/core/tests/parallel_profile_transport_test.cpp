// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
static thread_local bool watching=false;
static thread_local std::size_t allocations=0;
void *operator new(std::size_t n){if(watching)++allocations;if(auto *p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void *operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void *p)noexcept{std::free(p);}void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p,std::size_t)noexcept{std::free(p);}void operator delete[](void *p,std::size_t)noexcept{std::free(p);}
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static xc_status wait(xc_request *request){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(30);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Profile deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
template<class T>static void rejected(std::future<T> value,bool immediate=false){if(immediate)check(value.wait_for(std::chrono::seconds(0))==std::future_status::ready,"Invalid input rejects before FIFO");try{(void)value.get();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected profile rejection");}
int main(int argc,char **argv){try{
 check(argc==2,"Disposable transport root required");const auto root=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(root);
 const auto file=root/"profile.xml";std::filesystem::remove(file);
 {StudyWorker worker(root.u8string());const auto tab=worker.create_tab().get(),sibling=worker.create_tab().get();
  std::string path=file.u8string();ParallelProfileEdit edit{ParallelProfileAction::save,"Study_Set","",{"Text","Missing","Text",""},true};
  auto pending=worker.edit_parallel_profile(tab,path,edit);path.clear();edit.key="Changed";edit.modules.clear();
  auto read=worker.read_parallel_profile(tab,file.u8string());
  check(pending.get().current_key=="Study_Set"&&read.get().startup_modules==std::vector<std::string>({"Text","Missing","Text",""}),"FIFO save/read use copied path, key and modules");
  check(!worker.parallel_selection(tab).get()&&!worker.parallel_selection(sibling).get()&&worker.history(tab).get().entries.empty()&&!worker.search_generation(tab).get().id,"Repository edits preserve all reading state and sibling tabs");
  rejected(worker.read_parallel_profile(tab,std::string(4097,'x')),true);
  rejected(worker.read_parallel_profile(tab,std::string("A\0B",3)),true);
  rejected(worker.edit_parallel_profile(tab,file.u8string(),{ParallelProfileAction::save,"names","",{},false}),true);
  worker.close_tab(tab).get();const auto absent=root/"closed.xml";std::filesystem::remove(absent);
  rejected(worker.edit_parallel_profile(tab,absent.u8string(),{ParallelProfileAction::save,"Closed","",{"Text"},false}));
  check(!std::filesystem::exists(absent),"Closed tab checked before filesystem writes");
  worker.edit_parallel_profile(sibling,file.u8string(),{ParallelProfileAction::rename,"Study_Set","Drained",{},false});
 }
 check(read_saved_parallel_profile(file.u8string()).current_key=="Drained","Shutdown drains dropped accepted edit");
 xc_worker *worker=nullptr;xc_request *request=nullptr;
 check(xc_worker_open(root.u8string().c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Open C worker/tab");
 const auto tab=xc_request_tab(request);xc_request_release(request);
 const std::string too_long(4097,'x');const char *items[]={"Text","Notes",""};xc_parallel_profile_edit edit{XC_PARALLEL_PROFILE_SAVE,"Copied_Set",nullptr,items,3,1};
 allocations=0;watching=true;const auto invalid=xc_worker_edit_parallel_profile(worker,tab,too_long.c_str(),&edit,&request);watching=false;
 check(invalid==XC_INVALID_ARGUMENT&&!request&&!allocations,"Oversized path rejects without allocation");
 auto bad=edit;bad.action=99;check(xc_worker_edit_parallel_profile(worker,tab,file.u8string().c_str(),&bad,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid action rejects before queue");
 bad=edit;bad.module_count=4097;check(xc_worker_edit_parallel_profile(worker,tab,file.u8string().c_str(),&bad,&request)==XC_INVALID_ARGUMENT&&!request,"Count rejects before reading array");
 bad=edit;bad.action=XC_PARALLEL_PROFILE_REMOVE;check(xc_worker_edit_parallel_profile(worker,tab,file.u8string().c_str(),&bad,&request)==XC_INVALID_ARGUMENT&&!request,"Unused fields reject");
 std::string key="Copied_Set",module="Text&<>";const char *copied_items[]={module.c_str(),"Notes",""};edit.key=key.c_str();edit.modules=copied_items;
 check(xc_worker_edit_parallel_profile(worker,tab,file.u8string().c_str(),&edit,&request)==XC_OK,"C persistent edit accepted");key.clear();module.clear();
 allocations=0;watching=true;const auto status=wait(request);watching=false;
 check(status==XC_OK&&!allocations,"Successful committed result polling allocates nothing");
 auto *owned=xc_request_take_parallel_profile(request);check(owned&&!xc_request_take_parallel_profile(request),"Take result once");xc_request_release(request);
 check(xc_parallel_profile_count(owned)==2&&std::string(xc_parallel_profile_key(owned,1))=="Copied_Set"&&std::string(xc_parallel_profile_display_name(owned,1))=="Copied Set"&&std::string(xc_parallel_profile_module(owned,1,0))=="Text&<>","Owned C metadata and copied nested arguments");
 check(xc_parallel_profile_module_count(owned,1)==3&&std::string(xc_parallel_profile_module(owned,1,2)).empty()&&xc_parallel_profile_startup_module_count(owned)==3&&xc_parallel_profile_startup_source(owned)==3,"C preserves blank module slots and effective startup source");
 check(!xc_parallel_profile_key(owned,9)&&!xc_parallel_profile_module(owned,1,9)&&!xc_parallel_profile_legacy_slot(owned,5)&&!xc_parallel_profile_count(nullptr)&&!xc_parallel_profile_current_key(nullptr),"Invalid C snapshot access is safe");
 check(xc_worker_read_parallel_profile(worker,tab,file.u8string().c_str(),&request)==XC_OK&&wait(request)==XC_OK,"C explicit read");auto *read=xc_request_take_parallel_profile(request);xc_request_release(request);
 check(std::string(xc_parallel_profile_names_text(read))=="Drained,Copied_Set"&&std::string(xc_parallel_profile_current_key(read))=="Copied_Set"&&std::string(xc_parallel_profile_active_modules_text(read))=="Text&<>,Notes,","C raw scalar getters");xc_parallel_profile_release(read);
 edit={XC_PARALLEL_PROFILE_RENAME,"Copied_Set","Final",nullptr,0,0};check(xc_worker_edit_parallel_profile(worker,tab,file.u8string().c_str(),&edit,&request)==XC_OK,"C dropped rename accepted");xc_request_release(request);xc_worker_close(worker);
 check(read_saved_parallel_profile(file.u8string()).current_key=="Final","C shutdown drains dropped edit");
 std::filesystem::remove(file);check(std::string(xc_parallel_profile_current_key(owned))=="Copied_Set"&&std::string(xc_parallel_profile_startup_modules_text(owned))=="Text&<>,Notes,","Owned result survives later write/source deletion/worker teardown");xc_parallel_profile_release(owned);xc_parallel_profile_release(nullptr);
 std::cout<<"Parallel profile FIFO/C ownership, admission, copied inputs, no-allocation publication and draining passed\n";return 0;
}catch(const std::exception &e){watching=false;std::cerr<<e.what()<<'\n';return 1;}}
