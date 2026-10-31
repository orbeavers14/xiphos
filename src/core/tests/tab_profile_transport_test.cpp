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
static thread_local int fail_after=-1;
void *operator new(std::size_t n){if(watching){++allocations;if(fail_after==0)throw std::bad_alloc();if(fail_after>0)--fail_after;}if(auto *p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void *operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void *p)noexcept{std::free(p);}void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p,std::size_t)noexcept{std::free(p);}void operator delete[](void *p,std::size_t)noexcept{std::free(p);}
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static xc_status wait(xc_request *r){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(30);xc_status s;while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Tabs deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return s;}
template<class T>static void rejected(std::future<T> value,bool immediate=false){if(immediate)check(value.wait_for(std::chrono::seconds(0))==std::future_status::ready,"Admission rejection before FIFO");try{(void)value.get();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected tab profile rejection");}
int main(int argc,char **argv){try{
 check(argc==2,"Disposable tabs transport root required");const auto root=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(root);const auto file=root/"session.xml";std::filesystem::remove(file);
 SavedTabSession saved;saved.version="native";SavedTab tab;tab.text_module="Text";tab.reference="John3:16";tab.show_parallel="yes";saved.tabs={tab};
 {StudyWorker worker(root.u8string());const auto owner=worker.create_tab().get(),sibling=worker.create_tab().get();
  auto copied=saved;std::string path=file.u8string();auto pending=worker.save_tab_session(owner,path,copied);copied.tabs.clear();path.clear();auto read=worker.read_tab_session(owner,file.u8string());
  check(pending.get().tabs[0].reference=="John3:16"&&read.get().tabs[0].text_module=="Text","Copied inputs and FIFO read after save");
  check(worker.history(owner).get().entries.empty()&&worker.history(sibling).get().entries.empty()&&!worker.parallel_selection(owner).get()&&!worker.search_generation(owner).get().id,"File storage never restores/modifies active reading state");
  rejected(worker.read_tab_session(owner,std::string(4097,'p')),true);rejected(worker.read_tab_session(owner,std::string("a\0b",3)),true);
  auto invalid=saved;invalid.tabs[0].reference=std::string(1,'\1');rejected(worker.save_tab_session(owner,file.u8string(),invalid),true);
  worker.close_tab(owner).get();const auto absent=root/"closed.xml";std::filesystem::remove(absent);rejected(worker.save_tab_session(owner,absent.u8string(),saved));check(!std::filesystem::exists(absent),"Closed tab rejected before write");
  auto drain=saved;drain.version="drained";worker.save_tab_session(sibling,file.u8string(),drain);
 }
 check(read_saved_tab_session(file.u8string()).version=="drained","Shutdown drains dropped storage observer");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.u8string().c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C worker/tab");const auto owner=xc_request_tab(request);xc_request_release(request);
 std::string path=file.u8string(),version="C-version",text="Text&<>文字";xc_saved_tab tabs[2]{};
 const char *fields[]={text.c_str(),"Notes","Dictionary","Book","John3:16","key"," 12tail","yes","","yes","no","yes","yes"};
 for(size_t i=0;i<XC_TAB_FIELD_COUNT;++i)tabs[0].fields[i]=fields[i];
 xc_saved_tab_session input{version.c_str(),tabs,2};
 const std::string huge(32769,'x');auto bad=input;bad.version=huge.c_str();allocations=0;watching=true;auto status=xc_worker_save_tab_session(worker,owner,path.c_str(),&bad,&request);watching=false;check(status==XC_INVALID_ARGUMENT&&!request&&!allocations,"Oversized scalar rejects without observer/copy allocation");
 bad=input;bad.tab_count=257;check(xc_worker_save_tab_session(worker,owner,path.c_str(),&bad,&request)==XC_INVALID_ARGUMENT&&!request,"Count bound checked before array access");
 const std::string maximum(32768,'x');xc_saved_tab repeated{};for(auto &field:repeated.fields)field=maximum.c_str();std::vector<xc_saved_tab> repeated_tabs(256,repeated);bad={nullptr,repeated_tabs.data(),repeated_tabs.size()};
 allocations=0;watching=true;status=xc_worker_save_tab_session(worker,owner,path.c_str(),&bad,&request);watching=false;check(status==XC_INVALID_ARGUMENT&&!request&&!allocations,"Aggregate C byte bound checked before allocation");
 for(int failure:{0,1}){fail_after=failure;watching=true;status=xc_worker_save_tab_session(worker,owner,path.c_str(),&input,&request);watching=false;fail_after=-1;check(status==XC_ENGINE_ERROR&&!request,"Observer/result allocation failure rejects before acceptance");check(read_saved_tab_session(path).version=="drained","Allocation failure cannot commit a write");}
 check(xc_worker_save_tab_session(worker,owner,path.c_str(),&input,&request)==XC_OK,"C save accepted");path.clear();version.clear();text.clear();tabs[0]={};
 allocations=0;watching=true;status=wait(request);watching=false;check(status==XC_OK&&!allocations,"Committed tab snapshot publication allocates nothing");
 auto *owned=xc_request_take_tab_session(request);check(owned&&!xc_request_take_tab_session(request),"Take owned session once");xc_request_release(request);
 check(xc_tab_session_count(owned)==2&&std::string(xc_tab_session_version(owned))=="C-version"&&std::string(xc_tab_session_field(owned,0,XC_TAB_TEXT_MODULE))=="Text&<>文字","Nested copied arguments and version survive caller mutation");
 for(size_t i=1;i<XC_TAB_FIELD_COUNT;++i)check(std::string(xc_tab_session_field(owned,0,static_cast<int>(i)))==fields[i],"Every raw C field round trips");
 check(!xc_tab_session_field(owned,1,XC_TAB_TEXT_MODULE)&&!xc_tab_session_field(owned,0,-1)&&!xc_tab_session_field(owned,0,XC_TAB_FIELD_COUNT)&&!xc_tab_session_field(owned,9,0)&&!xc_tab_session_count(nullptr),"Safe missing/invalid snapshot access");
 xc_tab_visibility fallback{1,0,1,0},visibility{};check(xc_tab_session_visibility(owned,0,&fallback,&visibility)==XC_OK&&!visibility.texts&&visibility.preview&&!visibility.commentaries&&visibility.dictionaries,"Present empty showtexts uses per-tab flags");
 check(xc_tab_session_visibility(owned,1,&fallback,&visibility)==XC_OK&&visibility.texts&&!visibility.preview&&visibility.commentaries&&!visibility.dictionaries,"Absent showtexts inherits all defaults");
 check(xc_tab_session_commentary_showing(owned,0)&&xc_tab_session_parallel_showing(owned,0)&&!xc_tab_session_parallel_showing(owned,1),"Exact yes-token bool getters");
 auto invalid_fallback=fallback;invalid_fallback.texts=2;check(xc_tab_session_visibility(owned,0,&invalid_fallback,&visibility)==XC_INVALID_ARGUMENT&&!visibility.texts&&!visibility.preview&&!visibility.commentaries&&!visibility.dictionaries,"Invalid fallback clears output");
 check(xc_worker_read_tab_session(worker,owner,file.u8string().c_str(),&request)==XC_OK&&wait(request)==XC_OK,"C explicit storage read");auto *read=xc_request_take_tab_session(request);xc_request_release(request);xc_tab_session_release(read);
 input={"final",nullptr,0};check(xc_worker_save_tab_session(worker,owner,file.u8string().c_str(),&input,&request)==XC_OK,"C dropped clear accepted");xc_request_release(request);xc_worker_close(worker);check(read_saved_tab_session(file.u8string()).tabs.empty(),"C shutdown drains zero-tab save");
 std::filesystem::remove(file);check(std::string(xc_tab_session_field(owned,0,XC_TAB_REFERENCE))=="John3:16","Owned snapshot survives source/worker teardown");xc_tab_session_release(owned);xc_tab_session_release(nullptr);
 std::cout<<"Tab session FIFO/C copied storage, limits, ownership, visibility and no-allocation committed publication passed\n";return 0;
}catch(const std::exception &e){watching=false;std::cerr<<e.what()<<'\n';return 1;}}
