// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <thread>
#include <chrono>
#include <rawtext.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
namespace fs=std::filesystem;
static void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>static void rejects(F f){try{f();}catch(const std::exception &){return;}throw std::runtime_error("Expected rejection");}
static void fixture(const fs::path &root,const std::string &id,const std::string &version){
 fs::create_directories(root/"mods.d");auto data=root/"data"/id;fs::create_directories(data);
 check(sword::RawText::createModule(data.u8string().c_str())==0,"Create original installation fixture");
 {sword::RawText writer(data.u8string().c_str());writer.setKey("John3:16");writer.setEntry(("Original installation prose "+id+" version "+version).c_str());}
 std::ofstream conf(root/"mods.d"/(id+".conf"));conf<<"# Original fixture notice\n["<<id<<"]\nDataPath=./data/"<<id<<"/\nModDrv=RawText\nEncoding=UTF-8\nSourceType=Plain\nLang=en\nDescription=Original installation fixture\nVersion="<<version<<"\nCustomNotice=Preserve this unknown field\n";
 std::ofstream asset(data/"original-asset.txt");asset<<"Original module-owned asset "<<version;
}

int main(int argc,char **argv){try{
 check(argc==2,"Fixture path required");auto root=fs::u8path(argv[1]);check(root.filename()=="maintenance-fixture","Named fixture only");fs::remove_all(root);auto library=root/"library";fixture(library,"Base","1");fixture(library,"Unused","1");fixture(library,"Locked","1");{std::ofstream out(library/"mods.d/Locked.conf",std::ios::app);out<<"CipherKey=\n";}{std::ofstream out(library/"unrelated.txt");out<<"Original unrelated data";}
 StudyWorker worker(library.u8string());auto tab=worker.create_tab().get();worker.visit(tab,"Base","John3:16").get();auto p=worker.preview_module_maintenance("Base").get();rejects([&]{worker.maintain_module({"Base",p.revision,ModuleMaintenanceAction::remove,{}}).get();});check(fs::exists(library/"mods.d/Base.conf"),"In-use module retained");
 auto unused=worker.preview_module_maintenance("Unused").get();{std::ofstream out(library/"external.txt");out<<"External change";}rejects([&]{worker.maintain_module({"Unused",unused.revision,ModuleMaintenanceAction::remove,{}}).get();});fs::remove(library/"external.txt");
 p=worker.preview_module_maintenance("Locked").get();check(p.cipher_declared,"Cipher declaration detected");auto result=worker.maintain_module({"Locked",p.revision,ModuleMaintenanceAction::set_cipher_key,"Original fixture key"}).get();check(result.action==ModuleMaintenanceAction::set_cipher_key&&result.revision!=p.revision,"Cipher configuration persisted");{std::ifstream in(library/"mods.d/Locked.conf");std::string bytes{std::istreambuf_iterator<char>(in),{}};check(bytes.find("CipherKey=Original fixture key")!=bytes.npos&&bytes.find("CustomNotice=Preserve this unknown field")!=bytes.npos,"Unknown configuration retained");}
 p=worker.preview_module_maintenance("Base").get();rejects([&]{worker.maintain_module({"Base",p.revision,ModuleMaintenanceAction::set_cipher_key,"key"}).get();});rejects([&]{worker.maintain_module({"Locked",p.revision,ModuleMaintenanceAction::set_cipher_key,"key\nInjected=1"}).get();});
 p=worker.preview_module_maintenance("Unused").get();auto before=worker.history(tab).get();auto done=worker.maintain_module({"Unused",p.revision,ModuleMaintenanceAction::remove,{}}).get();check(!fs::exists(library/"data/Unused")&&!fs::exists(library/"mods.d/Unused.conf")&&fs::exists(library/"unrelated.txt"),"Only selected module removed");check(worker.modules().get().size()==2&&worker.history(tab).get().entries.size()==before.entries.size()&&worker.read_entry(tab,"Base","John3:16").get().html.find("Original installation prose")!=std::string::npos,"SDK refreshed, sibling tab retained");
 p=worker.preview_module_maintenance("Base").get();worker.maintain_module({"Base",p.revision,ModuleMaintenanceAction::delete_index,{}}).get();p=worker.preview_module_maintenance("Base").get();check(!p.indexed,"Index delete/noop verified");
 bool built=false;try{auto index=worker.maintain_module({"Base",p.revision,ModuleMaintenanceAction::build_index,{}}).get();check(index.indexed,"Built index usable");built=true;}catch(const std::exception &){auto after=worker.preview_module_maintenance("Base").get();check(after.revision==p.revision&&!after.indexed,"Unsupported/failed index creation leaves library unchanged");}
 if(built){p=worker.preview_module_maintenance("Base").get();worker.maintain_module({"Base",p.revision,ModuleMaintenanceAction::delete_index,{}}).get();check(!worker.preview_module_maintenance("Base").get().indexed,"Created index removed");}
 // Failed refresh rolls the complete filesystem transaction back.
 {Reader reader(library.u8string());auto preview=reader.preview_module_maintenance("Locked");rejects([&]{reader.maintain_module({"Locked",preview.revision,ModuleMaintenanceAction::remove,{}},[]{throw std::runtime_error("Injected refresh failure");});});check(reader.preview_module_maintenance("Locked").revision==preview.revision,"Refresh rollback restores original root");}
 {xc_worker *cworker=nullptr;check(xc_worker_open(library.u8string().c_str(),&cworker)==XC_OK,"Maintenance C worker");xc_request *request=nullptr;
 auto wait=[](xc_request *r){auto until=std::chrono::steady_clock::now()+std::chrono::seconds(30);xc_status status;while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<until,"C maintenance deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;};
 check(xc_worker_preview_module_maintenance(cworker,"Locked",&request)==XC_OK&&wait(request)==XC_OK,"C maintenance preview");auto preview=xc_request_take_module_maintenance_preview(request);xc_request_release(request);xc_module_maintenance_info info{};check(xc_module_maintenance_preview_get(preview,&info)==XC_OK,"C preview owns metadata");auto revision=std::string(info.revision);xc_module_maintenance_preview_release(preview);
 check(xc_worker_maintain_module(cworker,"Locked",revision.c_str(),1,"Replacement original key",&request)==XC_OK&&wait(request)==XC_OK,"C maintenance write");auto result=xc_request_take_module_maintenance_result(request);check(result&&!xc_request_take_module_maintenance_result(request),"Maintenance take once");xc_request_release(request);xc_worker_close(cworker);xc_module_maintenance_done info_done{};check(xc_module_maintenance_result_get(result,&info_done)==XC_OK&&info_done.action==1&&std::string(info_done.module)=="Locked","C result survives request/worker teardown");xc_module_maintenance_result_release(result);}
 check(std::distance(fs::directory_iterator(root),fs::directory_iterator{})==1,"No staging trees remain");worker.close_tab(tab).get();std::cout<<"Maintenance checks passed; real index creation "<<(built?"available":"unavailable on selected SDK")<<'\n';return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
