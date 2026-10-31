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
 check(argc==2,"Disposable bookmarks transport root required");const auto root=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(root);const auto file=root/"bookmarks.xml";std::filesystem::remove(file);
 BookmarkEdit folder;folder.kind=BookmarkKind::folder;folder.fields.caption="Personal";folder.fields.color="#123456";
 {StudyWorker worker(root.u8string());const auto tab=worker.create_tab().get(),sibling=worker.create_tab().get();
  auto edit=folder;std::string path=file.u8string();auto pending=worker.edit_bookmarks(tab,path,std::nullopt,edit);path.clear();edit.fields.caption="Changed";
  auto read=worker.read_bookmarks(tab,file.u8string());check(pending.get().items[0].caption=="Personal"&&read.get().items[0].caption=="Personal","Copied bookmark inputs and FIFO read after edit");
  check(worker.history(tab).get().entries.empty()&&worker.history(sibling).get().entries.empty()&&!worker.parallel_selection(tab).get()&&!worker.search_generation(tab).get().id,"Bookmark storage leaves reading/search/sibling state alone");
  rejected(worker.read_bookmarks(tab,std::string(4097,'p')),true);rejected(worker.read_bookmarks(tab,std::string("a\0b",3)),true);
  rejected(worker.edit_bookmarks(tab,file.u8string(),"invalid",folder),true);
  auto invalid=folder;invalid.fields.caption=std::string(1,'\1');rejected(worker.edit_bookmarks(tab,file.u8string(),std::nullopt,invalid),true);
  worker.close_tab(tab).get();const auto absent=root/"closed.xml";std::filesystem::remove(absent);rejected(worker.edit_bookmarks(tab,absent.u8string(),std::nullopt,folder));check(!std::filesystem::exists(absent),"Closed tab rejects before filesystem writes");
  BookmarkEdit update;update.action=BookmarkAction::update;update.kind=BookmarkKind::folder;update.target=0;update.fields.caption="Drained";
  worker.edit_bookmarks(sibling,file.u8string(),read_bookmarks(file.u8string()).revision,update);
 }
 check(read_bookmarks(file.u8string()).items[0].caption=="Drained","Dropped worker edit drains on shutdown");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.u8string().c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C worker/tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 std::string path=file.u8string(),revision=read_bookmarks(path).revision,key="John3:16",description="Original & <>文字";
 xc_bookmark_edit insert{};insert.action=XC_BOOKMARK_INSERT;insert.kind=XC_BOOKMARK_LEAF;insert.has_parent=1;insert.parent=0;insert.fields[XC_BOOKMARK_KEY]=key.c_str();insert.fields[XC_BOOKMARK_MODULE]="Missing";insert.fields[XC_BOOKMARK_DESCRIPTION]=description.c_str();insert.fields[XC_BOOKMARK_MODULE_DESCRIPTION]="Original module";
 auto bad=insert;bad.has_parent=2;allocations=0;watching=true;auto status=xc_worker_edit_bookmarks(worker,tab,path.c_str(),revision.c_str(),&bad,&request);watching=false;check(status==XC_INVALID_ARGUMENT&&!request&&!allocations,"Strict C bool admission allocates nothing");
 const std::string too_long(32769,'x');bad=insert;bad.fields[XC_BOOKMARK_KEY]=too_long.c_str();allocations=0;watching=true;status=xc_worker_edit_bookmarks(worker,tab,path.c_str(),revision.c_str(),&bad,&request);watching=false;check(status==XC_INVALID_ARGUMENT&&!request&&!allocations,"C scalar bounds precede observer/copy allocation");
 bad=insert;bad.action=99;check(xc_worker_edit_bookmarks(worker,tab,path.c_str(),revision.c_str(),&bad,&request)==XC_INVALID_ARGUMENT&&!request,"Strict action admission");
 for(int failure:{0,1}){fail_after=failure;watching=true;status=xc_worker_edit_bookmarks(worker,tab,path.c_str(),revision.c_str(),&insert,&request);watching=false;fail_after=-1;check(status==XC_ENGINE_ERROR&&!request,"C observer/result allocation failure before acceptance");check(read_bookmarks(path).revision==revision,"Observer allocation failure cannot publish write");}
 check(xc_worker_edit_bookmarks(worker,tab,path.c_str(),revision.c_str(),&insert,&request)==XC_OK,"C copied insert accepted");path.clear();revision.clear();key.clear();description.clear();insert={};
 allocations=0;watching=true;status=wait(request);watching=false;check(status==XC_OK&&!allocations,"Committed bookmark result polling allocates nothing");auto *owned=xc_request_take_bookmarks(request);check(owned&&!xc_request_take_bookmarks(request),"Take owned bookmark snapshot once");xc_request_release(request);
 check(xc_bookmarks_count(owned)==2&&std::string(xc_bookmarks_syntax_version(owned))=="1.0"&&std::string(xc_bookmarks_field(owned,1,XC_BOOKMARK_KEY))=="John3:16"&&std::string(xc_bookmarks_field(owned,1,XC_BOOKMARK_LABEL))=="Original & <>文字","Owned raw/derived C metadata and copied arguments");
 check(xc_bookmarks_kind(owned,0)==XC_BOOKMARK_FOLDER&&xc_bookmarks_kind(owned,1)==XC_BOOKMARK_LEAF&&xc_bookmarks_position(owned,1)==0&&!xc_bookmarks_ignored_subtrees(owned),"C hierarchy/type/position metadata");
 int has_parent=-1;size_t parent=99;check(xc_bookmarks_parent(owned,1,&has_parent,&parent)==XC_OK&&has_parent&&parent==0,"C copied parent index");check(xc_bookmarks_parent(owned,0,&has_parent,&parent)==XC_OK&&!has_parent&&parent==0,"Root parent is absent");
 check(xc_bookmarks_parent(owned,99,&has_parent,&parent)==XC_INVALID_ARGUMENT&&!has_parent&&!parent&&!xc_bookmarks_field(owned,1,99)&&!xc_bookmarks_field(owned,99,0)&&xc_bookmarks_kind(nullptr,0)==-1&&!xc_bookmarks_count(nullptr),"Invalid access safely clears outputs");
 const std::string old_revision=xc_bookmarks_revision(owned);
 xc_bookmark_edit update{};update.action=XC_BOOKMARK_UPDATE;update.kind=XC_BOOKMARK_LEAF;update.has_target=1;update.target=1;update.fields[XC_BOOKMARK_KEY]="John4:1";update.fields[XC_BOOKMARK_DESCRIPTION]="Changed";
 check(xc_worker_edit_bookmarks(worker,tab,file.u8string().c_str(),old_revision.c_str(),&update,&request)==XC_OK&&wait(request)==XC_OK,"C leaf update");auto *updated=xc_request_take_bookmarks(request);xc_request_release(request);
 check(std::string(xc_bookmarks_field(updated,1,XC_BOOKMARK_KEY))=="John4:1"&&!xc_bookmarks_field(updated,1,XC_BOOKMARK_MODULE),"C update clears optional module identity without resolving");
 check(xc_worker_edit_bookmarks(worker,tab,file.u8string().c_str(),old_revision.c_str(),&update,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT,"C stale revision fails at poll");check(!xc_request_take_bookmarks(request),"Failed edit cannot transfer a success snapshot");xc_request_release(request);
 xc_bookmark_edit move{};move.action=XC_BOOKMARK_MOVE;move.kind=XC_BOOKMARK_LEAF;move.has_target=1;move.target=1;move.position=1;
 check(xc_worker_edit_bookmarks(worker,tab,file.u8string().c_str(),xc_bookmarks_revision(updated),&move,&request)==XC_OK&&wait(request)==XC_OK,"C root move");auto *moved=xc_request_take_bookmarks(request);xc_request_release(request);xc_bookmarks_release(updated);check(xc_bookmarks_parent(moved,1,&has_parent,&parent)==XC_OK&&!has_parent&&xc_bookmarks_position(moved,1)==1,"C moved hierarchy");
 check(xc_worker_read_bookmarks(worker,tab,file.u8string().c_str(),&request)==XC_OK&&wait(request)==XC_OK,"C selected-file read");auto *read=xc_request_take_bookmarks(request);xc_request_release(request);check(std::string(xc_bookmarks_revision(read))==xc_bookmarks_revision(moved),"Read returns current exact revision");xc_bookmarks_release(read);
 xc_bookmark_edit remove{};remove.action=XC_BOOKMARK_REMOVE;remove.kind=XC_BOOKMARK_LEAF;remove.has_target=1;remove.target=1;
 check(xc_worker_edit_bookmarks(worker,tab,file.u8string().c_str(),xc_bookmarks_revision(moved),&remove,&request)==XC_OK,"Dropped C removal accepted");xc_request_release(request);xc_bookmarks_release(moved);xc_worker_close(worker);check(read_bookmarks(file.u8string()).items.size()==1,"C shutdown drains accepted dropped removal");
 std::filesystem::remove(file);check(std::string(xc_bookmarks_field(owned,1,XC_BOOKMARK_KEY))=="John3:16"&&std::string(xc_bookmarks_revision(owned))==old_revision,"Owned bookmark snapshot survives source/request/worker teardown and later edits");xc_bookmarks_release(owned);xc_bookmarks_release(nullptr);
 std::cout<<"Bookmark FIFO/C copied edits, stale guards, allocation-before-acceptance, no-allocation committed polling and ownership passed\n";return 0;
}catch(const std::exception &e){watching=false;std::cerr<<e.what()<<'\n';return 1;}}
