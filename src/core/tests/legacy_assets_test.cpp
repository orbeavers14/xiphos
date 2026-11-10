// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/remote_install.hpp"
#include "../bounded_file.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <new>
#include <cstdlib>
// One-shot allocation failures let rollback itself allocate normally.
static thread_local bool watching=false;
static thread_local std::size_t allocations=0;
static thread_local long long fail_after=-1;
void *operator new(std::size_t n){if(watching){++allocations;if(fail_after==0){fail_after=-1;throw std::bad_alloc();}if(fail_after>0)--fail_after;}if(auto *p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void *operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void *p)noexcept{std::free(p);}void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p,std::size_t)noexcept{std::free(p);}void operator delete[](void *p,std::size_t)noexcept{std::free(p);}
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool v,const char *why){if(!v)throw std::runtime_error(why);}
template<class F>void rejects(F f){try{f();}catch(const std::exception &){return;}throw std::runtime_error("Expected asset rejection");}
void write(const fs::path &p,const std::string &v){std::ofstream out(p,std::ios::binary);out<<v;}
std::string read(const fs::path &p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
xc_status wait(xc_request *r){auto until=std::chrono::steady_clock::now()+std::chrono::seconds(30);xc_status s;while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<until,"Asset request timeout");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return s;}
int main(int argc,char **argv)try{
 check(argc==2,"Disposable root required");auto root=fs::u8path(argv[1]);check(root.filename()=="legacy-assets-fixture","Named fixture only");fs::remove_all(root);fs::create_directories(root/"old/bookmarks");fs::create_directories(root/"old/pads/empty");fs::create_directories(root/"library/mods.d");auto source=root/"old";write(source/"bookmarks/bookmarks.xml","<SwordBookmarks custom='retain'/>");write(source/"pads/café.pad",std::string("Original Ω")+'\0'+"binary");write(source/".hidden","private");write(source/"template.pad","");
 StudyWorker worker((root/"library").u8string());auto plan=worker.inspect_legacy_assets(source.u8string()).get();check(plan.entries.size()==7&&plan.bytes>0,"Inventory includes hidden/empty files and empty directories");auto copied=worker.copy_legacy_assets(source.u8string(),plan.revision,(root/"copy").u8string()).get();check(copied.revision==plan.revision&&fs::is_directory(root/"copy/pads/empty")&&read(root/"copy/pads/café.pad")==read(source/"pads/café.pad"),"Complete binary Unicode tree verified without conversion");
 rejects([&]{copy_legacy_assets(source.u8string(),plan.revision,(root/"copy").u8string());});rejects([&]{copy_legacy_assets(source.u8string(),plan.revision,(source/"inside").u8string());});
 write(source/".hidden","changed");rejects([&]{copy_legacy_assets(source.u8string(),plan.revision,(root/"stale").u8string());});check(!fs::exists(root/"stale"),"Stale plan publishes nothing");write(source/".hidden","private");
 std::error_code linkError;fs::create_symlink(source/"template.pad",source/"linked",linkError);if(!linkError){rejects([&]{inspect_legacy_assets(source.u8string());});fs::remove(source/"linked");}else std::cout<<"Symlink fixture unavailable on this host\n";write(source/"CON.txt","unsupported portable name");rejects([&]{inspect_legacy_assets(source.u8string());});fs::remove(source/"CON.txt");
 linkError.clear();fs::create_symlink(root/"missing",root/"dangling",linkError);if(!linkError){rejects([&]{copy_legacy_assets(source.u8string(),plan.revision,(root/"dangling").u8string());});check(fs::is_symlink(fs::symlink_status(root/"dangling")),"Dangling destination symlink is preserved");fs::remove(root/"dangling");}
 write(source/"oversize","");fs::resize_file(source/"oversize",128ull*1024*1024+1);rejects([&]{inspect_legacy_assets(source.u8string());});fs::remove(source/"oversize");
 xc_worker *cw=nullptr;check(xc_worker_open((root/"library").u8string().c_str(),&cw)==XC_OK,"C worker");xc_request *request=nullptr;std::string selected=source.u8string();check(xc_worker_inspect_legacy_assets(cw,selected.c_str(),&request)==XC_OK,"C review admission");selected.clear();check(wait(request)==XC_OK,"C reviewed");auto owned=xc_request_take_legacy_assets(request);check(owned&&!xc_request_take_legacy_assets(request),"Owned take once");xc_request_release(request);xc_worker_close(cw);check(xc_legacy_assets_count(owned)==7&&std::string(xc_legacy_assets_revision(owned))==plan.revision,"Review survives worker teardown");xc_legacy_asset row{};check(xc_legacy_assets_get(owned,0,&row)==XC_OK&&row.path,"Borrowed entry");check(xc_legacy_assets_get(owned,999,&row)==XC_INVALID_ARGUMENT&&!row.path&&!row.bytes,"Invalid getter clears output");xc_legacy_assets_release(owned);
 check(xc_worker_open((root/"library").u8string().c_str(),&cw)==XC_OK,"C copy worker");std::string copiedSource=source.u8string(),copiedRevision=plan.revision,copiedTarget=(root/"c-copy").u8string();check(xc_worker_copy_legacy_assets(cw,copiedSource.c_str(),copiedRevision.c_str(),copiedTarget.c_str(),&request)==XC_OK,"C copy admission");copiedSource.clear();copiedRevision.clear();copiedTarget.clear();check(wait(request)==XC_OK,"C copy uses admission-time input copies");owned=xc_request_take_legacy_assets(request);xc_request_release(request);xc_worker_close(cw);check(owned&&std::string(xc_legacy_assets_revision(owned))==plan.revision&&read(root/"c-copy/pads/café.pad")==read(source/"pads/café.pad"),"C copy result owns verified tree after teardown");xc_legacy_assets_release(owned);
 auto cancelled=std::make_shared<TransferControl>();check(cancelled->request_cancel(),"Pre-cancel wins unused transfer gate");bool sawCancel=false;try{inspect_legacy_assets(source.u8string(),cancelled);}catch(const TransferCancelled &){sawCancel=true;}check(sawCancel&&cancelled->progress().phase==TransferPhase::cancelled,"Pre-cancelled review reports cancellation");
 cancelled=std::make_shared<TransferControl>();cancelled->request_cancel();sawCancel=false;try{copy_legacy_assets(source.u8string(),plan.revision,(root/"cancelled-copy").u8string(),cancelled);}catch(const TransferCancelled &){sawCancel=true;}check(sawCancel&&!fs::exists(root/"cancelled-copy"),"Cancellation before commit publishes no copy");
 write(root/"chunked",std::string(65536,'x'));unsigned checkpoints=0;sawCancel=false;try{detail::read_bounded_file(root/"chunked",65536,"Cancel fixture",[&]{if(++checkpoints==3)throw TransferCancelled();});}catch(const TransferCancelled &){sawCancel=true;}check(sawCancel&&checkpoints==3,"Bounded reads observe cooperative cancellation inside a file");
 auto completed=std::make_shared<TransferControl>();auto completedCopy=copy_legacy_assets(source.u8string(),plan.revision,(root/"controlled-copy").u8string(),completed);check(completedCopy.revision==plan.revision&&completed->progress().phase==TransferPhase::complete&&!completed->request_cancel(),"Cancellation after commit loses and completed copy remains successful");
 write(source/"large.bin",std::string(32*1024*1024,'x'));auto largePlan=inspect_legacy_assets(source.u8string());auto active=worker.copy_legacy_assets_task(source.u8string(),largePlan.revision,(root/"active-copy").u8string());const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);bool accepted=false;
 while(active.result.wait_for(std::chrono::seconds(0))!=std::future_status::ready){check(std::chrono::steady_clock::now()<deadline,"Active copy cancellation deadline");auto progress=active.control->progress();if(progress.phase==TransferPhase::extracting&&progress.expanded){accepted=active.control->request_cancel();break;}std::this_thread::yield();}
 sawCancel=false;try{(void)active.result.get();}catch(const TransferCancelled &){sawCancel=true;}check(accepted?sawCancel&&!fs::exists(root/"active-copy"):!sawCancel,"Active cancellation and commit have one atomic winner");
 const auto concurrent = root/"concurrent-copy";
 auto competing = worker.copy_legacy_assets_task(source.u8string(),largePlan.revision,concurrent.u8string());
 const auto raceDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
 while(competing.control->progress().phase!=TransferPhase::extracting){
  check(competing.result.wait_for(std::chrono::seconds(0))!=std::future_status::ready &&
        std::chrono::steady_clock::now()<raceDeadline,"Reach private staging before concurrent destination creation");
  std::this_thread::yield();
 }
 check(fs::create_directory(concurrent),"Create competing empty directory after copy starts");
 rejects([&]{(void)competing.result.get();});
 check(fs::is_directory(concurrent)&&fs::is_empty(concurrent),"Publication preserves a competing empty directory");
 fs::remove(source/"large.bin");
 check(xc_worker_open((root/"library").u8string().c_str(),&cw)==XC_OK,"C cancellable worker");check(xc_worker_copy_legacy_assets_task(cw,source.u8string().c_str(),plan.revision.c_str(),(root/"c-cancel").u8string().c_str(),&request)==XC_OK,"C cancellable admission");auto ccontrol=xc_request_take_transfer_control(request);check(ccontrol&&!xc_request_take_transfer_control(request),"C control take once before completion");auto caccepted=xc_transfer_control_cancel(ccontrol);allocations=0;watching=true;auto cstatus=wait(request);watching=false;check(!allocations,"C cancellable result polling allocates nothing");check(caccepted?cstatus==XC_CANCELLED&&!fs::exists(root/"c-cancel"):cstatus==XC_OK,"C cancellation maps to the actual publication winner");xc_transfer_control_release(ccontrol);xc_request_release(request);xc_worker_close(cw);
 const auto faultSource=source.u8string(),faultDestination=(root/"fault-copy").u8string();
 allocations=0;watching=true;auto faultBaseline=copy_legacy_assets(faultSource,plan.revision,faultDestination);watching=false;const auto sites=allocations;fs::remove_all(root/"fault-copy");check(sites>0&&sites<10000,"Bounded copy allocation-failure matrix");
 std::size_t failures=0;
 for(std::size_t site=0;site<sites;++site){bool failed=false;fail_after=static_cast<long long>(site);watching=true;try{auto result=copy_legacy_assets(faultSource,plan.revision,faultDestination);watching=false;}catch(const std::bad_alloc &){watching=false;failed=true;}catch(...){watching=false;throw;}fail_after=-1;
  if(failed){++failures;check(!fs::exists(root/"fault-copy"),"An allocation failure never reports failure after publishing a copy");}else fs::remove_all(root/"fault-copy");
  for(const auto &entry:fs::directory_iterator(root))check(entry.path().filename().u8string().rfind(".xiphos-assets-",0)!=0,"One-shot allocation failure rolls back owned staging");
 }
 check(failures>0,"Copy allocation failures were exercised");std::cout<<"Checked "<<sites<<" allocation boundaries\n";
 check(inspect_legacy_assets(source.u8string()).revision==plan.revision,"Source remains unchanged");std::cout<<"Legacy complete-tree asset review/copy checks passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
