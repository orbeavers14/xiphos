// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
template<class Action> void invalid(Action action) {
  bool rejected=false;
  try { action(); } catch (const std::invalid_argument &) { rejected=true; }
  check(rejected,"invalid dictionary history operation rejected");
}
xc_status wait(xc_request *request) {
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
  xc_status status;
  while ((status=xc_request_poll(request))==XC_PENDING) {
    check(std::chrono::steady_clock::now()<deadline,"dictionary history request deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return status;
}
int main(int argc,char **argv) {
  try {
    check(argc==2,"original dictionary fixture required");
    const std::string root=argv[1];
    {
      Reader reader(root); StudySession first(reader),second(reader);
      first.visit("OriginalBible","John3:16");
      first.read_dictionary("DictionaryOne","alpha");
      first.navigate_dictionary("DictionaryOne","alpha",1);
      check(first.dictionary_history().entries().empty(),"preview/key step do not record dictionary history");
      first.visit_dictionary("DictionaryOne","alpha");
      first.visit_dictionary("DictionaryTwo","alpha");
      first.visit_dictionary("DictionaryTwo","café");
      const auto back=first.navigate_dictionary_history(-1);
      check(back.module=="DictionaryTwo" && back.html.find("DictionaryTwo")!=std::string::npos,
        "history returns rendered recorded module/key");
      first.visit_dictionary("DictionaryTwo","alpha");
      check(first.dictionary_history().entries().size()==3 && first.dictionary_history().can_forward(),
        "duplicate visit retains forward history");
      invalid([&] { first.visit_dictionary("OriginalBible","John3:16"); });
      invalid([&] { first.visit_dictionary("DictionaryOne",std::string("alpha\0ignored",13)); });
      invalid([&] { first.navigate_dictionary_history(0); });
      check(first.dictionary_history().cursor()==1 && first.dictionary_history().can_forward(),
        "failed visit/navigation preserves dictionary history");
      check(first.navigate_dictionary_history(1).html.find("Unicode")!=std::string::npos,
        "history forward renders stored target");
      second.visit_dictionary("DictionaryOne","G03056");
      check(first.dictionary_history().entries().size()==3 && second.dictionary_history().entries().size()==1 &&
        first.history().entries().size()==1 && second.history().entries().empty(),"separate dictionary/Bible/tab histories");
      LexiconPreferences lexicons; lexicons.greek="DictionaryOne";
      first.set_lexicon_preferences(lexicons);
      const std::string uri="xiphos.url?action=showStrongs&type=Greek&value=alpha";
      (void)first.read_lexical("OriginalBible","John3:16",uri,XIPHOS_LEXICON_MAIN);
      check(first.dictionary_history().entries().size()==3,"lexical preview does not record visit");
      const auto lexical=first.visit_lexical("OriginalBible","John3:16",uri,XIPHOS_LEXICON_MAIN);
      check(lexical.entry.module=="DictionaryOne" && first.dictionary_history().entries().size()==4 &&
        first.dictionary_history().entries().back().key=="alpha","explicit lexical visit records selected dictionary/raw value");
      first.clear_dictionary_history();
      check(first.dictionary_history().entries().empty() && first.history().entries().size()==1,
        "dictionary clear leaves Bible history");
      invalid([&] { first.navigate_dictionary_history(-1); });
    }
    DictionaryHistorySnapshot retained;
    {
      StudyWorker worker(root); const auto tab=worker.create_tab().get();
      std::string key="alpha"; auto submitted=worker.visit_dictionary(tab,"DictionaryOne",key); key.clear();
      check(submitted.get().html.find("DictionaryOne")!=std::string::npos,"worker copies dictionary visit arguments");
      (void)worker.visit_dictionary(tab,"DictionaryOne","G03056").get();
      auto red=engine_options_from_preferences({}); red.words_in_red=true;
      worker.set_module_engine_options(tab,"DictionaryOne",red).get();
      (void)worker.navigate_dictionary_history(tab,-1).get();
      check(worker.navigate_dictionary_history(tab,1).get().html.find("wordsOfJesus")!=std::string::npos,
        "history navigation applies current tab options");
      retained=worker.dictionary_history(tab).get();
      check(retained.entries.size()==2 && retained.entries.front().key=="alpha" && retained.cursor==1,
        "history stores requested key and owns snapshot");
      LexiconPreferences lexicons; lexicons.greek="DictionaryTwo"; worker.set_lexicon_preferences(tab,lexicons).get();
      const std::string uri="xiphos.url?action=showStrongs&type=Greek&value=alpha";
      (void)worker.resolve_study_link_at(tab,"OriginalBible","John3:16",uri).get();
      check(worker.dictionary_history(tab).get().entries.size()==2,"contextual lexical resolution stays unrecorded");
      const auto click=worker.follow_study_link_at(tab,"OriginalBible","John3:16",uri).get();
      check(std::holds_alternative<LexicalEntry>(click) && std::get<LexicalEntry>(click).entry.module=="DictionaryTwo" &&
        worker.dictionary_history(tab).get().entries.size()==3,"contextual click records selected dictionary");
      const auto passage=worker.follow_study_link_at(tab,"OriginalBible","John3:16","sword:///John3:17").get();
      check(std::holds_alternative<PassageLink>(passage) && worker.dictionary_history(tab).get().entries.size()==3 &&
        worker.history(tab).get().entries.empty(),"follow passage target retains no-visit dispatch behavior");
      lexicons.greek="Missing"; worker.set_lexicon_preferences(tab,lexicons).get();
      bool missing=false;
      try { (void)worker.follow_study_link_at(tab,"OriginalBible","John3:16",uri).get(); } catch (const std::out_of_range &) { missing=true; }
      check(missing && worker.dictionary_history(tab).get().entries.size()==3,"failed lexical click retains dictionary history");
      worker.clear_dictionary_history(tab).get();
      check(worker.dictionary_history(tab).get().entries.empty(),"worker dictionary clear");
      worker.close_tab(tab).get(); invalid([&] { (void)worker.dictionary_history(tab).get(); });
    }
    check(retained.entries.front().module=="DictionaryOne","snapshot survives worker teardown");
    xc_dictionary_history *owned=nullptr;
    {
      xc_worker *worker=nullptr; xc_request *request=nullptr;
      check(xc_worker_open(root.c_str(),&worker)==XC_OK,"C history worker");
      check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"C history tab");
      const auto tab=xc_request_tab(request); xc_request_release(request);
      for (const auto *key : {"alpha","café"}) {
        check(xc_worker_visit_dictionary(worker,tab,"DictionaryOne",key,&request)==XC_OK && wait(request)==XC_OK,
          "C recorded dictionary visit");
        auto *entry=xc_request_take_dictionary_entry(request);
        check(entry && !xc_request_take_dictionary_history(request),"visit result is dictionary not history");
        xc_dictionary_entry_release(entry); xc_request_release(request);
      }
      check(xc_worker_dictionary_history(worker,tab,&request)==XC_OK && !xc_request_take_dictionary_history(request) && wait(request)==XC_OK,
        "C dictionary history pending/query");
      owned=xc_request_take_dictionary_history(request);
      check(owned && !xc_request_take_dictionary_history(request) && !xc_request_take_history(request) &&
        xc_dictionary_history_count(owned)==2 && xc_dictionary_history_cursor(owned)==1 &&
        xc_dictionary_history_can_back(owned) && !xc_dictionary_history_can_forward(owned) &&
        std::string(xc_dictionary_history_key(owned,0))=="alpha" && !xc_dictionary_history_key(owned,2) &&
        !xc_dictionary_history_module(nullptr,0) && !xc_dictionary_history_count(nullptr),"C owned history kind/bounds");
      xc_request_release(request);
      check(xc_worker_navigate_dictionary_history(worker,tab,-1,&request)==XC_OK && wait(request)==XC_OK,
        "C dictionary history back");
      xc_request_release(request); // unobserved owned entry released with request
      check(xc_worker_navigate_dictionary_history(worker,tab,0,&request)==XC_INVALID_ARGUMENT && !request,
        "C invalid history direction prequeue");
      const xc_lexicon_names names={"DictionaryTwo",nullptr,nullptr,nullptr,nullptr};
      check(xc_worker_set_lexicon_preferences(worker,tab,&names,&request)==XC_OK && wait(request)==XC_OK,"C recorded lexical preferences");
      xc_request_release(request);
      check(xc_worker_follow_study_link_at(worker,tab,"OriginalBible","John3:16","xiphos.url?action=showStrongs&type=Greek&value=alpha",&request)==XC_OK && wait(request)==XC_OK,"C contextual recorded click");
      auto *lexical=xc_request_take_lexical_entry(request);
      check(lexical && std::string(xc_lexical_entry_dictionary_module(lexical))=="DictionaryTwo","C recorded lexical result kind");
      xc_lexical_entry_release(lexical); xc_request_release(request);
      check(xc_worker_dictionary_history(worker,tab,&request)==XC_OK && wait(request)==XC_OK,"C recorded lexical history");
      auto *updated=xc_request_take_dictionary_history(request);
      check(updated && xc_dictionary_history_count(updated)==2 && !xc_dictionary_history_can_forward(updated) &&
        std::string(xc_dictionary_history_module(updated,1))=="DictionaryTwo","C changed click clears forward dictionary history");
      xc_dictionary_history_release(updated); xc_request_release(request);
      check(xc_worker_clear_dictionary_history(worker,tab,&request)==XC_OK && wait(request)==XC_OK,"C dictionary history clear");
      xc_request_release(request); xc_worker_close(worker);
    }
    check(std::string(xc_dictionary_history_module(owned,0))=="DictionaryOne" &&
      std::string(xc_dictionary_history_key(owned,1))=="café","C dictionary snapshot outlives worker/request");
    xc_dictionary_history_release(owned);
    std::cout << "Shared dictionary history command checks passed\n";
  } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
