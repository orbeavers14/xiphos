// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include <rawtext.h>
#include <swcipher.h>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace xiphos::core;
namespace fs=std::filesystem;
static const std::string prose="Original encrypted maintenance prose Ω.";
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::string bytes(const fs::path &p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
bool readable(StudyWorker &worker,StudyTab tab){try{return worker.read_entry(tab,"LockedOriginal","John3:16").get().html.find("Original encrypted maintenance prose")!=std::string::npos;}catch(const std::exception &){return false;}}
int main(int argc,char **argv){try{
 check(argc==2,"Fixture directory required");auto root=fs::u8path(argv[1]);check(root.filename()=="encrypted-maintenance-fixture","Named disposable root only");fs::remove_all(root);fs::create_directories(root/"mods.d");auto data=root/"data/LockedOriginal";fs::create_directories(data);

 std::string key="Original-fixture-unlock-key";
 check(sword::RawText::createModule(data.u8string().c_str())==0,"Create original encrypted fixture");
 {sword::SWCipher cipher(reinterpret_cast<unsigned char *>(key.data()));cipher.setUncipheredBuf(prose.c_str(),static_cast<unsigned long>(prose.size()));unsigned long length=0;const auto *encrypted=cipher.getCipheredBuf(&length);check(encrypted&&length,"SDK encrypts original prose");sword::RawText writer(data.u8string().c_str());writer.setKey("John3:16");writer.setEntry(encrypted,static_cast<long>(length));}
 {std::ofstream out(root/"mods.d/LockedOriginal.conf");out<<"[LockedOriginal]\nDataPath=./data/LockedOriginal/\nModDrv=RawText\nEncoding=UTF-8\nSourceType=Plain\nDescription=Original encrypted fixture\nCipherKey=\nCustomNotice=Keep original metadata\n";}

 auto original_data=bytes(data/"nt");check(original_data.find("Original encrypted maintenance prose")==std::string::npos,"Stored data is encrypted, not merely a CipherKey declaration");
 {StudyWorker worker(root.u8string());auto tab=worker.create_tab().get();check(!readable(worker,tab),"Blank key cannot read the fixture");
 auto preview=worker.preview_module_maintenance("LockedOriginal").get();(void)worker.maintain_module({"LockedOriginal",preview.revision,ModuleMaintenanceAction::set_cipher_key,"Wrong-original-key"}).get();check(!readable(worker,tab),"Wrong key cannot reveal original content");
 preview=worker.preview_module_maintenance("LockedOriginal").get();(void)worker.maintain_module({"LockedOriginal",preview.revision,ModuleMaintenanceAction::set_cipher_key,key}).get();check(readable(worker,tab),"Correct key unlocks actual encrypted content after SDK refresh");
 check(bytes(data/"nt")==original_data,"Key changes never rewrite encrypted module data");
 auto opened=worker.visit(tab,"LockedOriginal","John3:16").get();(void)opened;preview=worker.preview_module_maintenance("LockedOriginal").get();(void)worker.maintain_module({"LockedOriginal",preview.revision,ModuleMaintenanceAction::set_cipher_key,""}).get();check(!readable(worker,tab),"Clearing key invalidates readable content and cached state");
 preview=worker.preview_module_maintenance("LockedOriginal").get();(void)worker.maintain_module({"LockedOriginal",preview.revision,ModuleMaintenanceAction::set_cipher_key,key}).get();check(readable(worker,tab),"Correct key can be reapplied without changing active tab identity");}
 {StudyWorker worker(root.u8string());auto tab=worker.create_tab().get();check(readable(worker,tab),"Unlock configuration survives a fresh worker");}
 check(bytes(root/"mods.d/LockedOriginal.conf").find("CustomNotice=Keep original metadata")!=std::string::npos,"Unknown metadata retained");
 std::cout<<"Actual encrypted original-prose unlock, wrong/blank key, cache refresh and restart checks passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
