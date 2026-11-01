// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/library.hpp"
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <rawtext.h>
#include <algorithm>
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
static thread_local int fail_after=-1;
static thread_local std::size_t allocations=0;
void *operator new(std::size_t n){if(watching){++allocations;if(fail_after==0)throw std::bad_alloc();if(fail_after>0)--fail_after;}if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void *operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void *p) noexcept{std::free(p);}void operator delete[](void *p) noexcept{std::free(p);}
void operator delete(void *p,std::size_t) noexcept{std::free(p);}void operator delete[](void *p,std::size_t) noexcept{std::free(p);}
static void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F> static void rejects(F f){try{f();}catch(const std::exception &){return;}throw std::runtime_error("Expected rejection");}
static xc_status wait(xc_request *r){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(30);xc_status status;while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Catalog deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
// A non-SWORD, original-prose catalog provider exercises the SAME identities,
// permission gates and install decisions. It has no SDK or parallel study engine.
class OriginalProvider final:public LibraryProvider {
public:
 LibraryCatalog catalog() override {
  LibraryItem item;item.identity={"fixture.publisher","original-2026"};item.title="Original prose edition";
  item.translation="Original prose";item.edition="2026 study edition";item.language="en";item.version="1.0";
  item.attribution="Original test prose by the project";item.license="Fixture use only";
  item.access=LibraryAccess::available;item.delivery=LibraryDelivery::provider_package;
  item.capabilities=library_online_read|library_offline_package;
  return {"fixture.catalog",{item}};
 }
};
static void decisions(){
 OriginalProvider provider;auto source=provider.catalog();LibraryCatalog installed{"library.installed",{}};const auto id=source.items.front().identity;
 auto decision=[&]{return plan_library_install(source,installed,id).decision;};
 check(decision()==LibraryInstallDecision::permission_unknown,"Download rights default unknown");
 auto &item=source.items.front();item.download=LibraryPermission::allowed;
 check(decision()==LibraryInstallDecision::permission_unknown,"Download grant does not imply full offline storage");
 item.offline_storage=LibraryPermission::denied;check(decision()==LibraryInstallDecision::permission_denied,"Explicit offline denial wins");
 item.offline_storage=LibraryPermission::allowed;check(decision()==LibraryInstallDecision::install,"Granted provider package installation plan");
 check(item.text_export==LibraryPermission::unknown&&!(item.capabilities&library_text_export),"Download grant does not grant exports");
 item.access=LibraryAccess::sign_in_required;check(decision()==LibraryInstallDecision::sign_in_required,"Authentication gate");
 item.access=LibraryAccess::license_required;check(decision()==LibraryInstallDecision::license_required,"Entitlement gate");
 item.access=LibraryAccess::unavailable;check(decision()==LibraryInstallDecision::unavailable,"Unavailable provider");
 item.access=LibraryAccess::available;item.delivery=LibraryDelivery::none;check(decision()==LibraryInstallDecision::unsupported,"Online access does not imply download");
 item.delivery=LibraryDelivery::provider_package;
 auto existing=item;existing.installed=true;existing.identity.provider="sword";installed.items={existing};
 check(decision()==LibraryInstallDecision::install,"Different provider never overwrites same item ID");
 installed.items.front().identity=id;check(decision()==LibraryInstallDecision::already_installed,"Exact nonempty version equality");
 installed.items.front().version="1.10";item.version="1.2";
 check(decision()==LibraryInstallDecision::replace_confirmation_required,"Opaque versions never guessed to be newer");
 item.version.clear();check(decision()==LibraryInstallDecision::replace_confirmation_required,"Unknown version requires replacement decision");
 auto revision=library_catalog_revision(source);item.license+=" Changed notice";check(library_catalog_revision(source)!=revision,"License change invalidates catalog metadata revision");
 auto duplicate=source;duplicate.items.push_back(item);rejects([&]{validate_library_catalog(duplicate);});
 auto invalid=source;invalid.items.front().capabilities=512;rejects([&]{validate_library_catalog(invalid);});
 invalid=source;invalid.items.front().identity.provider="fixture/publisher";rejects([&]{validate_library_catalog(invalid);});
 invalid=source;invalid.items.front().title=std::string("bad\0text",8);rejects([&]{validate_library_catalog(invalid);});
 invalid=source;invalid.items.front().license=std::string(32769,'x');rejects([&]{validate_library_catalog(invalid);});
 invalid=source;invalid.items.resize(4097);rejects([&]{validate_library_catalog(invalid);});
 auto two=source;auto second=item;second.identity.item="another";two.items.push_back(second);revision=library_catalog_revision(two);std::reverse(two.items.begin(),two.items.end());check(library_catalog_revision(two)==revision,"Revision independent of provider enumeration order");
 installed.items.front().installed=false;rejects([&]{(void)plan_library_install(source,installed,id);});
}
static void fixture(const std::filesystem::path &root,const std::string &version){
 std::filesystem::create_directories(root/"mods.d");auto data=root/"data/Original";std::filesystem::create_directories(data);
 check(sword::RawText::createModule(data.u8string().c_str())==0,"Create original library fixture");
 {sword::RawText writer(data.u8string().c_str());writer.setKey("John3:16");writer.setEntry("Original library fixture prose.");}
 std::ofstream conf(root/"mods.d/original.conf");conf<<"[Original]\nDataPath=./data/Original/\nModDrv=RawText\nEncoding=UTF-8\nSourceType=Plain\nLang=en\nVersion="<<version<<"\nVersification=KJV\nDescription=Original library prose\nCopyright=Original project fixture\nDistributionLicense=Fixture use only\n";
}
int main(int argc,char **argv){try{
 decisions();check(argc==2,"Disposable library fixture path required");auto root=std::filesystem::u8path(argv[1]);check(root.filename()=="library-catalog-fixture","Named disposable root only");std::filesystem::remove_all(root);
 auto destination=root/"installed",source=root/"source";fixture(destination,"1.0");fixture(source,"2.0");
 {StudyWorker worker(destination.u8string());auto tab=worker.create_tab().get();(void)worker.visit(tab,"Original","John3:16").get();auto before=worker.history(tab).get();
  auto installed=worker.library_catalog().get();auto local=worker.local_library_catalog(source.u8string()).get();
  check(installed.items.size()==1&&local.items.size()==1,"Explicit source has no standard/home modules");
  check(installed.items.front().installed&&(installed.items.front().capabilities&library_offline_read)&&installed.items.front().download==LibraryPermission::unknown,"Installed availability is not a download grant");
  check(!local.items.front().installed&&local.items.front().identity.provider=="sword"&&local.items.front().delivery==LibraryDelivery::local_sword,"SWORD adapter uses neutral DTOs");
  check(local.items.front().attribution=="Original project fixture"&&local.items.front().license=="Fixture use only"&&local.items.front().translation.empty()&&local.items.front().edition.empty(),"Preserve notices without inventing edition identity");
  check(plan_library_install(local,installed,{"sword","Original"}).decision==LibraryInstallDecision::replace_confirmation_required,"Local SWORD replacement plan");
  check(worker.history(tab).get().entries.size()==before.entries.size(),"Catalog inspection leaves history alone");
  check(worker.read_entry(tab,"Original","John3:16").get().html.find("Original library fixture prose")!=std::string::npos,"Catalog inspection leaves live reading usable");
  rejects([&]{(void)worker.local_library_catalog(root.u8string()).get();});
  auto invalid=worker.local_library_catalog("");check(invalid.wait_for(std::chrono::seconds(0))==std::future_status::ready,"Invalid root rejected before FIFO");rejects([&]{(void)invalid.get();});
  {std::ofstream config(source/"mods.d/original.conf",std::ios::app);config<<"CipherKey=\n";}
  local=worker.local_library_catalog(source.u8string()).get();check(local.items.front().access==LibraryAccess::locked&&!(local.items.front().capabilities&library_offline_read),"Locked content is not advertised as readable");
  // Installing encrypted content may precede unlocking; no key is copied into
  // the neutral catalog and no automatic entitlement is created.
  check(plan_library_install(local,installed,{"sword","Original"}).decision==LibraryInstallDecision::replace_confirmation_required,"Locked local package can be planned independently of reading");
 }
 xc_worker *worker=nullptr;check(xc_worker_open(destination.u8string().c_str(),&worker)==XC_OK,"C worker");xc_request *r=nullptr;
 for(int fail:{0,1}){watching=true;fail_after=fail;auto status=xc_worker_library_catalog(worker,&r);watching=false;fail_after=-1;check(status!=XC_OK&&!r,"Catalog observer/envelope allocation precedes acceptance");}
 check(xc_worker_library_catalog(worker,&r)==XC_OK,"C installed catalog");watching=true;allocations=0;fail_after=0;auto status=wait(r);watching=false;fail_after=-1;check(status==XC_OK&&allocations==0,"Catalog polling moves preallocated owned result");auto installed=xc_request_take_library_catalog(r);check(installed&&!xc_request_take_library_catalog(r),"Take catalog once");xc_request_release(r);
 auto copied=source.u8string();check(xc_worker_local_library_catalog(worker,copied.c_str(),&r)==XC_OK,"C local catalog accepted");copied.assign("unrelated after acceptance");check(wait(r)==XC_OK,"C local source path copied");auto local=xc_request_take_library_catalog(r);xc_request_release(r);xc_worker_close(worker);
 xc_library_item row{};check(xc_library_catalog_count(local)==1&&xc_library_catalog_item(local,0,&row)==XC_OK&&std::string(row.provider)=="sword"&&std::string(row.version)=="2.0","Catalog owns strings after worker teardown");
 check(xc_library_catalog_item(local,99,&row)==XC_INVALID_ARGUMENT&&!row.item,"Invalid row clears output");
 xc_library_install_plan *plan=nullptr;check(xc_library_plan_install(local,installed,"sword","Original",&plan)==XC_OK,"C pure install decision");xc_library_catalog_release(local);xc_library_catalog_release(installed);
 xc_library_plan value{};check(xc_library_install_plan_get(plan,&value)==XC_OK&&value.decision==XC_LIBRARY_REPLACE_CONFIRMATION_REQUIRED&&std::string(value.source_version)=="2.0"&&std::string(value.source_revision).size()==64,"Plan owns metadata after catalogs released");xc_library_install_plan_release(plan);
 std::cout<<"Provider-neutral library identities, explicit grants, local SWORD catalogs and owned installation decisions passed\n";return 0;
 }catch(const std::exception &e){watching=false;std::cerr<<e.what()<<'\n';return 1;}}
