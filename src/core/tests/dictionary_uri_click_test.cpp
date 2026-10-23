// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){try{f();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected rejected dictionary click");}
xc_status wait(xc_request *r){xc_status s;const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Click deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return s;}
int main(int argc,char **argv){try{
 check(argc==2,"Original navigation fixture required");const std::string root=argv[1];std::future<ContextualStudyLink> accepted;
 {StudyWorker worker(root);const auto tab=worker.create_tab().get(),other=worker.create_tab().get();
  (void)worker.visit_passage(tab,"Text","Personal","John3:17").get();
  const auto before=worker.history(tab).get();const auto generation=worker.search_generation(tab).get().id;
  const auto previewTarget=std::get<DictionaryUriEntry>(worker.resolve_study_link_at(tab,"Text","John3:17","sword://D/alpha-beta;gamma,delta/part%23tag!note?query").get());
  const auto &preview=previewTarget.entry;check(previewTarget.anchor=="tag!note?query","Resolve owns stripped dictionary URI anchor separately from lookup key");
  check(preview.module=="Dictionary"&&preview.key=="ALPHA-BETA;GAMMA,DELTA/PART"&&preview.html.find("Key alpha-beta;gamma,delta/part")!=std::string::npos&&preview.html.find("#tag")==std::string::npos&&worker.dictionary_history(tab).get().entries.empty(),"Resolve strips click anchor but does not visit dictionary history");
  const auto clicked=std::get<DictionaryUriEntry>(worker.follow_pane_link_at(tab,"Text","Personal","John3:17","sword://D/anchor-key!before%23after").get());
  auto entry=clicked.entry;check(clicked.anchor=="after","Click preserves retained dictionary output anchor");
  auto history=worker.dictionary_history(tab).get();
  check(entry.module=="Dictionary"&&entry.key=="ANCHOR-KEY!BEFORE"&&history.entries.size()==1&&history.entries.front().module=="Dictionary"&&history.entries.front().key=="ANCHOR-KEY!BEFORE","Click hash precedes earlier bang and records normalized requested key with actual module");
  entry=std::get<DictionaryUriEntry>(worker.follow_study_link_at(tab,"Text","John3:17","bible://D/anchor-key!only").get()).entry;
  check(entry.key=="ANCHOR-KEY"&&worker.dictionary_history(tab).get().entries.size()==2,"Bang-only anchor strips before dictionary visit");
  entry=std::get<DictionaryUriEntry>(worker.follow_study_link_at(tab,"Text","John3:17","sword://D/al").get()).entry;history=worker.dictionary_history(tab).get();
  check(history.entries.back().key=="AL"&&entry.key!="AL","History retains normalized requested key separately from SDK match");
  const auto top=std::get<DictionaryUriEntry>(worker.follow_pane_link_at(tab,"Dictionary","Personal","John3:17","sword:///alpha%23").get());entry=top.entry;check(top.anchor&&top.anchor->empty(),"Explicit empty dictionary anchor requests top");check(entry.module=="Dictionary"&&entry.key=="ALPHA","Omitted click module uses main navigation context, including empty anchor");
  entry=std::get<DictionaryUriEntry>(worker.follow_study_link_at(tab,"Text","John3:17","sword://LongDictionaryName20/alpha").get()).entry;check(entry.module=="LongDictionaryName20","Click does not inherit hover's short declared-module scan");
  const auto count=worker.dictionary_history(tab).get().entries.size();
  for(const auto *bad:{"sword://DailyAlias/alpha","sword://D/%23only","sword://D/!only","sword://D/a%G0"})rejects([&]{(void)worker.follow_study_link_at(tab,"Text","John3:17",bad).get();});
  check(worker.dictionary_history(tab).get().entries.size()==count&&worker.dictionary_history(other).get().entries.empty()&&worker.history(tab).get().entries.size()==before.entries.size()&&worker.search_generation(tab).get().id==generation,"Rejected/devotional clicks and successful dictionary visits preserve Bible/search/sibling state");
  auto back=worker.navigate_dictionary_history(tab,-1).get();check(back.module=="Dictionary","Existing dictionary Back consumes URI visits without transient anchor metadata");
  std::string uri="sword://D/alpha#tag";accepted=worker.follow_study_link_at(tab,"Text","John3:17",uri);uri.clear();worker.close_tab(tab).get();
  rejects([&]{(void)worker.follow_study_link_at(tab,"Text","John3:17","sword://D/alpha").get();});
 }
 check(std::get<DictionaryUriEntry>(accepted.get()).entry.module=="Dictionary","Accepted click drains to copied result after shutdown");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C click owner");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_follow_pane_link_at(worker,tab,"Text","Personal","John3:17","sword://D/anchor-key!before%23after",&request)==XC_OK,"C dictionary click submission");xc_worker_close(worker);check(wait(request)==XC_OK,"C click drains");auto *entry=xc_request_take_dictionary_entry(request);check(entry&&!xc_request_take_dictionary_entry(request)&&!xc_request_take_link(request)&&!xc_request_take_lexical_entry(request),"Distinct C dictionary click result transfers once");xc_request_release(request);check(xc_dictionary_entry_anchor(entry)&&std::string(xc_dictionary_entry_anchor(entry))=="after"&&std::string(xc_dictionary_entry_key(entry))=="ANCHOR-KEY!BEFORE"&&std::string(xc_dictionary_entry_html(entry)).find("Original Dictionary")!=std::string::npos,"C dictionary click result owns data after request/owner");xc_dictionary_entry_release(entry);check(!xc_dictionary_entry_anchor(nullptr),"Null dictionary anchor getter");
 std::cout<<"Dictionary URI click: retained anchor/uppercase policy, requested history keys, main/source separation, copied C result and explicit devotional exclusion passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
