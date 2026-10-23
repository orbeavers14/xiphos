// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/study_link_admission.hpp"
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <new>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
// Watch only the submitting thread, not unrelated SDK initialization/owner work.
// Rejected ABI input must not allocate an observer or an owned argument copy.
static thread_local bool watching=false;
static thread_local std::size_t largest=0;
void *operator new(std::size_t n) { if(watching&&n>largest)largest=n; if(auto *p=std::malloc(n?n:1))return p;throw std::bad_alloc(); }
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p,std::size_t) noexcept { std::free(p); }
void operator delete[](void *p,std::size_t) noexcept { std::free(p); }
void check(bool ok,const char *message) {if(!ok)throw std::runtime_error(message);}
xc_status wait(xc_request *r) {const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);xc_status status;while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Link admission deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
template<class T>void rejected(std::future<T> value) {check(value.wait_for(std::chrono::seconds(0))==std::future_status::ready,"Rejected worker inputs return an already completed future");try{value.get();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected admission rejection");}
int main(int argc,char **argv){try {
 check(argc==2,"Original alias fixture required");const std::string root=argv[1];
 const std::string hugeUri(XC_STUDY_LINK_MAX_URI_BYTES+1,'u'),hugeContext(XC_STUDY_LINK_MAX_CONTEXT_BYTES+1,'c');
 std::string fullUri="sword://AFirst/John1:1";fullUri.resize(XC_STUDY_LINK_MAX_URI_BYTES,' ');
 std::string unicodeContext;for(unsigned i=0;i<2048;++i)unicodeContext+="é";
 const std::string fullContext(XC_STUDY_LINK_MAX_CONTEXT_BYTES,'c');
 {StudyWorker worker(root);const auto tab=worker.create_tab().get();
  const auto generation=worker.search_module(tab,{"AFirst","Original"}).get().generation.id;
  rejected(worker.visit_linked_passage(tab,hugeContext,"",{"AFirst","John1:1"}));
  rejected(worker.visit_linked_passage(tab,"AFirst",hugeContext,{"AFirst","John1:1"}));
  rejected(worker.visit_linked_passage(tab,"AFirst","",{hugeContext,"John1:1"}));
  rejected(worker.visit_linked_passage(tab,"AFirst","",{"AFirst",hugeUri}));
  rejected(worker.visit_linked_passage(tab,"AFirst","",{"AFirst","John1:1",hugeUri}));
  rejected(worker.visit_linked_passage(tab,"AFirst","",{"AFirst","John1:1",std::string("a\0b",3)}));
  rejected(worker.follow_book_pane_link_at(tab,"AFirst","John1:1",hugeUri));
  rejected(worker.follow_book_pane_link_at(tab,hugeContext,"John1:1","sword://AFirst/John1:1"));
  rejected(worker.follow_book_pane_link_at(tab,"AFirst",hugeContext,"sword://AFirst/John1:1"));
  rejected(worker.resolve_link("AFirst",hugeUri));rejected(worker.resolve_study_link(tab,"AFirst",hugeUri));
  rejected(worker.study_note_link(tab,"AFirst",hugeUri));rejected(worker.resolve_study_link_at(tab,"AFirst","John1:1",hugeUri));
  rejected(worker.follow_study_link_at(tab,"AFirst",hugeContext,"sword://AFirst/John1:1"));
  rejected(worker.hover_study_link_at(tab,hugeContext,"John1:1","sword://AFirst/John1:1"));
  rejected(worker.follow_pane_link_at(tab,hugeContext,"AFirst","John1:1","sword://AFirst/John1:1"));
  rejected(worker.follow_pane_link_at(tab,"AFirst","AFirst","John1:1",std::string("sword://AFirst/John1:1\0tail",25)));
  const auto exact=worker.resolve_link(unicodeContext,fullUri).get();check(exact.module=="AFirst"&&exact.reference.size()==fullUri.size()-std::string("sword://AFirst/").size(),"Exact byte bounds and Unicode context remain admitted without forced normalization");
  std::string copied="sword://Shared/John1:1";auto valid=worker.resolve_link("AFirst",copied);copied.clear();check(valid.get().module=="BSecond","Valid requests still copy before caller mutation");
  check(worker.search_generation(tab).get().id==generation&&worker.history(tab).get().entries.empty(),"Rejections and target resolution preserve search/history state");
  worker.close_tab(tab).get();rejected(worker.follow_study_link_at(tab,"AFirst","John1:1",hugeUri));
 }
 xc_worker *worker=nullptr;xc_request *request=nullptr;
 check(xc_worker_open(root.c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C owner/tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 using Submit=std::function<xc_status(const char *,const char *,const char *,xc_request **)>;
 struct Route {Submit call;bool context;};
 const std::vector<Route> routes={
  {[&](const char *m,const char *,const char *u,xc_request **o){return xc_worker_resolve_link(worker,m,u,o);},false},
  {[&](const char *m,const char *,const char *u,xc_request **o){return xc_worker_resolve_study_link(worker,tab,m,u,o);},false},
  {[&](const char *m,const char *,const char *u,xc_request **o){return xc_worker_study_note_link(worker,tab,m,u,o);},false},
  {[&](const char *m,const char *c,const char *u,xc_request **o){return xc_worker_resolve_study_link_at(worker,tab,m,c,u,o);},true},
  {[&](const char *m,const char *c,const char *u,xc_request **o){return xc_worker_follow_study_link_at(worker,tab,m,c,u,o);},true},
  {[&](const char *m,const char *c,const char *u,xc_request **o){return xc_worker_hover_study_link_at(worker,tab,m,c,u,o);},true},
  {[&](const char *m,const char *c,const char *u,xc_request **o){return xc_worker_follow_pane_link_at(worker,tab,"AFirst",m,c,u,o);},true},
  {[&](const char *m,const char *c,const char *u,xc_request **o){return xc_worker_follow_book_pane_link_at(worker,tab,m,c,u,o);},true}
 };
 const std::vector<char> unterminated(XC_STUDY_LINK_MAX_URI_BYTES+1,'x');
 for(bool anchored:{false,true})for(unsigned field=0;field<(anchored?5u:4u);++field){
  const char *bible=field==0?hugeContext.c_str():"AFirst",*commentary=field==1?hugeContext.c_str():"",*target=field==2?hugeContext.c_str():"AFirst",*reference=field==3?unterminated.data():"John1:1",*anchor=field==4?unterminated.data():"";
  request=reinterpret_cast<xc_request *>(1);largest=0;watching=true;
  const auto status=anchored?xc_worker_visit_linked_passage_with_anchor(worker,tab,bible,commentary,target,reference,anchor,&request):xc_worker_visit_linked_passage(worker,tab,bible,commentary,target,reference,&request);
  watching=false;check(status==XC_INVALID_ARGUMENT&&!request&&largest==0,"Both linked-visit C variants reject oversized/unterminated fields before copies/observer allocation");
 }

 for(const auto &route:routes)for(unsigned which=0;which<4;++which){
  if(which==2&&!route.context)continue;
  request=reinterpret_cast<xc_request *>(1);largest=0;watching=true;
  const auto status=route.call(which==1?hugeContext.c_str():"AFirst",which==2?hugeContext.c_str():"John1:1",which==3?unterminated.data():(which==0?hugeUri.c_str():"sword://AFirst/John1:1"),&request);
  watching=false;check(status==XC_INVALID_ARGUMENT&&!request&&largest==0,"Oversized/unterminated C input rejects before copying or observer allocation");
 }
 request=reinterpret_cast<xc_request *>(1);check(xc_worker_follow_pane_link_at(worker,tab,hugeContext.c_str(),"AFirst","John1:1","sword://AFirst/John1:1",&request)==XC_INVALID_ARGUMENT&&!request,"Navigation module is bounded independently");
 check(xc_worker_follow_study_link_at(worker,tab,unicodeContext.c_str(),fullContext.c_str(),fullUri.c_str(),&request)==XC_OK&&wait(request)==XC_OK,"Exact byte caps admitted through C contextual boundary");auto *target=xc_request_take_link(request);xc_request_release(request);check(target&&std::string(xc_link_module(target))=="AFirst","Actual identity preserved at admission cap");xc_link_release(target);
 check(xc_worker_resolve_link(worker,nullptr,"sword://AFirst/John1:1",&request)==XC_OK&&wait(request)==XC_OK,"Optional null current module remains supported with explicit URI target");target=xc_request_take_link(request);xc_request_release(request);xc_worker_close(worker);check(target&&std::string(xc_link_module(target))=="AFirst","Accepted results own strings after shutdown");xc_link_release(target);
 {xc_reader *reader=nullptr;check(xc_open(root.c_str(),&reader)==XC_OK,"Synchronous C owner");xc_link *link=reinterpret_cast<xc_link *>(1);largest=0;watching=true;const auto status=xc_resolve_link(reader,"AFirst",hugeUri.c_str(),&link);watching=false;check(status==XC_INVALID_ARGUMENT&&!link&&!largest,"Synchronous reader admission rejects before argument/result allocation");
  check(xc_resolve_link(reader,unicodeContext.c_str(),fullUri.c_str(),&link)==XC_OK,"Synchronous exact caps");xc_close(reader);check(std::string(xc_link_module(link))=="AFirst","Synchronous owned target lifetime");xc_link_release(link);}
 std::cout<<"Link byte admission, no-copy/no-observer C rejection, exact UTF-8 caps, futures, state and lifetime passed\n";return 0;
 }catch(const std::exception &e){watching=false;std::cerr<<e.what()<<'\n';return 1;}}
