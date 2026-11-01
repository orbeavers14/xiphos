// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/repository.hpp"
#include <rawtext.h>
#include <rawgenbook.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
namespace fs=std::filesystem;
static thread_local bool watching=false;static thread_local int fail_after=-1;static thread_local std::size_t allocations=0;
void *operator new(std::size_t n){if(watching){++allocations;if(fail_after==0)throw std::bad_alloc();if(fail_after>0)--fail_after;}if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void *operator new[](std::size_t n){return ::operator new(n);}void operator delete(void *p)noexcept{std::free(p);}void operator delete[](void *p)noexcept{std::free(p);}void operator delete(void *p,std::size_t)noexcept{std::free(p);}void operator delete[](void *p,std::size_t)noexcept{std::free(p);}
static void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>static void rejects(F f){try{f();}catch(const std::exception &){return;}throw std::runtime_error("Expected rejection");}
static xc_status wait(xc_request *r){auto until=std::chrono::steady_clock::now()+std::chrono::seconds(30);xc_status s;while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<until,"Install deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return s;}
static std::string bytes(const fs::path &p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
static void fixture(const fs::path &root,const std::string &id,const std::string &version){
 fs::create_directories(root/"mods.d");auto data=root/"data"/id;fs::create_directories(data);
 check(sword::RawText::createModule(data.u8string().c_str())==0,"Create original installation fixture");
 {sword::RawText writer(data.u8string().c_str());writer.setKey("John3:16");writer.setEntry(("Original installation prose "+id+" version "+version).c_str());}
 std::ofstream conf(root/"mods.d"/(id+".conf"));conf<<"# Original fixture notice\n["<<id<<"]\nDataPath=./data/"<<id<<"/\nModDrv=RawText\nEncoding=UTF-8\nSourceType=Plain\nLang=en\nDescription=Original installation fixture\nVersion="<<version<<"\nCustomNotice=Preserve this unknown field\n";
 std::ofstream asset(data/"original-asset.txt");asset<<"Original module-owned asset "<<version;
}
static LocalInstallRequest request(const fs::path &source,const LocalInstallPreview &p,bool replace=false){return {source.u8string(),p.module,p.source_revision,p.destination_revision,replace};}
static void endpoints(){
 HttpsRepository repo{"crosswire","www.crosswire.org","/ftpmirror/pub/sword/raw/","/ftpmirror/pub/sword/packages/rawzip/"};
 check(repository_catalog_url(repo)=="https://www.crosswire.org/ftpmirror/pub/sword/raw/mods.d.tar.gz","HTTPS catalog endpoint");
 check(repository_module_url(repo,"ASV")=="https://www.crosswire.org/ftpmirror/pub/sword/packages/rawzip/ASV.zip","Package directory is independent of raw catalog");
 check(repository_module_url(repo,"Original Ω %20").find("Original%20%CE%A9%20%2520.zip")!=std::string::npos,"URL bytes encoded once");
 for(const auto *name:{"../ASV","/ASV","a\\b",".."})rejects([&]{repository_module_url(repo,name);});
 auto invalid=repo;invalid.host="https://www.crosswire.org";rejects([&]{repository_catalog_url(invalid);});
 invalid=repo;invalid.host="user@host.invalid";rejects([&]{repository_catalog_url(invalid);});
 invalid=repo;invalid.package_directory.clear();rejects([&]{repository_module_url(invalid,"ASV");});
 invalid=repo;invalid.catalog_directory="/raw/../outside";rejects([&]{repository_catalog_url(invalid);});
}
int main(int argc,char **argv){try{
 endpoints();check(argc==2,"Disposable installation root required");auto root=fs::u8path(argv[1]);check(root.filename()=="local-install-fixture","Named disposable installation root only");fs::remove_all(root);
 auto destination=root/"installed",source=root/"source";fixture(destination,"Base","1");fixture(source,"Base","2");fixture(source,"Added","1");
 {auto data=source/"data/Book";fs::create_directories(data);const auto prefix=(data/"entries").u8string();check(sword::RawGenBook::createModule(prefix.c_str())==0,"Create prefix-layout book fixture");{sword::RawGenBook writer(prefix.c_str());writer.setKey("/Original");writer.setEntry("Original installed general-book prose");}std::ofstream out(source/"mods.d/Book.conf");out<<"[Book]\nDataPath=./data/Book/entries\nModDrv=RawGenBook\nEncoding=UTF-8\nSourceType=ThML\nDescription=Original book fixture\nVersion=1\n";}
 {std::ofstream out(destination/"unrelated.txt");out<<"Original unrelated library data";}
 {std::ofstream out(destination/"mods.d/modules-conf.cache");out<<"# Stale derived cache must not hide real modules\n";}
 {StudyWorker worker(destination.u8string());auto tab=worker.create_tab().get();(void)worker.visit(tab,"Base","John3:16").get();auto history=worker.history(tab).get();SearchQuery q;q.module="Base";q.expression="Original";auto serial=worker.search_module(tab,q).get().generation.id;
  auto preview=worker.prepare_local_install(source.u8string(),"Added").get();check(preview.decision==LibraryInstallDecision::install,"New module preview");check(!fs::exists(source/"mods.d/modules-conf.cache"),"Preview does not write a source cache");auto done=worker.install_local_module(request(source,preview)).get();
  check(!done.replaced&&worker.modules().get().size()==2&&worker.read_entry(tab,"Added","John3:16").get().html.find("Added version 1")!=std::string::npos,"Live SDK discovers installed module");
  check(!fs::exists(destination/"mods.d/modules-conf.cache"),"Stale derived destination cache invalidates at commit");
  check(bytes(destination/"mods.d/Added.conf")==bytes(source/"mods.d/Added.conf")&&bytes(destination/"data/Added/original-asset.txt")==bytes(source/"data/Added/original-asset.txt"),"Exact configuration/notices/assets copied");
  check(bytes(destination/"unrelated.txt")=="Original unrelated library data"&&worker.history(tab).get().entries.size()==history.entries.size(),"Unrelated data/history retained");
  check(worker.search_generation(tab).get().id==0&&worker.search_module(tab,q).get().generation.id>serial,"SDK search caches invalidate without ID reuse");
  preview=worker.prepare_local_install(source.u8string(),"Book").get();(void)worker.install_local_module(request(source,preview)).get();auto bookTab=worker.create_tab().get();auto book=worker.select_book_document(bookTab,{"Book",std::string("/Original")}).get();check(book.html.find("Original installed general-book prose")!=std::string::npos,"Prefix-layout general book installs and renders");
  preview=worker.prepare_local_install(source.u8string(),"Base").get();rejects([&]{(void)worker.install_local_module(request(source,preview)).get();});
  done=worker.install_local_module(request(source,preview,true)).get();check(done.replaced&&worker.read_entry(tab,"Base","John3:16").get().html.find("Base version 2")!=std::string::npos,"Explicit replacement reloads selected module");
  check(worker.book_document_selection(bookTab).get()->html==book.html,"Sibling book tab cache survives another module installation");
  preview=worker.prepare_local_install(source.u8string(),"Base").get();{std::ofstream out(source/"data/Base/changed.txt");out<<"Changed source";}
  rejects([&]{(void)worker.install_local_module(request(source,preview,true)).get();});fs::remove(source/"data/Base/changed.txt");
  {std::ofstream out(destination/"unrelated.txt");out<<"External destination change";}
  rejects([&]{(void)worker.install_local_module(request(source,preview,true)).get();});check(bytes(destination/"unrelated.txt")=="External destination change","Stale install never overwrites external destination changes");
  rejects([&]{(void)worker.prepare_local_install(destination.u8string(),"Base").get();});
  std::error_code ec;fs::create_symlink("original-asset.txt",source/"data/Base/alias",ec);if(!ec){rejects([&]{(void)worker.prepare_local_install(source.u8string(),"Base").get();});fs::remove(source/"data/Base/alias");}
 }
 // Reject traversal before constructing module drivers; no file escapes staging.
 {auto unsafe=root/"unsafe";fixture(unsafe,"Unsafe","1");auto config=bytes(unsafe/"mods.d/Unsafe.conf");auto at=config.find("./data/Unsafe/");config.replace(at,14,"../../outside/");{std::ofstream out(unsafe/"mods.d/Unsafe.conf");out<<config;}Reader reader(destination.u8string());rejects([&]{(void)reader.prepare_local_install(unsafe.u8string(),"Unsafe");});check(!fs::exists(root/"outside"),"Rejected module creates no escaped data");}
 // Post-install reload/cache failure must retain the original live Reader and disk.
 fixture(source,"Base","3");{Reader reader(destination.u8string());auto p=reader.prepare_local_install(source.u8string(),"Base");rejects([&]{reader.install_local_module(request(source,p,true),[]{throw std::runtime_error("Injected refresh failure");});});check(reader.prepare_local_install(source.u8string(),"Base").destination_revision==p.destination_revision,"Failed refresh restores full data/config root");}
 fixture(source,"Third","1");xc_worker *worker=nullptr;check(xc_worker_open(destination.u8string().c_str(),&worker)==XC_OK,"C installer worker");xc_request *r=nullptr;
 check(xc_worker_prepare_local_install(worker,source.u8string().c_str(),"Third",&r)==XC_OK&&wait(r)==XC_OK,"C prepare installation");auto preview=xc_request_take_local_install_preview(r);xc_request_release(r);xc_local_install_info info{};check(xc_local_install_preview_get(preview,&info)==XC_OK,"C preview metadata");const std::string from=info.source_revision,to=info.destination_revision;
 const auto source_text=source.u8string();
 for(int fail:{0,1}){watching=true;fail_after=fail;auto s=xc_worker_install_local_module(worker,source_text.c_str(),"Third",from.c_str(),to.c_str(),0,&r);watching=false;fail_after=-1;check(s!=XC_OK&&!r,"Observer/result allocation precedes installation acceptance");}
 check(xc_worker_install_local_module(worker,source_text.c_str(),"Third",from.c_str(),to.c_str(),0,&r)==XC_OK,"C install accepted");xc_local_install_preview_release(preview);watching=true;allocations=0;fail_after=0;auto status=wait(r);watching=false;fail_after=-1;check(status==XC_OK&&allocations==0,"Committed install polling does not allocate");auto owned=xc_request_take_local_install_result(r);check(owned&&!xc_request_take_local_install_result(r),"Take installed result once");xc_request_release(r);xc_worker_close(worker);
 xc_local_install_done done{};check(xc_local_install_result_get(owned,&done)==XC_OK&&std::string(done.module)=="Third"&&std::string(done.destination_revision).size()==64,"Installed result survives worker teardown");xc_local_install_result_release(owned);
 for(const auto &entry:fs::directory_iterator(root))check(entry.path().filename().string().rfind(".xiphos-install-",0)!=0,"No successful/rolled-back staging leftovers");
 std::cout<<"Verified staged local installation/replacement, stale rejection, SDK reload, rollback, ownership and HTTPS endpoints passed\n";return 0;
 }catch(const std::exception &e){watching=false;std::cerr<<e.what()<<'\n';return 1;}}
