// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/search_policy.hpp"
#include "xiphos/core/c_api.h"
#include "../sdk_search.hpp"
#include <rawld4.h>
#include <rawgenbook.h>
#include <markupfiltmgr.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
using namespace xiphos::core;
namespace fs=std::filesystem;
static void check(bool ok,const std::string &why){if(!ok)throw std::runtime_error(why);}
static auto bytes(const fs::path &root){std::map<std::string,std::string> out;for(const auto &file:fs::recursive_directory_iterator(root))if(file.is_regular_file()){std::ifstream in(file.path(),std::ios::binary);out.emplace(fs::relative(file.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(in),{}));}return out;}
static std::vector<std::string> keys(sword::ListKey &list){std::vector<std::string> out;list.setPosition(sword::TOP);while(list.getCount()&&!list.popError()){out.emplace_back(list.getText());list++;}return out;}
static std::string content(unsigned i,bool thml){
 const std::string lemma=i==0?"G140":i==1?"G1401":"G141",morph=i==0?"N-ASF":i==1?"N-ASF1":"V-PAP",note=i==0?"Original Alpha café note":i==1?"Original alpha continuation":"Original other note";
 if(thml)return "Original word<sync type=\"Strongs\" value=\""+lemma+"\"/><sync type=\"morph\" class=\"Robinson\" value=\""+morph+"\"/><note>"+note+"</note>";
 return "Original <w lemma=\"strong:"+lemma+"\" morph=\"robinson:"+morph+"\">word</w><note type=\"study\" n=\"a\">"+note+"</note>";
}
static xc_status wait(xc_request *r){auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);xc_status status;while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<deadline,"C keyed attribute deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv)try{
 check(argc==2,"Named disposable keyed-attribute root required");const fs::path root=argv[1];check(root.filename()=="keyed-attributes-fixture","Named scratch only");fs::remove_all(root);fs::create_directories(root/"mods.d");
 const std::vector<std::string> ids{"AttrDictionaryOSIS","AttrBookOSIS","AttrDictionaryThML","AttrBookThML"};
 for(const auto &id:ids){const bool dictionary=id.find("Dictionary")!=id.npos,thml=id.find("ThML")!=id.npos;auto directory=root/"data"/id;fs::create_directories(directory);auto prefix=(directory/"entries").string();
  if(dictionary){check(sword::RawLD4::createModule(prefix.c_str())==0,"Create dictionary");sword::RawLD4 writer(prefix.c_str());for(unsigned i=0;i<3;++i){writer.setKey(i==0?"John.3.16":i==1?"Alpha.Key":"שלום");writer.setEntry(content(i,thml).c_str());}}
  else{check(sword::RawGenBook::createModule(prefix.c_str())==0,"Create book");sword::RawGenBook writer(prefix.c_str());for(unsigned i=0;i<3;++i){writer.setKey(i==0?"/Part/Entry Ω":i==1?"/Part/Other":"/Part/שלום");writer.setEntry(content(i,thml).c_str());}}
  std::ofstream conf(root/"mods.d"/(id+".conf"));conf<<'['<<id<<"]\nDataPath=./data/"<<id<<"/entries\nModDrv="<<(dictionary?"RawLD4":"RawGenBook")<<"\nSourceType="<<(thml?"ThML":"OSIS")<<"\nEncoding=UTF-8\n";for(const char *filter:{"Strongs","Morph","Footnotes"})conf<<"GlobalOptionFilter="<<(thml?"ThML":"OSIS")<<filter<<'\n';
 }
 const auto original=bytes(root/"data");
 std::map<std::string,std::vector<std::string>> expected;
 {
  EngineManager manager(root.string().c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);
  for(const auto &id:ids)for(auto mode:{AttributeSearch::lemma,AttributeSearch::morphology,AttributeSearch::footnote})for(bool sensitive:{false,true}){
   auto *module=manager.getModule(id.c_str());check(module,"Installed keyed fixture");auto query=attribute_search_text(mode==AttributeSearch::lemma?"G140":mode==AttributeSearch::morphology?"N-ASF":"Alpha",mode);std::string stripped=sword::SWBuf(module->stripText(query.expression.c_str())).c_str();
   auto result=keys(module->search(stripped.c_str(),detail::sdk_search_attribute,(sensitive?0:XIPHOS_SWORD_SEARCH_ICASE_FLAG)|(query.whole_entry?detail::sdk_search_whole_entry:0)));
   expected[id+std::to_string(static_cast<int>(mode))+std::to_string(sensitive)]=result;
   std::cout<<id<<" mode="<<static_cast<int>(mode)<<" sensitive="<<sensitive<<" hits="<<result.size()<<'\n';
   check(result.size()==(mode==AttributeSearch::footnote&&!sensitive?2:1),"Actual SDK keyed attribute count: "+id);
  }
 }
 {
  Reader reader(root.string());StudySession session(reader);
  for(const auto &id:ids)for(auto mode:{SearchMode::lemma,SearchMode::morphology,SearchMode::footnote})for(bool sensitive:{false,true}){
   const auto attr=mode==SearchMode::lemma?AttributeSearch::lemma:mode==SearchMode::morphology?AttributeSearch::morphology:AttributeSearch::footnote;
   SearchQuery query{id,mode==SearchMode::lemma?"G140":mode==SearchMode::morphology?"N-ASF":"Alpha",mode};query.case_sensitive=sensitive;
   auto result=reader.search_keyed_module(query);
   check(result.executed&&result.references==expected.at(id+std::to_string(static_cast<int>(attr))+std::to_string(sensitive)),"Shared attribute results preserve exact SDK opaque keys");
   check(result.key_kind==(id.find("Dictionary")!=id.npos?SearchKeyKind::dictionary:SearchKeyKind::general_book),"Attribute key family preserved");
  }
  for(const auto &id:ids){
   auto seed=session.search_mixed_batch({{id,"G140",SearchMode::lemma}});
   check(!seed.failures&&seed.copied_results==1,"Mixed attribute seed");
   SearchQuery query{id,"Alpha",SearchMode::footnote};query.within_last_results=true;
   auto narrowed=session.search_mixed_batch({query});
   check(!narrowed.failures&&narrowed.copied_results==1&&narrowed.groups[0].result->references==seed.groups[0].result->references,"Attribute last scope cannot expand to second note");
   auto prefix=reader.search_keyed_module({id,"Alpha",SearchMode::footnote},{1,4096});
   check(prefix.limited&&prefix.total_results==2&&prefix.references.size()==1,"Attribute copied prefix preserves full count");
   auto control=std::make_shared<SearchControl>();control->request_cancel();
   auto stopped=reader.search_keyed_module({id,"G140",SearchMode::lemma},{},control);
   check(stopped.cancelled&&!stopped.executed,"Attribute pre-execution cancellation");
  }
 }
 xc_worker *worker=nullptr;check(xc_worker_open(root.string().c_str(),&worker)==XC_OK,"C attribute worker");
 xc_request *request=nullptr;check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C attribute tab");
 const auto tab=xc_request_tab(request);xc_request_release(request);
 xc_search_options options;xc_search_options_default(&options);options.mode=XC_SEARCH_LEMMA;
 std::string expression="G140";
 check(xc_worker_search_keyed_module(worker,tab,"AttrBookOSIS",expression.c_str(),&options,&request)==XC_OK,"C attribute admission");
 expression="changed";options.mode=XC_SEARCH_FOOTNOTE;
 check(wait(request)==XC_OK,"C attribute execution");auto *owned=xc_request_take_search(request);
 check(owned&&!xc_request_take_search(request),"C attribute take once");xc_request_release(request);xc_worker_close(worker);
 xc_search_info info{};check(xc_search_get_info(owned,&info)==XC_OK&&info.result_count==1&&xc_search_key_kind(owned)==2&&std::string(xc_search_reference(owned,0))=="/Part/Entry Ω","Copied attribute input/result survives worker teardown");
 xc_search_release(owned);
 check(bytes(root/"data")==original,"Attribute searches never modify source data");
 std::cout<<"Actual OSIS/ThML dictionary/book attribute execution and exact identifier/case semantics passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
