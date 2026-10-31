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
static void rejects(const std::function<void()> &f){static int attempt=0;++attempt;try{f();}catch(const std::exception &){return;}throw std::runtime_error("Expected document rejection at check "+std::to_string(attempt));}
static std::string bytes(const std::filesystem::path &file){std::ifstream in(file,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
int main(int argc,char **argv){try{
 check(argc==2,"Disposable editor root required");const auto root=std::filesystem::u8path(argv[1]);check(root.filename()=="editor-document-fixture","Only named disposable editor fixture may be replaced");std::filesystem::remove_all(root);std::filesystem::create_directories(root/"mods.d");const auto file=root/"original.spt";
 const std::string original="\xef\xbb\xbf<html>\r\n<p>Original Ω editor prose &amp; literal %20.</p>\r\n</html>\r\n";
 auto created=save_editable_document(file.u8string(),std::nullopt,original);check(created.text==original&&bytes(file)==original&&editable_document_revision(created.revision),"Create preserves UTF8 BOM, CRLF, markup and percent bytes");
 auto read=read_editable_document(file.u8string());check(read.text==original&&read.revision==created.revision,"Owned exact-byte read/revision");
 rejects([&]{save_editable_document(file.u8string(),std::nullopt,"unexpected overwrite");});rejects([&]{save_editable_document(file.u8string(),std::string(64,'0'),"stale overwrite");});check(bytes(file)==original,"Missing/stale revisions retain existing bytes");
 auto empty=save_editable_document(file.u8string(),created.revision,"");check(empty.text.empty()&&empty.revision=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"&&bytes(file).empty(),"Empty source is valid and SHA256 is independently known");
 const auto updated=save_editable_document(file.u8string(),empty.revision,"Original revised source\n");rejects([&]{save_editable_document(file.u8string(),created.revision,original);});check(bytes(file)==updated.text,"Old revision cannot replace a later save");
 {std::ofstream out(file,std::ios::binary);out<<"External original prose edit";}
 rejects([&]{save_editable_document(file.u8string(),updated.revision,"stale");});check(bytes(file)=="External original prose edit","External change is not overwritten");
 const auto gone=root/"missing.spt";rejects([&]{save_editable_document(gone.u8string(),updated.revision,"no");});rejects([&]{read_editable_document(gone.u8string());});check(!std::filesystem::exists(gone),"Missing replacement/read do not create files");
 for(const auto &text:{std::string("a\0b",3),std::string("\xff",1),std::string(maximum_editable_document_bytes+1,'a')})rejects([&]{save_editable_document(gone.u8string(),std::nullopt,text);});check(!std::filesystem::exists(gone),"NUL/invalidUTF8/oversize reject before write");
 rejects([&]{save_editable_document(file.u8string(),"invalid revision","no");});rejects([&]{read_editable_document(root.u8string());});
 const auto large=root/"large.spt";const std::string limit(maximum_editable_document_bytes,'x');auto max=save_editable_document(large.u8string(),std::nullopt,limit);check(read_editable_document(large.u8string()).text.size()==maximum_editable_document_bytes,"Exact byte limit is not silently trimmed");
 std::filesystem::permissions(file,std::filesystem::perms::owner_read|std::filesystem::perms::owner_write,std::filesystem::perm_options::replace);auto current=read_editable_document(file.u8string());save_editable_document(file.u8string(),current.revision,"Permission-preserved original prose");
 check((std::filesystem::status(file).permissions()&std::filesystem::perms::owner_write)!=std::filesystem::perms::none,"Replacement retains owner write permission");
 std::error_code symlink_error;const auto alias=root/"alias.spt";std::filesystem::create_symlink(file.filename(),alias,symlink_error);if(!symlink_error){auto aliased=read_editable_document(alias.u8string());save_editable_document(alias.u8string(),aliased.revision,"Original alias prose");check(std::filesystem::is_symlink(alias)&&bytes(file)=="Original alias prose","Symlink saves preserve alias and update target");}
 {StudyWorker worker(root.u8string());const auto tab=worker.create_tab().get();const auto captured=worker.capture_native_session({{tab,"empty"}}).get();auto doc=worker.save_editable_document(tab,(root/"worker.spt").u8string(),std::nullopt,original).get();check(doc.text==original&&worker.history(tab).get().entries.empty()&&worker.capture_native_session({{tab,"empty"}}).get().tabs[0].state.reading==std::nullopt,"Editing leaves tab reading/history unchanged");worker.close_tab(tab).get();rejects([&]{(void)worker.save_editable_document(tab,(root/"worker.spt").u8string(),doc.revision,"closed").get();});check(bytes(root/"worker.spt")==original,"Closed tab rejects before IO");}
 xc_worker *worker=nullptr;check(xc_worker_open(root.u8string().c_str(),&worker)==XC_OK,"C editor worker");xc_request *r=nullptr;check(xc_worker_create_tab(worker,&r)==XC_OK&&wait(r)==XC_OK,"C tab");const auto tab=xc_request_tab(r);xc_request_release(r);const auto path=(root/"c.spt").u8string();
 for(int fail:{0,1}){watching=true;fail_after=fail;auto status=xc_worker_save_editable_document(worker,tab,path.c_str(),nullptr,original.c_str(),&r);watching=false;fail_after=-1;check(status!=XC_OK&&!r&&!std::filesystem::exists(path),"Observer/result allocation failure leaves file missing");}
 watching=true;allocations=0;const auto invalid=xc_worker_save_editable_document(worker,tab,path.c_str(),"bad",original.c_str(),&r);watching=false;check(invalid==XC_INVALID_ARGUMENT&&allocations==0&&!r,"Invalid revision rejects before observer/copy allocation");
 std::string caller=original;check(xc_worker_save_editable_document(worker,tab,path.c_str(),nullptr,caller.c_str(),&r)==XC_OK,"C editor save accepted");caller="mutated caller";watching=true;allocations=0;fail_after=0;const auto status=wait(r);watching=false;fail_after=-1;check(status==XC_OK&&allocations==0,"Committed editor polling does not allocate");auto held=xc_request_take_editable_document(r);check(held&&!xc_request_take_editable_document(r),"Take once");xc_request_release(r);
 const auto revision=std::string(xc_editable_document_revision(held));check(xc_worker_save_editable_document(worker,tab,path.c_str(),revision.c_str(),"",&r)==XC_OK&&wait(r)==XC_OK,"C explicit empty source save");auto empty_handle=xc_request_take_editable_document(r);check(empty_handle&&xc_editable_document_bytes(empty_handle)==0,"Empty C document has owned result");xc_editable_document_release(empty_handle);xc_request_release(r);
 check(xc_worker_save_editable_document(worker,tab,path.c_str(),revision.c_str(),"stale",&r)==XC_OK&&wait(r)!=XC_OK,"C stale revision surfaces without write");xc_request_release(r);check(bytes(path).empty(),"Stale C save retains empty latest source");
 const auto drain=(root/"drain.spt").u8string();check(xc_worker_save_editable_document(worker,tab,drain.c_str(),nullptr,"Original drained editor prose",&r)==XC_OK,"Dropped editor save accepted");xc_request_release(r);xc_worker_close(worker);check(bytes(drain)=="Original drained editor prose","Accepted dropped save drains at shutdown");check(xc_editable_document_bytes(held)==original.size()&&std::string(xc_editable_document_text(held))==original,"Owned source survives caller mutation, request and worker teardown");xc_editable_document_release(held);
 std::cout<<"Exact editor source, revisions, failure retention and FIFO/C ownership passed\n";return 0;
 }catch(const std::exception &e){watching=false;std::cerr<<e.what()<<'\n';return 1;}}
