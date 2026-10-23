// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/engine_manager.hpp"
#include <markupfiltmgr.h>
#include <rawtext.h>
#include <rawcom.h>
#include <rawfiles.h>
#include <rawld.h>
#include <versekey.h>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <iostream>
#include <thread>
#include <stdexcept>
using namespace sword;
using namespace xiphos::core;
constexpr auto TEXT_MODS="Biblical Texts",COMM_MODS="Commentaries",DICT_MODS="Lexicons / Dictionaries",BOOK_MODS="Generic Books";
enum {TEXT_TYPE,COMMENTARY_TYPE,PERCOM_TYPE,DICTIONARY_TYPE,BOOK_TYPE,PRAYERLIST_TYPE};
static const char *sword_locale=nullptr;
namespace oracle {
struct BackEnd {SWMgr &manager;SWModule *get_SWModule(const char *name){return manager.getModule(name);}int module_type(const char *);char *get_valid_key(const char *,const char *);};
#define FrozenBackend BackEnd
#include "legacy_module_type.inc"
#undef FrozenBackend
#include "legacy_navigation_key.inc"
}
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class E,class F>void rejects(F f){try{f();}catch(const E &){return;}throw std::runtime_error("Expected rejected navigation target");}
xc_status wait(xc_request *r){xc_status s;const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Navigation target poll deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return s;}
void fixture(const std::filesystem::path &root){
 std::filesystem::remove_all(root);std::filesystem::create_directories(root/"mods.d");
 for(const auto *id:{"Text","TextAlt","Notes","Personal","Dictionary","Daily","LongDictionaryNameX","LongDictionaryName20"}){
  const bool bible=std::string(id).find("Text")==0,alt=std::string(id)=="TextAlt",personal=std::string(id)=="Personal",dict=std::string(id)=="Dictionary"||std::string(id)=="Daily"||std::string(id).find("LongDictionary")==0;
  const auto data=root/id;std::filesystem::create_directories(data);
  const std::string canon=alt?"KJVA":"KJV",driver=dict?"RawLD":bible?"RawText":personal?"RawFiles":"RawCom";
  const auto prefix=dict?data/"entries":data;
  check((dict?RawLD::createModule(prefix.string().c_str()):bible?RawText::createModule(prefix.string().c_str(),canon.c_str()):personal?RawFiles::createModule(prefix.string().c_str()):RawCom::createModule(prefix.string().c_str()))==0,"Create original navigation fixture");
  std::unique_ptr<SWModule> writer;
  if(dict)writer.reset(new RawLD(prefix.string().c_str()));
  else if(bible)writer.reset(new RawText(prefix.string().c_str(),nullptr,nullptr,nullptr,ENC_UTF8,DIRECTION_LTR,FMT_PLAIN,"en",canon.c_str()));
  else if(personal)writer.reset(new RawFiles(prefix.string().c_str()));else writer.reset(new RawCom(prefix.string().c_str()));
  for(const auto *reference:dict?std::vector<const char *>{"alpha","alpha-beta;gamma,delta/part#tag!note?query","café 文字","literal%20key","plus+key"}:std::vector<const char *>{"Genesis1:1","John3:16","John3:17"}){writer->setKey(reference);writer->setEntry(("Original "+std::string(id)+" navigation prose.").c_str());}
  const std::string alias=dict?(std::string(id)=="Dictionary"?"D":std::string(id)=="Daily"?"DailyAlias":std::string(id)=="LongDictionaryNameX"?"L19":"L20"):bible?(alt?"TA":"T"):personal?"P":"N";
  std::ofstream config(root/"mods.d"/(std::string(id)+".conf"));
  config<<'['<<id<<"]\nDataPath=./"<<id<<(dict?"/entries":"/")<<"\nModDrv="<<driver<<"\nSourceType=Plain\nEncoding=UTF-8\nLang=en\nVersification="<<canon<<"\nDescription=Original navigation fixture\nAbbreviation="<<alias<<'\n';if(std::string(id)=="Daily")config<<"Feature=DailyDevotion\n";check(bool(config),"Write fixture catalog");
 }
}
int main(int argc,char **argv){try {
 check(argc==2,"Disposable navigation target root required");const std::string root=argv[1];fixture(std::filesystem::u8path(root));unsigned comparisons=0;PassageLink held;
 {Reader reader(root);EngineManager baseline(root.c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);oracle::BackEnd old{baseline};
  for(const auto *id:{"Text","TextAlt","Notes","Personal"})for(const auto *context:{"John3:23","Genesis1:1"})
  for(const auto *input:{"John3:16","John3:16-18","John3:16;18","16-18;20","John3:36-4:2","John3:999","Rev999:999","Tob1:3","not-a-reference"}){
   (void)reader.read(id,context);auto *module=baseline.getModule(id);module->setKey(context);
   std::unique_ptr<char,decltype(&std::free)> expected(old.get_valid_key(id,input),&std::free);
   const bool invalid=module->getKey()->getError()!=0;
   if(invalid)rejects<std::invalid_argument>([&]{(void)reader.navigation_target(id,input);});
   else {const auto actual=reader.navigation_target(id,input);check(actual.module==id&&actual.reference==expected.get(),"Cloned native target equals complete frozen navigation reference on valid SDK output");}
   ++comparisons;
  }
  (void)reader.read("Text","John3:23");const auto beforeRelative=reader.navigation_target("Text","24").reference;held=reader.navigation_target("Text","Genesis1:1-2");
  check(held.reference=="Genesis 1:1"&&reader.navigation_target("Text","24").reference==beforeRelative,"Resolving a target on a clone preserves live module position for later relative input");
  rejects<std::out_of_range>([&]{reader.navigation_target("Missing","John3:16");});rejects<std::invalid_argument>([&]{reader.navigation_target("Dictionary","John3:16");});
  for(const auto &bad:{std::string(),std::string("John3:16\0tail",13),std::string(32769,'x')})rejects<std::invalid_argument>([&]{reader.navigation_target("Text",bad);});
 }
 check(held.module=="Text"&&held.reference=="Genesis 1:1","Copied target survives Reader teardown");
 std::future<ContextualStudyLink> accepted;
 {StudyWorker worker(root);const auto tab=worker.create_tab().get();const auto generation=worker.search_module(tab,{"Text","Original"}).get().generation.id;
  (void)worker.read_passage(tab,"Text","Notes","John3:23").get();
  const auto unknown=std::get<PassageLink>(worker.follow_pane_link_at(tab,"Text","Notes","Genesis1:1","sword://Missing/John3:16-18").get());
  const auto notes=std::get<PassageLink>(worker.follow_pane_link_at(tab,"Text","Personal","John3:23","sword://N/John3:16%3B18").get());
  const auto personal=std::get<PassageLink>(worker.resolve_study_link_at(tab,"Text","John3:23","bible://P/John3:16-18").get());
  check(unknown.module=="Text"&&notes.module=="Notes"&&personal.module=="Personal"&&unknown.reference=="John 3:16"&&notes.reference==unknown.reference&&personal.reference==unknown.reference,"Unknown URI ranges use exact main; commentary aliases use their actual module and canonical target");
  const auto bible=std::get<ReferenceList>(worker.follow_pane_link_at(tab,"Text","Notes","John3:23","sword://T/John3:16-18").get());check(bible.module=="Text"&&bible.entries.size()==3,"Bible ranges retain sidebar classification");
  rejects<std::invalid_argument>([&]{(void)worker.resolve_study_link_at(tab,"Text","John3:23","sword://D/John3:16-18").get();});
  rejects<std::out_of_range>([&]{(void)worker.follow_pane_link_at(tab,"T","Notes","John3:23","sword://Missing/John3:16-18").get();});
  check(worker.search_generation(tab).get().id==generation&&worker.history(tab).get().entries.empty(),"Target resolution does not publish navigation/search changes");
  const auto visited=worker.visit_linked_passage(tab,"Text","Personal",notes).get();check(visited.module=="Text"&&visited.commentary_module=="Notes"&&visited.commentary&&visited.commentary->html.find("Original Notes")!=std::string::npos&&worker.history(tab).get().entries.size()==1,"Existing shared host application navigates correct commentary and records one visit");
  std::string uri="sword://N/Genesis1:1-2";accepted=worker.follow_pane_link_at(tab,"Text","Notes","John3:16",uri);uri.clear();worker.close_tab(tab).get();
  rejects<std::invalid_argument>([&]{(void)worker.resolve_study_link_at(tab,"Text","John3:16","sword://N/John3:16-18").get();});
 }
 check(std::get<PassageLink>(accepted.get()).reference=="Genesis 1:1","Accepted target copies URI and drains before owner shutdown");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C navigation owner");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_follow_pane_link_at(worker,tab,"Text","Personal","John3:23","sword://N/John3:16-18",&request)==XC_OK&&wait(request)==XC_OK,"C contextual commentary target");auto *link=xc_request_take_link(request);check(link&&!xc_request_take_link(request)&&!xc_request_take_reference_list(request),"C target kind/take once");xc_request_release(request);
 check(xc_worker_visit_linked_passage(worker,tab,"Text","Personal",xc_link_module(link),xc_link_reference(link),&request)==XC_OK,"Apply copied C target through existing navigation");xc_link_release(link);check(wait(request)==XC_OK,"C target navigation done");auto *passage=xc_request_take_passage(request);xc_request_release(request);check(passage&&std::string(xc_passage_commentary_module(passage))=="Notes"&&std::string(xc_passage_commentary_html(passage)).find("Original Notes")!=std::string::npos,"C actual-source commentary output");xc_passage_release(passage);
 check(xc_worker_follow_pane_link_at(worker,tab,"Text","Notes","John3:23","sword://Missing/Genesis1:1-2",&request)==XC_OK,"Queue unknown range target");xc_worker_close(worker);check(wait(request)==XC_OK,"C target drains on close");link=xc_request_take_link(request);xc_request_release(request);check(link&&std::string(xc_link_module(link))=="Text"&&std::string(xc_link_reference(link))=="Genesis 1:1","C actual main target owns canonical reference after shutdown");xc_link_release(link);
 std::cout<<"Cloned navigation targets: "<<comparisons<<" frozen SDK comparisons, range type/fallback routing, C ownership and explicit application passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
