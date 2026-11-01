// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "remote_io.hpp"
#include <rawtext.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <thread>
using namespace xiphos::core;
namespace fs=std::filesystem;
static thread_local bool watching=false;static thread_local int fail_after=-1;static thread_local std::size_t allocations=0;
void *operator new(std::size_t n){if(watching){++allocations;if(fail_after==0)throw std::bad_alloc();if(fail_after>0)--fail_after;}if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void *operator new[](std::size_t n){return ::operator new(n);}void operator delete(void *p)noexcept{std::free(p);}void operator delete[](void *p)noexcept{std::free(p);}void operator delete(void *p,std::size_t)noexcept{std::free(p);}void operator delete[](void *p,std::size_t)noexcept{std::free(p);}
static void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>static void rejects(F f){try{f();}catch(const std::exception &){return;}throw std::runtime_error("Expected remote transfer rejection");}
static xc_status wait(xc_request *r){auto until=std::chrono::steady_clock::now()+std::chrono::seconds(40);xc_status s;while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<until,"Remote request deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return s;}
static void fixture(const fs::path &root,const std::string &id){
 fs::create_directories(root/"mods.d");auto data=root/"data"/id;fs::create_directories(data);check(sword::RawText::createModule(data.u8string().c_str())==0,"Create original remote fixture");
 {sword::RawText writer(data.u8string().c_str());writer.setKey("John3:16");writer.setEntry(("Original HTTPS fixture prose "+id).c_str());}
 std::ofstream out(root/"mods.d"/(id+".conf"));out<<'['<<id<<"]\nDataPath=./data/"<<id<<"/\nModDrv=RawText\nEncoding=UTF-8\nSourceType=Plain\nLang=en\nDescription=Original HTTPS fixture\nVersion=1\nCopyright=Original fixture\n";
 std::ofstream unicode(data/fs::u8path("original-Ω.txt"));unicode<<"Original Unicode asset";
 std::ofstream asset(data/"large-original.bin",std::ios::binary);for(unsigned i=0;i<128*1024;++i)asset.put(static_cast<char>(i%251));
}
int main(int argc,char **argv){try{
 check(argc>=3,"Disposable root and fixture mode/port required");auto root=fs::u8path(argv[1]);check(root.filename()=="remote-install-fixture","Named disposable remote fixture only");
 if(std::string(argv[2])=="--fixture"){fs::remove_all(root);fixture(root/"installed","Base");fixture(root/"source","Added");return 0;}
 const auto port=static_cast<std::uint16_t>(std::stoul(argv[2]));TransferOptions options;options.workspace=root.u8string();options.ca_bundle=(root/"certificate.pem").u8string();options.remote_access_acknowledged=true;options.timeout_seconds=15;
 HttpsRepository repository{"fixture","127.0.0.1","/catalog","/packages",port};
 // All archive cases are generated locally, with no licensed content or network.
 for(const auto &file:fs::directory_iterator(root/"bad")){
  auto destination=root/("expanded-"+file.path().stem().string());TransferControl control;
  auto bounded=options;if(file.path().filename()=="expansion.zip")bounded.maximum_expanded_bytes=1024;
  rejects([&]{detail::extract_repository_archive(file.path(),destination,file.path().extension()==".gz"?detail::RepositoryArchive::catalog:detail::RepositoryArchive::module,bounded,control);});check(!fs::exists(destination),"Rejected archive leaves no extracted tree");
 }
 {TransferControl control;check(control.request_cancel(),"Queued cancel wins");rejects([&]{detail::TransferAccess::commit(control);});}
 {TransferControl control;detail::TransferAccess::commit(control);check(!control.request_cancel(),"Commit wins late cancellation");detail::TransferAccess::finish(control);}
 {StudyWorker worker((root/"installed").u8string());auto tab=worker.create_tab().get();(void)worker.visit(tab,"Base","John3:16").get();auto history=worker.history(tab).get();
  auto untrusted=options;untrusted.ca_bundle.clear();rejects([&]{(void)worker.fetch_remote_catalog(repository,untrusted).result.get();});
  auto denied=options;denied.remote_access_acknowledged=false;rejects([&]{(void)worker.fetch_remote_catalog(repository,denied).result.get();});
  for(const auto *route:{"/downgrade","/notfound","/corrupt"}){auto bad=repository;bad.catalog_directory=route;rejects([&]{(void)worker.fetch_remote_catalog(bad,options).result.get();});}
  auto tiny=options;tiny.maximum_download_bytes=8;rejects([&]{(void)worker.fetch_remote_catalog(repository,tiny).result.get();});
  auto chunked=repository;chunked.catalog_directory="/chunked";rejects([&]{(void)worker.fetch_remote_catalog(chunked,tiny).result.get();});
  auto little=options;little.maximum_expanded_bytes=1024;rejects([&]{(void)worker.fetch_remote_catalog(repository,little).result.get();});
  auto redirect=repository;redirect.catalog_directory="/redirect";auto catalog=worker.fetch_remote_catalog(redirect,options).result.get();check(catalog.catalog.items.size()==1&&catalog.archive_sha256.size()==64,"Trusted HTTPS redirect/catalog extraction");catalog.repository=repository;
  auto cancelled=catalog;cancelled.repository.package_directory="/slow";auto task=worker.install_remote_module(cancelled,"Added",options);
  auto until=std::chrono::steady_clock::now()+std::chrono::seconds(10);while(task.control->progress().downloaded<4096){check(std::chrono::steady_clock::now()<until,"Slow download progress deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}
  check(task.control->request_cancel(),"In-flight cancel wins before commit");rejects([&]{(void)task.result.get();});check(task.control->progress().phase==TransferPhase::cancelled&&worker.modules().get().size()==1,"Cancelled transfer leaves library unchanged");
  auto mismatch=catalog;mismatch.catalog.items.front().version="2";mismatch.revision=library_catalog_revision(mismatch.catalog);rejects([&]{(void)worker.install_remote_module(mismatch,"Added",options).result.get();});
  auto external=catalog;{std::ofstream out(root/"installed/external.txt");out<<"External library change";}rejects([&]{(void)worker.install_remote_module(external,"Added",options).result.get();});fs::remove(root/"installed/external.txt");
  check(worker.history(tab).get().entries.size()==history.entries.size()&&worker.read_entry(tab,"Base","John3:16").get().html.find("Original HTTPS")!=std::string::npos,"Transfer failures preserve reading/history");
 }
 xc_worker *worker=nullptr;check(xc_worker_open((root/"installed").u8string().c_str(),&worker)==XC_OK,"C HTTPS worker");xc_request *r=nullptr;xc_transfer_options o{};xc_transfer_options_default(&o);const auto workspace=root.u8string(),ca=(root/"certificate.pem").u8string();o.workspace=workspace.c_str();o.ca_bundle=ca.c_str();o.remote_access_acknowledged=1;o.timeout_seconds=15;
 xc_https_repository slow_repo{"fixture","127.0.0.1","/catalog","/slow",port};check(xc_worker_fetch_remote_catalog(worker,&slow_repo,&o,&r)==XC_OK&&wait(r)==XC_OK,"C cancellation catalog");auto slow_catalog=xc_request_take_remote_catalog(r);xc_request_release(r);
 check(xc_worker_install_remote_module(worker,slow_catalog,"Added",0,&o,&r)==XC_OK,"C cancellable install accepted");xc_remote_catalog_release(slow_catalog);auto cancelled_control=xc_request_take_transfer_control(r);xc_transfer_progress cancelling{};auto cancel_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
 do{check(xc_transfer_control_progress(cancelled_control,&cancelling)==XC_OK,"C cancellation progress");check(std::chrono::steady_clock::now()<cancel_deadline,"C cancellation deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(cancelling.downloaded<4096);
 check(xc_transfer_control_cancel(cancelled_control)&&wait(r)==XC_CANCELLED&&!xc_request_take_remote_install_result(r),"C cancellation has a distinct terminal status and no installed result");xc_request_release(r);xc_transfer_control_release(cancelled_control);
 xc_https_repository repo{"fixture","127.0.0.1","/catalog","/packages",port};check(xc_worker_fetch_remote_catalog(worker,&repo,&o,&r)==XC_OK&&wait(r)==XC_OK,"C HTTPS catalog");auto catalog=xc_request_take_remote_catalog(r);check(catalog&&!xc_request_take_remote_catalog(r),"Remote catalog take once");xc_request_release(r);
 for(int fail:{0,1,2}){watching=true;fail_after=fail;auto s=xc_worker_install_remote_module(worker,catalog,"Added",0,&o,&r);watching=false;fail_after=-1;check(s!=XC_OK&&!r,"Remote observer/result/cancel envelopes precede acceptance");}
 check(xc_worker_install_remote_module(worker,catalog,"Added",0,&o,&r)==XC_OK,"C HTTPS installation accepted");auto control=xc_request_take_transfer_control(r);check(control&&!xc_request_take_transfer_control(r),"Take prepared transfer control once");xc_remote_catalog_release(catalog);
 watching=true;fail_after=0;allocations=0;auto status=wait(r);watching=false;fail_after=-1;check(status==XC_OK,"HTTPS install did not commit successfully");check(allocations==0,"Committed HTTPS result polling does not allocate");auto owned=xc_request_take_remote_install_result(r);xc_request_release(r);xc_worker_close(worker);
 xc_local_install_done done{};check(xc_local_install_result_get(xc_remote_install_result_installed(owned),&done)==XC_OK&&std::string(done.module)=="Added"&&!done.replaced,"Remote result owns successful local commit");
 check(xc_library_catalog_count(xc_remote_catalog_library(xc_remote_install_result_next_catalog(owned)))==1&&std::string(xc_remote_install_result_archive_sha256(owned)).size()==64,"Remote result retains next-install catalog and archive digest");xc_transfer_progress progress{};check(xc_transfer_control_progress(control,&progress)==XC_OK&&progress.phase==XC_TRANSFER_COMPLETE&&progress.downloaded>0&&!xc_transfer_control_cancel(control),"Progress survives worker and late cancellation loses");xc_transfer_control_release(control);
 check(std::string(xc_remote_catalog_destination_revision(xc_remote_install_result_next_catalog(owned)))==done.destination_revision,"Next catalog uses the committed destination fingerprint");
 check(xc_worker_open((root/"installed").u8string().c_str(),&worker)==XC_OK,"Fresh worker for next catalog choice");
 check(xc_worker_install_remote_module(worker,xc_remote_install_result_next_catalog(owned),"Added",1,&o,&r)==XC_OK,"Reinstall from the updated borrowed catalog");xc_remote_install_result_release(owned);
 check(wait(r)==XC_OK,"Accepted next-install catalog copied before original release");auto next=xc_request_take_remote_install_result(r);xc_request_release(r);xc_worker_close(worker);check(xc_local_install_result_get(xc_remote_install_result_installed(next),&done)==XC_OK&&done.replaced,"Explicit same-version remote replacement");xc_remote_install_result_release(next);
 check(fs::is_regular_file(root/"installed/data/Added"/fs::u8path("original-Ω.txt")),"Valid Unicode archive filenames survive installation");
 {Reader reader((root/"installed").u8string());check(reader.modules().size()==2,"Remote install persists after fresh SDK open");}
 for(const auto &entry:fs::directory_iterator(root))check(entry.path().filename().string().rfind(".xiphos-transfer-",0)!=0,"Transfer workspaces always cleaned");
 std::cout<<"HTTPS certificate/redirect/size/cancel gates, bounded hostile archives, metadata verification, staged install and C ownership passed\n";return 0;
 }catch(const std::exception &e){watching=false;std::cerr<<e.what()<<'\n';return 1;}}
