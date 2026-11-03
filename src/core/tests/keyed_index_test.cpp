// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/search_policy.hpp"
#include <rawld4.h>
#include <rawgenbook.h>
#include <markupfiltmgr.h>
#include <swmodule.h>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace xiphos::core;
namespace fs=std::filesystem;
static void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char **argv)try{
 check(argc==2||argc==3,"Disposable fixture root and optional required index gate");
 const bool required=argc==3&&std::string(argv[2])=="--require-index";
 fs::path root=argv[1];check(root.filename()=="keyed-index-fixture","Named disposable root only");fs::remove_all(root);fs::create_directories(root/"mods.d");
 for(const auto *id:{"IndexDictionary","IndexBook"}){
  auto directory=root/"data"/id;fs::create_directories(directory);auto prefix=(directory/"entries").string();
  const bool dictionary=std::string(id)=="IndexDictionary";
  if(dictionary){check(sword::RawLD4::createModule(prefix.c_str())==0,"Create dictionary");sword::RawLD4 writer(prefix.c_str());writer.setKey("Alpha.Key");writer.setEntry("Original alpha token");writer.setKey("Beta.Key");writer.setEntry("Original beta token");}
  else {check(sword::RawGenBook::createModule(prefix.c_str())==0,"Create book");sword::RawGenBook writer(prefix.c_str());writer.setKey("/Part/Alpha");writer.setEntry("Original alpha token");writer.setKey("/Part/Beta");writer.setEntry("Original beta token");}
  std::ofstream conf(root/"mods.d"/(std::string(id)+".conf"));conf<<'['<<id<<"]\nDataPath=./data/"<<id<<"/entries\nModDrv="<<(dictionary?"RawLD4":"RawGenBook")<<"\nSourceType=Plain\nEncoding=UTF-8\nLang=en\n";
 }
 {
  EngineManager manager(root.string().c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);
  for(const auto *id:{"IndexDictionary","IndexBook"}){auto *module=manager.getModule(id);check(module,"Fixture source installed");module->deleteSearchFramework();module->createSearchFramework();
   if(!supports_indexed_search(*module)){std::cout<<(required?"FAIL: ":"SKIP: ")<<"SDK did not create usable keyed index for "<<id<<"\n";return required?1:77;}}
 }
 Reader reader(root.string());StudySession session(reader);
 for(const auto *id:{"IndexDictionary","IndexBook"}){
  auto raw=reader.search_keyed_module({id,"alpha beta",SearchMode::indexed});
  check(raw.engine_type==-4&&!raw.index_fallback&&raw.executed&&raw.references.size()==2,"Actual keyed raw index syntax uses OR");
  auto words=reader.search_keyed_module({id,"alpha beta",SearchMode::words});
  check(words.engine_type==-4&&!words.index_fallback&&words.references.empty(),"Actual keyed indexed Words uses AND");
  auto seed=session.search_mixed_batch({{id,"alpha",SearchMode::indexed}});check(!seed.failures&&seed.copied_results==1,"Indexed mixed generation retains exact key");
  SearchQuery narrowed{id,"Original",SearchMode::indexed};narrowed.within_last_results=true;
  auto last=session.search_mixed_batch({narrowed});check(!last.failures&&last.copied_results==1&&last.groups[0].result->references==seed.groups[0].result->references,"Actual keyed indexed last scope preserves opaque source key");
 }
 std::cout<<"Actual dictionary/book indexed execution, raw OR, Words AND and last scope passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
