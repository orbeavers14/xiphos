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
 for(const auto *id:{"IndexDictionary","IndexBook","IndexLatinDictionary"}){
  auto directory=root/"data"/id;fs::create_directories(directory);auto prefix=(directory/"entries").string();
  const bool latin=std::string(id)=="IndexLatinDictionary",dictionary=std::string(id)!="IndexBook";
  if(dictionary){check(sword::RawLD4::createModule(prefix.c_str())==0,"Create dictionary");sword::RawLD4 writer(prefix.c_str());writer.setKey("Alpha.Key");writer.setEntry(latin?"Original caf\xe9 alpha token":"Original alpha token");writer.setKey("Beta.Key");writer.setEntry(latin?"Original caf\xe9 beta token":"Original beta token");}
  else {check(sword::RawGenBook::createModule(prefix.c_str())==0,"Create book");sword::RawGenBook writer(prefix.c_str());writer.setKey("/Part/Alpha");writer.setEntry("Original alpha token");writer.setKey("/Part/Beta");writer.setEntry("Original beta token");}
  std::ofstream conf(root/"mods.d"/(std::string(id)+".conf"));conf<<'['<<id<<"]\nDataPath=./data/"<<id<<"/entries\nModDrv="<<(dictionary?"RawLD4":"RawGenBook")<<"\nSourceType=Plain\nEncoding="<<(latin?"Latin-1":"UTF-8")<<"\nLang=en\n";
 }
 {
  EngineManager manager(root.string().c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);
  for(const auto *id:{"IndexDictionary","IndexBook","IndexLatinDictionary"}){auto *module=manager.getModule(id);check(module,"Fixture source installed");module->deleteSearchFramework();module->createSearchFramework();
   if(!supports_indexed_search(*module)){std::cout<<(required?"FAIL: ":"SKIP: ")<<"SDK did not create usable keyed index for "<<id<<"\n";return required?1:77;}}
 }
 Reader reader(root.string());StudySession session(reader);
 for(const auto *id:{"IndexDictionary","IndexBook"}){
  auto raw=reader.search_keyed_module({id,"alpha beta",SearchMode::indexed});
  check(raw.engine_type==-4&&!raw.index_fallback&&raw.executed&&raw.references.size()==2,"Actual keyed raw index syntax uses OR");
  SearchQuery interval{id,"alpha beta",SearchMode::indexed};const auto endpoint=std::string(id)=="IndexDictionary"?"BETA.KEY":"/Part/Beta";interval.keyed_interval=KeyedSearchInterval{endpoint,endpoint};
  auto bounded=reader.search_keyed_module(interval);check(bounded.engine_type==-4&&bounded.total_results==1&&bounded.references==std::vector<std::string>{endpoint},"Actual index cannot escape exact inclusive interval");
  auto words=reader.search_keyed_module({id,"alpha beta",SearchMode::words});
  check(words.engine_type==-4&&!words.index_fallback&&words.references.empty(),"Actual keyed indexed Words uses AND");
  auto seed=session.search_mixed_batch({{id,"alpha",SearchMode::indexed}});check(!seed.failures&&seed.copied_results==1,"Indexed mixed generation retains exact key");
  SearchQuery narrowed{id,"Original",SearchMode::indexed};narrowed.within_last_results=true;
  auto last=session.search_mixed_batch({narrowed});check(!last.failures&&last.copied_results==1&&last.groups[0].result->references==seed.groups[0].result->references,"Actual keyed indexed last scope preserves opaque source key");
 }
 auto latin=reader.search_keyed_module({"IndexLatinDictionary","café",SearchMode::indexed});check(latin.executed&&latin.engine_type==-4&&!latin.index_fallback&&latin.references.size()==2,"Actual keyed index resolves UTF-8 queries against original Latin-1 content");
 for(const auto &key:latin.references)check(reader.preview_keyed_search("IndexLatinDictionary",key,latin.key_kind,4096).html.find("café")!=std::string::npos,"Actual indexed Latin-1 hits preview as decoded UTF-8");
 std::cout<<"Actual dictionary/book indexed execution, raw OR, Words AND and last scope passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
