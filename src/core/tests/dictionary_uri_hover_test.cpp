// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/link.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){try{f();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected invalid URI");}
xc_status wait(xc_request *r){xc_status s;const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Hover deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return s;}
int main(int argc,char **argv){try{
 check(argc==2,"Original navigation fixture required");const std::string root=argv[1];
 const std::string key="alpha-beta;gamma,delta/part#tag!note?query";
 const auto raw=module_uri_link("sword://D/"+key);check(raw.module=="D"&&raw.key==key,"Raw keyed URI retains scripture delimiters, slash, query and anchors");
 const auto unicode=module_uri_link("bible://D/caf%C3%A9+%E6%96%87%E5%AD%97");check(unicode.key=="café 文字","Whole URI decodes once with retained plus-space semantics");
 check(module_uri_link("sword://D/literal%2520key").key=="literal%20key"&&module_uri_link("sword://D/plus%2Bkey").key=="plus+key","Escaped percent/plus are not decoded twice");
 for(const auto *bad:{"https://D/alpha","sword://D","sword://D/","sword://D/a%","sword://D/a%G0","sword://D/%00","sword://D/%0A"})rejects([&]{(void)module_uri_link(bad);});
 rejects([&]{(void)module_uri_link("sword://D/"+std::string(32769,'a'));});
 DictionaryEntry held;std::future<StudyHoverLink> accepted;
 {StudyWorker worker(root);const auto tab=worker.create_tab().get(),other=worker.create_tab().get();
  (void)worker.visit_passage(tab,"Text","Personal","John3:17").get();
  (void)worker.visit_dictionary(tab,"Dictionary","alpha").get();
  const auto history=worker.history(tab).get();const auto dictionary=worker.dictionary_history(tab).get();const auto generation=worker.search_generation(tab).get().id;
  for(const auto &candidate:std::vector<std::pair<std::string,std::string>>{{"sword://D/"+key,"Dictionary"},{"bible://D/caf%C3%A9+%E6%96%87%E5%AD%97","Dictionary"},{"sword://D/literal%2520key","Dictionary"},{"sword://D/plus%2Bkey","Dictionary"},{"sword://DailyAlias/alpha","Daily"},{"sword://LongDictionaryNameX/alpha","LongDictionaryNameX"},{"sword://L20/alpha","LongDictionaryName20"}}){
   held=std::get<DictionaryEntry>(worker.hover_study_link_at(tab,"Text","John3:17",candidate.first).get());
   check(held.module==candidate.second&&held.html.find("Original "+candidate.second+" navigation prose.")!=std::string::npos,"Hover returns actual declared dictionary body including daily-devotion SDK dictionary");
   if(candidate.first=="sword://D/"+key)check(held.key=="ALPHA-BETA;GAMMA,DELTA/PART#TAG!NOTE?QUERY","SDK matching receives the entire raw dictionary key");
  }
  for(const auto *uri:{"sword:///alpha","sword://Missing/alpha","sword://T/John3:16-18","sword://N/John3:16","sword://LongDictionaryName20/alpha"})check(std::holds_alternative<std::monostate>(worker.hover_study_link_at(tab,"Text","John3:17",uri).get()),"Non-dictionary/empty/unknown/long declared hover modules do not navigate or preview");
  const auto after=worker.dictionary_history(tab).get();check(after.entries.size()==dictionary.entries.size()&&after.entries.front().module==dictionary.entries.front().module&&after.entries.front().key==dictionary.entries.front().key&&after.cursor==dictionary.cursor&&worker.history(tab).get().entries.size()==history.entries.size()&&worker.search_generation(tab).get().id==generation&&worker.dictionary_history(other).get().entries.empty(),"Dictionary hover preserves history/search and sibling tabs");
  std::string uri="sword://D/"+key;accepted=worker.hover_study_link_at(tab,"Text","John3:17",uri);uri.clear();worker.close_tab(tab).get();
  rejects([&]{(void)worker.hover_study_link_at(tab,"Text","John3:17","sword://D/alpha").get();});
 }
 held=std::get<DictionaryEntry>(accepted.get());check(held.module=="Dictionary"&&held.html.find("Original Dictionary")!=std::string::npos,"Accepted hover drains with owned copied result after shutdown");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C hover owner");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_hover_study_link_at(worker,tab,"Text","John3:17",("sword://D/"+key).c_str(),&request)==XC_OK,"C dictionary URI submission");xc_worker_close(worker);check(wait(request)==XC_OK&&!xc_request_no_presentation(request),"C dictionary hover drains");auto *entry=xc_request_take_dictionary_entry(request);check(entry&&!xc_dictionary_entry_anchor(entry)&&!xc_request_take_dictionary_entry(request)&&!xc_request_take_lexical_entry(request)&&!xc_request_take_note(request),"C dictionary result kind/take once");xc_request_release(request);check(std::string(xc_dictionary_entry_module(entry))=="Dictionary"&&std::string(xc_dictionary_entry_html(entry)).find("Original Dictionary")!=std::string::npos,"C copied dictionary survives request/owner");xc_dictionary_entry_release(entry);
 std::cout<<"Dictionary URI hover: raw SDK keys, aliases/types, bounded parsing, history isolation and copied C ownership passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
