// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){try{f();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected invalid anchor URI");}
xc_status wait(xc_request *r){xc_status s;const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Anchor target deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return s;}
int main(int argc,char **argv){try{
 check(argc==2,"Original navigation fixture required");const std::string root=argv[1];
 for(const auto &row:std::vector<std::pair<std::string,std::string>>{{"#tag","tag"},{"!note","note"},{"!before#after","after"},{"#first!later","first!later"},{"#",""},{"%23caf%C3%A9+%E6%96%87%E5%AD%97","café 文字"},{"#literal%2520","literal%20"},{"#quote%22%5C%27%3C%2Fscript%3E","quote\"\\'</script>"}}){
  const auto parsed=std::get<PassageLink>(contextual_study_link("sword://T/John3:16"+row.first,"Text"));
  check(parsed.module=="T"&&parsed.anchor&&*parsed.anchor==row.second,"Typed parser copies literal decoded anchor with retained precedence");
  check(parsed.reference==(row.first=="!before#after"?"John3:16!before":"John3:16"),"Anchor removes only retained selected suffix");
 }
 check(!std::get<PassageLink>(contextual_study_link("sword://T/John3:16","Text")).anchor,"Absent anchor remains distinct from empty");
 const auto list=std::get<ReferenceListLink>(contextual_study_link("bible://N/John3:16-18%23note","Text"));check(list.expression=="John3:16-18"&&list.anchor=="note","Range staging owns stripped anchor before module classification");
 for(const auto *bad:{"sword://T/#only","sword://T/!only","sword://T/John3:16#%00","sword://T/John3:16#%0A","sword://T/John3:16#%GG","sword://T/John3:16?query#tag"})rejects([&]{(void)contextual_study_link(bad,"Text");});
 {Reader reader(root);rejects([&]{(void)reader.resolve_link("Text","sword://T/John3:16#note");});}
 std::future<ContextualStudyLink> accepted;
 {StudyWorker worker(root);const auto tab=worker.create_tab().get(),other=worker.create_tab().get();
  (void)worker.visit_passage(tab,"Text","Personal","John3:17").get();const auto before=worker.history(tab).get();const auto generation=worker.search_generation(tab).get().id;
  for(const auto *token:{"T","N","P","Missing",""})for(const auto *suffix:{"#tag","!note","#","%23caf%C3%A9+%E6%96%87%E5%AD%97","#%22%5C%27%3C%2Fscript%3E"}){
   const auto target=std::get<PassageLink>(worker.follow_pane_link_at(tab,"Text","Personal","John3:17",std::string("sword://")+token+"/John3:16"+suffix).get());
   const std::string expected=std::string(token)=="N"?"Notes":std::string(token)=="P"?"Personal":"Text";
   check(target.module==expected&&target.reference=="John 3:16"&&target.anchor,"Owner returns actual canonical Bible/commentary target with owned anchor");
  }
  const auto commentary=std::get<PassageLink>(worker.follow_pane_link_at(tab,"Text","Personal","John3:17","sword://N/John3:16-18#note").get());check(commentary.module=="Notes"&&commentary.anchor=="note","Commentary ranges retain navigation anchor");
  const auto missing=std::get<PassageLink>(worker.follow_pane_link_at(tab,"Text","Personal","John3:17","sword://Missing/John3:16-18#note").get());check(missing.module=="Text"&&missing.anchor=="note","Unknown range exact main fallback retains anchor");
  const auto sidebar=std::get<ReferenceList>(worker.follow_pane_link_at(tab,"Text","Personal","John3:17","sword://T/John3:16-18#note").get());check(sidebar.entries.size()==3&&sidebar.expression=="John3:16-18","Bible range stays sidebar after anchor split");
  check(worker.history(tab).get().entries.size()==before.entries.size()&&worker.history(other).get().entries.empty()&&worker.search_generation(tab).get().id==generation,"Anchor target resolution does not navigate or publish history/search");
  std::string uri="sword://N/John3:16#owned";accepted=worker.follow_pane_link_at(tab,"Text","Personal","John3:17",uri);uri.clear();worker.close_tab(tab).get();
 }
 check(std::get<PassageLink>(accepted.get()).anchor=="owned","Anchor owns copied bytes through drain and shutdown");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C anchor owner");const auto tab=xc_request_tab(request);xc_request_release(request);
 for(const auto *suffix:{"","#","#owned"}){
  check(xc_worker_follow_pane_link_at(worker,tab,"Text","Personal","John3:17",(std::string("sword://N/John3:16")+suffix).c_str(),&request)==XC_OK&&wait(request)==XC_OK,"C anchor target");
  auto *target=xc_request_take_link(request);check(target&&!xc_request_take_link(request),"C anchor target takes once");xc_request_release(request);
  const auto *anchor=xc_link_anchor(target);check(*suffix?anchor&&std::string(anchor)==(std::string(suffix)=="#"?"":"owned"):anchor==nullptr,"C null/empty/owned anchor semantics");xc_link_release(target);
 }
 check(xc_worker_follow_pane_link_at(worker,tab,"Text","Personal","John3:17","sword://N/John3:16#held",&request)==XC_OK,"C drain anchor target");xc_worker_close(worker);check(wait(request)==XC_OK,"C anchor request drains");auto *target=xc_request_take_link(request);xc_request_release(request);check(target&&std::string(xc_link_anchor(target))=="held"&&std::string(xc_link_module(target))=="Notes","C anchor target survives request and owner");xc_link_release(target);check(!xc_link_anchor(nullptr),"Null anchor getter");
 std::cout<<"Owned contextual anchor targets: precedence, single decoding, canonical owner routing, sidebar distinction, isolation and C lifetime passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
