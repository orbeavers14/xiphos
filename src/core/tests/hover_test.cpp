// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>
using namespace xiphos::core;
namespace {
void check(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
template<class F> void rejected(F operation) { bool failed=false; try { operation(); } catch(const std::invalid_argument &) { failed=true; } check(failed,"invalid hover rejected"); }
xc_status wait(xc_request *request) {
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
 xc_status status;
 while((status=xc_request_poll(request))==XC_PENDING) { check(std::chrono::steady_clock::now()<deadline,"hover request deadline"); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
 return status;
}
const char *body="passagestudy.jsp?action=showNote&type=n&value=1&module=OptionOther&passage=John3%3A19";
const char *xref="passagestudy.jsp?action=showNote&type=x&value=1&module=OptionOther&passage=John3%3A20";
const char *reference="passagestudy.jsp?action=showRef&type=scripRef&value=16-19&module=OptionOther";
}
int main(int argc,char **argv) {
 try {
  check(argc==3,"note/dictionary fixture roots required");
  std::future<StudyHoverLink> surviving;
  {
   StudyWorker worker(argv[1]); const auto first=worker.create_tab().get(),second=worker.create_tab().get();
   worker.enable_module_preferences(first).get(); worker.enable_module_preferences(second).get();
   (void)worker.visit(first,"OptionOther","John3:23").get();
   check(worker.cross_reference_preview_list(first).get() && worker.cross_reference_preview_list(second).get(),"legacy list preference default");
   const auto expected=worker.study_note_link(first,"OptionOther",body).get();
   const auto note=std::get<StudyNote>(worker.hover_study_link_at(first,"OptionOther","John3:23",body).get());
   check(note.html==expected.html && note.reference==expected.reference && note.html.find("Original other note")!=std::string::npos,"hover uses existing exact-source/settings/body renderer");
   const auto list=std::get<ReferenceList>(worker.hover_study_link_at(first,"OptionOther","John3:23",xref).get());
   check(list.entries.size()==5 && list.context=="John 3:23" && list.entries[0].reference=="John 3:16" && list.entries[4].reference=="John 3:16","cross-reference list uses committed context/order/duplicates");
   worker.set_cross_reference_preview_list(first,false).get();
   check(!worker.cross_reference_preview_list(first).get() && worker.cross_reference_preview_list(second).get(),"per-tab preview preference isolation");
   const auto links=std::get<StudyNote>(worker.hover_study_link_at(first,"OptionOther","John3:23",xref).get());
   check(links.type=="crossReference" && links.reference=="John 3:20" && links.html.find("Back to John 3:23")!=std::string::npos && links.html.find("sword:///John%203:16")!=std::string::npos && links.html.find("Original other fixture")==std::string::npos,"information mode contains links and committed Back target, without passage bodies");
   check(std::holds_alternative<ReferenceList>(worker.hover_study_link_at(second,"OptionOther","John3:23",xref).get()),"other tab retains sidebar mode");
   for(const auto *uri:{reference,"sword:///John3:16"}) check(std::holds_alternative<std::monostate>(worker.hover_study_link_at(first,"OptionOther","John3:23",uri).get()),"explicit references/passages do not become hover reading snapshots");
   rejected([&] { (void)worker.hover_study_link_at(first,"OptionOther","",body).get(); });
   rejected([&] { (void)worker.hover_study_link_at(first,"OptionOther",std::string("John3:23\0ignored",16),body).get(); });
   rejected([&] { (void)worker.hover_study_link_at(first,"OptionOther","John3:23","https://example.invalid/passagestudy.jsp?action=showNote&type=n&value=1").get(); });
   rejected([&] { (void)worker.hover_study_link_at(first,"OptionOther","John3:23","passagestudy.jsp?action=showNote&type=x&value=1&module=OptionOther&passage=John3%3A19").get(); });
   check(worker.history(first).get().entries.size()==1 && worker.dictionary_history(first).get().entries.empty(),"all hovers/preferences/errors preserve both histories and queue");
   worker.close_tab(second).get(); rejected([&] { (void)worker.cross_reference_preview_list(second).get(); });
   rejected([&] { (void)worker.hover_study_link_at(second,"OptionOther","John3:23",reference).get(); });
   std::string copied=body; surviving=worker.hover_study_link_at(first,"OptionOther","John3:23",copied); copied.clear();
  }
  check(std::get<StudyNote>(surviving.get()).html.find("Original other note")!=std::string::npos,"copied queued hover result survives shutdown");
  ReferenceList malicious; malicious.context="John<&\"'"; malicious.entries.push_back({"John<&\"'","Unexpected body"});
  const auto escaped=display_reference_links(malicious);
  check(escaped.find("John&lt;&amp;&quot;&#39;")!=std::string::npos && escaped.find("Unexpected body")==std::string::npos && escaped.find("John%3C%26%22%27")!=std::string::npos,"reference labels and encoded URI attributes safely escaped");
  xc_note *held=nullptr;
  {
   xc_worker *worker=nullptr; xc_request *request=nullptr;
   check(xc_worker_open(argv[1],&worker)==XC_OK,"C hover worker");
   check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"C hover tab");
   const auto tab=xc_request_tab(request); int enabled=9;
   check(xc_request_cross_reference_preview_list(request,&enabled)==XC_INVALID_ARGUMENT && enabled==0,"wrong scalar kind clears output"); xc_request_release(request);
   check(xc_worker_enable_module_preferences(worker,tab,&request)==XC_OK && wait(request)==XC_OK,"C configured note defaults"); xc_request_release(request);
   check(xc_worker_cross_reference_preview_list(worker,tab,&request)==XC_OK && wait(request)==XC_OK && xc_request_cross_reference_preview_list(request,&enabled)==XC_OK && enabled==1,"C copied default preference");
   check(xc_request_cross_reference_preview_list(request,&enabled)==XC_OK && enabled==1,"copied scalar can be read again"); xc_request_release(request);
   check(xc_worker_hover_study_link_at(worker,tab,"OptionOther","John3:23",body,&request)==XC_OK && wait(request)==XC_OK,"C body hover");
   check(!xc_request_no_presentation(request) && !xc_request_take_reference_list(request),"body result type isolation");
   held=xc_request_take_note(request); check(held&&!xc_request_take_note(request),"owned note transfers once"); xc_request_release(request);
   check(xc_worker_hover_study_link_at(worker,tab,"OptionOther","John3:23",xref,&request)==XC_OK && wait(request)==XC_OK,"C list hover");
   auto *list=xc_request_take_reference_list(request); check(list&&xc_reference_list_count(list)==5,"C ordered reference list"); xc_reference_list_release(list); xc_request_release(request);
   check(xc_worker_set_cross_reference_preview_list(worker,tab,0,&request)==XC_OK && wait(request)==XC_OK,"C alternate information mode"); xc_request_release(request);
   check(xc_worker_hover_study_link_at(worker,tab,"OptionOther","John3:23",xref,&request)==XC_OK && wait(request)==XC_OK,"C information links hover");
   auto *links=xc_request_take_note(request); check(links&&std::string(xc_note_html(links)).find("Back to John 3:23")!=std::string::npos,"C copied link-only note"); xc_note_release(links); xc_request_release(request);
   check(xc_worker_hover_study_link_at(worker,tab,"OptionOther","John3:23",reference,&request)==XC_OK && wait(request)==XC_OK,"C no-presentation hover");
   check(xc_request_no_presentation(request)&&xc_request_no_presentation(request)&&!xc_request_take_note(request)&&!xc_request_take_link(request),"explicit copied no-op marker has no snapshot"); xc_request_release(request);
   check(xc_worker_set_cross_reference_preview_list(worker,tab,2,&request)==XC_INVALID_ARGUMENT&&!request,"unknown preference flag rejected");
   check(xc_worker_hover_study_link_at(worker,tab,"OptionOther",nullptr,body,&request)==XC_INVALID_ARGUMENT&&!request,"missing source context rejected");
   check(xc_request_cross_reference_preview_list(nullptr,&enabled)==XC_INVALID_ARGUMENT&&enabled==0&&!xc_request_no_presentation(nullptr),"null scalar/marker queries");
   check(xc_worker_hover_study_link_at(worker,tab,"OptionOther","John3:23",body,nullptr)==XC_INVALID_ARGUMENT,"null observer rejected before queue");
   xc_worker_close(worker);
  }
  check(held&&std::string(xc_note_html(held)).find("Original other note")!=std::string::npos,"C hover snapshot survives worker/request close"); xc_note_release(held);
  {
   StudyWorker worker(argv[2]); const auto tab=worker.create_tab().get(); LexiconPreferences names; names.greek="DictionaryOne"; worker.set_lexicon_preferences(tab,names).get();
   const auto value=std::get<LexicalEntry>(worker.hover_study_link_at(tab,"OriginalBible","John3:16","xiphos.url?action=showStrongs&type=Greek&value=caf%C3%A9").get());
   check(value.command.value=="café"&&value.entry.module=="DictionaryOne"&&worker.dictionary_history(tab).get().entries.empty()&&worker.history(tab).get().entries.empty(),"general hover retains raw lexical lookup/no-record semantics");
  }
  std::cout<<"Shared main hover destinations, preference, ownership and failure checks passed\n";
 } catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
