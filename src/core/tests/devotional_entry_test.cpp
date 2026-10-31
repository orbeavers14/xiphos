// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/render.hpp"
#include <markupfiltmgr.h>
#include <swmodule.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class E,class F>void rejects(F f){try{f();}catch(const E &){return;}throw std::runtime_error("Expected rejected devotional read");}
xc_status wait(xc_request *r){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);xc_status status;while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Devotional deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Original navigation fixture required");const std::string root=argv[1];
 DevotionalEntry held;
 {Reader reader(root);EngineManager baseline(root.c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);
  auto *daily=baseline.getModule("Daily");check(daily,"Daily fixture installed");
  for(const auto *key:{"","01.01","02.29","07.09","07.10","12.31","99.99","ab.cd","07x09","02.30"}){
   held=reader.read_devotional("Daily",key,7,9);
   const auto expected=render_entry(*daily,held.date.key.c_str());
   check(held.module=="Daily"&&held.html==expected&&held.matched_key==daily->getKeyText(),"Owned devotional agrees with SDK rendering and matching");
   if(!*key||std::string(key)=="99.99"||std::string(key)=="ab.cd")check(held.date.used_today&&held.date.key=="07.09"&&held.html.find("Original Daily devotional prose for 07.09")!=std::string::npos,"Local today and invalid month use owned selected key");
   if(std::string(key)=="07.10")check(held.date.key=="07.10"&&held.matched_key!=held.date.key,"Selected date remains distinct from nearest SDK match");
  }
  for(const auto *module:{"Dictionary","Text"})rejects<std::invalid_argument>([&]{(void)reader.read_devotional(module,"07.09",7,9);});
  for(const auto *module:{"Missing","DailyAlias"})rejects<std::out_of_range>([&]{(void)reader.read_devotional(module,"07.09",7,9);});
 }
 check(!held.html.empty(),"Owned result survives Reader close");
 std::future<DevotionalEntry> accepted;
 {StudyWorker worker(root);const auto tab=worker.create_tab().get(),other=worker.create_tab().get();
  (void)worker.visit_passage(tab,"Text","Notes","John3:17").get();(void)worker.visit_dictionary(tab,"Dictionary","alpha").get();
  const auto history=worker.history(tab).get();const auto dictionary=worker.dictionary_history(tab).get();const auto generation=worker.search_generation(tab).get().id;
  const std::string longDate(maximum_devotional_date_bytes+1,'x'),longModule(maximum_devotional_module_bytes+1,'m');
  auto bad=worker.read_devotional(tab,"Daily",longDate,7,9);check(bad.wait_for(std::chrono::seconds(0))==std::future_status::ready,"Oversized date rejects before FIFO");rejects<std::invalid_argument>([&]{(void)bad.get();});
  auto badModule=worker.read_devotional(tab,longModule,"",7,9);check(badModule.wait_for(std::chrono::seconds(0))==std::future_status::ready,"Oversized module rejects before FIFO");rejects<std::invalid_argument>([&]{(void)badModule.get();});
  auto nul=worker.read_devotional(tab,"Daily",std::string("07\0.9",5),7,9);check(nul.wait_for(std::chrono::seconds(0))==std::future_status::ready,"NUL date rejects before FIFO");rejects<std::invalid_argument>([&]{(void)nul.get();});
  held=worker.read_devotional(tab,"Daily",std::string(maximum_devotional_date_bytes,'x'),7,9).get();check(held.date.key=="07.09","Exact date byte boundary admitted and falls back");
  rejects<std::out_of_range>([&]{(void)worker.read_devotional(tab,std::string(maximum_devotional_module_bytes,'m'),"",7,9).get();});
  check(worker.history(tab).get().entries.size()==history.entries.size()&&worker.dictionary_history(tab).get().entries.size()==dictionary.entries.size()&&worker.dictionary_history(tab).get().cursor==dictionary.cursor&&worker.search_generation(tab).get().id==generation&&worker.dictionary_history(other).get().entries.empty(),"Devotional reads preserve Bible/dictionary history, search generation and sibling state");
  std::string input="02.29";accepted=worker.read_devotional(tab,"Daily",input,7,9);input.clear();worker.close_tab(tab).get();
  rejects<std::invalid_argument>([&]{(void)worker.read_devotional(tab,"Daily","07.09",7,9).get();});
 }
 held=accepted.get();check(held.date.key=="02.29"&&held.html.find("02.29")!=std::string::npos,"Accepted copied devotional drains and survives worker close");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C devotional owner");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_read_devotional(worker,tab,"Dictionary","07.09",7,9,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT,"C rejects ordinary dictionary as devotional");check(!xc_request_take_devotional_entry(request),"Failed C read has no devotional result");xc_request_release(request);
 for(const auto *date:{static_cast<const char *>(nullptr),"99.99","07.10"}){
  check(xc_worker_read_devotional(worker,tab,"Daily",date,7,9,&request)==XC_OK&&wait(request)==XC_OK,"C devotional read");
  auto *entry=xc_request_take_devotional_entry(request);check(entry&&!xc_request_take_devotional_entry(request)&&!xc_request_take_dictionary_entry(request),"Distinct C devotional kind takes once");xc_request_release(request);
  check(std::string(xc_devotional_entry_date(entry))==(date&&std::string(date)=="07.10"?"07.10":"07.09")&&xc_devotional_entry_month(entry)==7&&xc_devotional_entry_day(entry)==(date&&std::string(date)=="07.10"?10:9)&&xc_devotional_entry_used_today(entry)==(!date||std::string(date)=="99.99"),"C copied selected date metadata");xc_devotional_entry_release(entry);
 }
 check(xc_worker_read_devotional(worker,tab,"Daily","02.29",7,9,&request)==XC_OK,"C pending read");xc_worker_close(worker);check(wait(request)==XC_OK,"C accepted read drains");auto *entry=xc_request_take_devotional_entry(request);xc_request_release(request);
 check(entry&&std::string(xc_devotional_entry_module(entry))=="Daily"&&std::string(xc_devotional_entry_date(entry))=="02.29"&&std::string(xc_devotional_entry_matched_key(entry))=="02.29"&&std::string(xc_devotional_entry_html(entry)).find("02.29")!=std::string::npos,"C result survives request and owner");xc_devotional_entry_release(entry);
 check(!xc_devotional_entry_module(nullptr)&&!xc_devotional_entry_date(nullptr)&&!xc_devotional_entry_matched_key(nullptr)&&!xc_devotional_entry_html(nullptr)&&!xc_devotional_entry_month(nullptr)&&!xc_devotional_entry_day(nullptr)&&!xc_devotional_entry_used_today(nullptr),"C null getters");xc_devotional_entry_release(nullptr);
 std::cout<<"Owned devotional service: SDK date/matched-key split, bounded admission, history isolation, drain and C lifetime passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
