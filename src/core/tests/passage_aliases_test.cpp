// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <rawld.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
void check(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
template<class E,class F>void rejects(F call) {try{call();}catch(const E &){return;}throw std::runtime_error("Expected rejected passage alias");}
xc_status wait(xc_request *r){xc_status status;const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Passage alias poll deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Generated alias fixture required");const std::string root=argv[1];PassageLink held;
 check(passage_link("sword://Shared/John1:1","AFirst").module=="Shared","Pure URI parsing preserves literal tokens before owner resolution");
 {Reader reader(root);
  for(const auto &uri:{"sword://Shared/John1:1","bible://Shared/John1:1"}) {
   const auto target=reader.resolve_link("AFirst",uri);check(target.module=="BSecond"&&target.reference=="John1:1","Typed passage target uses shared alias registration");
  }
  check(reader.resolve_link("Shared","sword:///John1:1").module=="BSecond","URI module omission uses supplied current alias");
  check(reader.resolve_link("Shared","sword://AFirst/John1:1").module=="AFirst","Explicit installed ID overrides fallback alias");
  held=reader.resolve_link("AFirst","sword://文字+caf%C3%A9/John1:1");check(held.module=="Unicode","Decoded Unicode/space alias remains exact");
  for(const auto &uri:{"https://example.com","sword://Shared/John1:1-2","sword://Shared/John1:1#anchor","sword://%00/John1:1","sword://%FF/John1:1"})rejects<std::invalid_argument>([&]{reader.resolve_link("AFirst",uri);});
  rejects<std::out_of_range>([&]{reader.resolve_link("AFirst","sword://Missing/John1:1");});
  rejects<std::invalid_argument>([&]{reader.resolve_link(std::string("AFirst\0hidden",13),"sword:///John1:1");});
  StudySession session(reader);session.visit("AFirst","John1:1");const auto before=session.search_module({"AFirst","Original"}).generation.id;
  reader.resolve_link("AFirst","sword://Shared/John1:1");
  check(session.history().entries().size()==1&&session.search_generation().id==before,"Read-only passage resolution never applies navigation or search state");
 }
 std::future<PassageLink> draining;
 {StudyWorker worker(root);const auto tab=worker.create_tab().get();
  std::string uri="sword://Shared/John1:1";auto copied=worker.resolve_link("AFirst",uri);uri.clear();check(copied.get().module=="BSecond","FIFO passage request copies URI before caller mutation");
  auto contextual=worker.resolve_study_link_at(tab,"AFirst","John1:1","sword://Shared/John1:1").get();
  check(std::get<PassageLink>(contextual).module=="BSecond"&&worker.history(tab).get().entries.empty(),"Contextual passage resolution returns installed source without history visit");
  auto pane=worker.follow_pane_link_at(tab,"AFirst","BSecond","John1:1","sword://Shared/John1:1").get();check(std::get<PassageLink>(pane).module=="BSecond","Pane dispatch resolves explicit aliases on shared owner");
  auto fallback=worker.follow_pane_link_at(tab,"AFirst","BSecond","John1:1","sword:///John1:1").get();check(std::get<PassageLink>(fallback).module=="AFirst","Relative URI retains existing navigation-pane fallback");
  const auto command=worker.resolve_study_link(tab,"AFirst","passagestudy.jsp?action=showRef&type=scripRef&module=AFirst&value=John1:1").get();check(std::get<PassageLink>(command).module=="AFirst","Exact single-reference command remains supported");
  rejects<std::out_of_range>([&]{(void)worker.resolve_study_link(tab,"AFirst","passagestudy.jsp?action=showRef&type=scripRef&module=Shared&value=John1:1").get();});
  rejects<std::out_of_range>([&]{(void)worker.resolve_study_link_at(tab,"AFirst","John1:1","passagestudy.jsp?action=showRef&type=scripRef&module=Shared&value=John1:1").get();});
  worker.close_tab(tab).get();rejects<std::invalid_argument>([&]{(void)worker.resolve_study_link_at(tab,"AFirst","John1:1","sword://Shared/John1:1").get();});
  draining=worker.resolve_link("AFirst","sword://Shared/John1:1");
 }
 check(draining.get().module=="BSecond"&&held.module=="Unicode","Owned targets survive shutdown and accepted lookups drain");
 {Reader bad(root+"/bad-metadata");check(bad.resolve_link("Bad","sword:///John1:1").module=="Bad","Exact link IDs bypass unrelated bad alias metadata");rejects<std::invalid_argument>([&]{bad.resolve_link("Bad","sword://Unknown/John1:1");});}
 const auto dictRoot=std::filesystem::u8path(root)/"link-dictionary";
 std::filesystem::remove_all(dictRoot);std::filesystem::create_directories(dictRoot/"mods.d");std::filesystem::create_directories(dictRoot/"data");
 check(sword::RawLD::createModule((dictRoot/"data").string().c_str())==0,"Create original dictionary type fixture");
 {std::ofstream config(dictRoot/"mods.d/dictionary.conf");config<<"[Dictionary]\nDataPath=./data\nModDrv=RawLD\nSourceType=Plain\nEncoding=UTF-8\nLang=en\nDescription=Original alias type fixture\nAbbreviation=Lex\n";}
 {Reader dictionary(dictRoot.u8string());check(dictionary.resolve_module_name("Lex").module=="Dictionary","Non-verse alias registers before passage-family rejection");rejects<std::invalid_argument>([&]{dictionary.resolve_link("Dictionary","sword://Lex/John1:1");});}
 xc_reader *reader=nullptr;xc_link *link=nullptr;
 check(xc_open(root.c_str(),&reader)==XC_OK&&xc_resolve_link(reader,"AFirst","sword://Shared/John1:1",&link)==XC_OK,"Existing C resolver consumes shared aliases");xc_close(reader);
 check(std::string(xc_link_module(link))=="BSecond"&&std::string(xc_link_reference(link))=="John1:1","Owned C link survives reader teardown");xc_link_release(link);
 xc_worker *worker=nullptr;xc_request *request=nullptr;
 check(xc_worker_open(root.c_str(),&worker)==XC_OK&&xc_worker_resolve_link(worker,"AFirst","sword://Shared/John1:1",&request)==XC_OK&&!xc_request_take_link(request),"Queue C passage alias with pre-poll guard");xc_worker_close(worker);
 check(wait(request)==XC_OK,"C shutdown drains passage aliases");link=xc_request_take_link(request);check(link&&!xc_request_take_link(request),"C passage alias take once");xc_request_release(request);check(std::string(xc_link_module(link))=="BSecond","C worker passage target owns actual ID after shutdown");xc_link_release(link);
 std::cout<<"Typed passage URI alias resolution, fallback, type guards, copied ownership and state isolation passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
