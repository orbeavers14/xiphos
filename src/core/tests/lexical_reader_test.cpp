// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
template<class Error, class Action> void rejects(Action action, const char *message) {
  bool rejected = false; try { action(); } catch (const Error &) { rejected = true; }
  check(rejected, message);
}
xc_status wait(xc_request *request) {
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
  xc_status status;
  while ((status=xc_request_poll(request))==XC_PENDING) {
    check(std::chrono::steady_clock::now()<deadline,"C lexical request deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return status;
}
int main(int argc, char **argv) {
 try {
  check(argc == 2, "original dictionary fixture required");
  const std::string root(argv[1]);
  LexiconPreferences preferences;
  check(preferences.greek == "StrongsGreek" && preferences.hebrew == "StrongsHebrew" &&
        preferences.greek_morph_nt == "Robinson" && preferences.greek_morph_ot == "Packard" &&
        preferences.hebrew_morph == "OSHM", "legacy lexicon defaults");
  preferences.greek="DictionaryOne"; preferences.hebrew="DictionaryTwo";
  preferences.greek_morph_nt="DictionaryOne"; preferences.greek_morph_ot="DictionaryTwo";
  preferences.hebrew_morph="DictionaryTwo";
  LexicalEntry retained;
  {
   Reader reader(root); StudySession first(reader), second(reader);
   rejects<std::invalid_argument>([&] { first.read_lexical("OriginalBible", "John3:16",
     "passagestudy.jsp?action=showStrongs&type=Greek&value=alpha", XIPHOS_LEXICON_MAIN); }, "lexicon settings are explicit opt-in");
   first.set_lexicon_preferences(preferences); preferences.greek="DictionaryTwo";
   second.set_lexicon_preferences(preferences);
   check(first.lexicon_preferences()->greek == "DictionaryOne", "session copies lexicons");
   const std::string strongs="passagestudy.jsp?action=showStrongs&type=Greek&value=alpha";
   retained=first.read_lexical("OriginalBible", "John3:16", strongs, XIPHOS_LEXICON_MAIN);
   check(retained.entry.module == "DictionaryOne" && retained.entry.html.find("café") != std::string::npos &&
         retained.command.module == "OriginalBible" && retained.command.value == "alpha" && retained.source_reference == "John3:16",
         "lookup owns dictionary/source/key values");
   check(second.read_lexical("OriginalBible", "John3:16", strongs, XIPHOS_LEXICON_MAIN).entry.module == "DictionaryTwo",
         "lexicon preferences are separate per tab");
   const std::string morph="xiphos.url?action=showMorph&type=Greek&value=alpha";
   check(first.read_lexical("OriginalBible", "John3:16", morph, XIPHOS_LEXICON_MAIN).entry.module == "DictionaryOne" &&
         first.read_lexical("OriginalBible", "Gen1:1", morph, XIPHOS_LEXICON_DIALOG).entry.module == "DictionaryTwo",
         "actual source testament selects configured OT/NT lexicons");
   check(first.read_lexical("Missing", "Gen1:1", morph+"&module=OriginalBible&passage=John3:16", XIPHOS_LEXICON_MAIN).entry.module == "DictionaryTwo",
         "explicit module overrides fallback; query passage does not replace committed source context");
   check(first.read_lexical("OriginalBible", "Gen1:1", "xiphos.url?action=showMorph&type=x-HebMorph&value=alpha", XIPHOS_LEXICON_DIALOG).entry.module == "DictionaryTwo",
         "dialog Hebrew morphology path");
   rejects<std::invalid_argument>([&] { first.read_lexical("OriginalBible", "Gen1:1", "xiphos.url?action=showMorph&type=Hebrew&value=alpha", XIPHOS_LEXICON_MAIN); },
     "main Hebrew recognition remains distinct");
   const std::string custom="xiphos.url?action=showStrongs&type=lemma.fixture&value=alpha";
   check(first.read_lexical("", "", custom, XIPHOS_LEXICON_MAIN).entry.module == "DictionaryTwo", "legacy main custom type fallback needs no verse source");
   rejects<std::invalid_argument>([&] { first.read_lexical("OriginalBible", "John3:16", custom, XIPHOS_LEXICON_DIALOG); }, "dialog custom type not mapped silently");
   rejects<std::out_of_range>([&] { first.read_lexical("Missing", "John3:16", morph, XIPHOS_LEXICON_MAIN); }, "missing Greek source rejected");
   rejects<std::invalid_argument>([&] { first.read_lexical("DictionaryOne", "alpha", morph, XIPHOS_LEXICON_MAIN); }, "non-verse source rejected");
   rejects<std::invalid_argument>([&] { first.read_lexical("OriginalBible", "", morph, XIPHOS_LEXICON_MAIN); }, "empty Greek source reference rejected");
   rejects<std::invalid_argument>([&] { first.read_lexical("OriginalBible", "not a verse", morph, XIPHOS_LEXICON_MAIN); }, "unresolved Greek context rejected");
   rejects<std::invalid_argument>([&] { first.read_lexical("OriginalBible", "John3:16", strongs, 99); }, "invalid lookup origin rejected");
   rejects<std::invalid_argument>([&] { first.read_lexical("OriginalBible", "John3:16", "https://example/xiphos.url?action=showStrongs&value=1", XIPHOS_LEXICON_MAIN); }, "external lookup origin rejected");
   auto direct=lexical_link("xiphos.url?action=showMorph&type=Greek&value=alpha","OriginalBible");
   rejects<std::invalid_argument>([&] { reader.select_lexicon(direct,std::string("John3:16\0Gen1:1",15),XIPHOS_LEXICON_MAIN,preferences); }, "NUL source context rejected before C string truncation");
   direct.type=std::string("Greek\0Hebrew",12);
   rejects<std::invalid_argument>([&] { reader.select_lexicon(direct,"John3:16",XIPHOS_LEXICON_MAIN,preferences); }, "NUL typed command rejected");
   auto bad=preferences; bad.greek=std::string("Broken\0Name",11);
   rejects<std::invalid_argument>([&] { first.set_lexicon_preferences(bad); }, "embedded NUL preference rejected");
   check(first.lexicon_preferences()->greek == "DictionaryOne", "failed preference change preserves prior settings");
   EngineOptions options{}; options.words_in_red=true; first.set_module_engine_options("DictionaryOne",options);
   options.words_in_red=false; second.set_module_engine_options("DictionaryTwo",options);
   check(first.read_lexical("", "", "xiphos.url?action=showStrongs&type=Greek&value=G03056", XIPHOS_LEXICON_MAIN).entry.html.find("wordsOfJesus") != std::string::npos &&
         second.read_lexical("", "", "xiphos.url?action=showStrongs&type=Greek&value=G03056", XIPHOS_LEXICON_MAIN).entry.html.find("wordsOfJesus") == std::string::npos,
         "lookup uses selected dictionary settings, not source Bible settings");
   first.visit("OriginalBible","John3:16"); const auto location=first.history().entries().at(first.history().cursor());
   (void)first.read_lexical("OriginalBible","Gen1:1",morph,XIPHOS_LEXICON_MAIN);
   check(first.history().entries().size()==1 && first.history().entries().at(first.history().cursor()).reference==location.reference && second.history().entries().empty(), "lexical preview preserves Bible history");
   check(reader.read("OriginalBible","17").reference.find("John") != std::string::npos, "testament lookup clones source key without moving module context");
   bad=preferences; bad.greek="Missing"; first.set_lexicon_preferences(bad);
   rejects<std::out_of_range>([&] { first.read_lexical("", "", strongs, XIPHOS_LEXICON_MAIN); }, "missing preferred lexicon rejected by renderer");
   bad.greek="OriginalBible"; first.set_lexicon_preferences(bad);
   rejects<std::invalid_argument>([&] { first.read_lexical("", "", strongs, XIPHOS_LEXICON_MAIN); }, "verse module cannot become lexicon");
  }
  check(retained.entry.html.find("café") != std::string::npos, "lookup snapshot survives reader teardown");
  std::future<LexicalEntry> accepted;
  {
   StudyWorker worker(root); const auto first=worker.create_tab().get(), second=worker.create_tab().get();
   check(!worker.lexicon_preferences(first).get(), "worker tabs start unconfigured");
   preferences.greek="DictionaryOne"; auto configured=worker.set_lexicon_preferences(first,preferences); preferences.greek="DictionaryTwo"; configured.get();
   worker.set_lexicon_preferences(second,preferences).get();
   const std::string strongs="xiphos.url?action=showStrongs&type=Greek&value=alpha";
   check(worker.read_lexical(second,"","",strongs,XIPHOS_LEXICON_MAIN).get().entry.module=="DictionaryTwo", "worker separate tab settings");
   std::string uri=strongs; accepted=worker.read_lexical(first,"OriginalBible","John3:16",uri,XIPHOS_LEXICON_MAIN); uri.clear();
   rejects<std::out_of_range>([&] { (void)worker.read_lexical(first,"Missing","John3:16","xiphos.url?action=showMorph&type=Greek&value=alpha",XIPHOS_LEXICON_MAIN).get(); }, "worker error returned");
   check(worker.history(first).get().entries.empty() && worker.lexicon_preferences(first).get()->greek=="DictionaryOne", "worker queue/preferences/history survive failure");
   worker.close_tab(first).get();
   rejects<std::invalid_argument>([&] { (void)worker.read_lexical(first,"","",strongs,XIPHOS_LEXICON_MAIN).get(); }, "closed lookup tab rejected");
  }
  check(accepted.get().entry.html==retained.entry.html, "accepted copied lookup survives shutdown");
  xc_lexicons *heldNames=nullptr; xc_request *acceptedRequest=nullptr;
  {
   xc_worker *worker=nullptr; xc_request *request=nullptr;
   check(xc_worker_open(root.c_str(),&worker)==XC_OK,"C lexical worker");
   check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"C lexical tab");
   const auto tab=xc_request_tab(request); check(!xc_request_take_lexicons(request) && !xc_request_take_lexical_entry(request),"C tab result type"); xc_request_release(request);
   check(xc_worker_lexicon_preferences(worker,tab,&request)==XC_OK && wait(request)==XC_OK,"C unconfigured preferences query");
   auto *unset=xc_request_take_lexicons(request); check(unset && !xc_lexicons_configured(unset) && !xc_lexicons_greek(unset),"C unconfigured snapshot retained");
   xc_lexicons_release(unset); xc_request_release(request);
   const char *uri="xiphos.url?action=showStrongs&type=Greek&value=alpha";
   check(xc_worker_read_lexical(worker,tab,nullptr,nullptr,uri,XIPHOS_LEXICON_MAIN,&request)==XC_OK && wait(request)==XC_INVALID_ARGUMENT,"C lookup requires configured lexicons"); xc_request_release(request);
   check(xc_worker_enable_lexicon_preferences(worker,tab,&request)==XC_OK && wait(request)==XC_OK,"C enable defaults"); xc_request_release(request);
   check(xc_worker_lexicon_preferences(worker,tab,&request)==XC_OK && wait(request)==XC_OK,"C defaults snapshot");
   auto *defaults=xc_request_take_lexicons(request);
   check(defaults && std::string(xc_lexicons_greek(defaults))=="StrongsGreek" && std::string(xc_lexicons_greek_morph_nt(defaults))=="Robinson" &&
         std::string(xc_lexicons_greek_morph_ot(defaults))=="Packard" && std::string(xc_lexicons_hebrew(defaults))=="StrongsHebrew" &&
         std::string(xc_lexicons_hebrew_morph(defaults))=="OSHM","C legacy defaults come from core"); xc_lexicons_release(defaults); xc_request_release(request);
   std::string name="DictionaryOne"; xc_lexicon_names names{name.c_str(),"", "DictionaryOne","DictionaryTwo",nullptr};
   check(xc_worker_set_lexicon_preferences(worker,tab,&names,&request)==XC_OK,"C copied preference submission"); name.clear();
   check(wait(request)==XC_OK,"C copied preference completion"); xc_request_release(request);
   check(xc_worker_lexicon_preferences(worker,tab,&request)==XC_OK && !xc_request_take_lexicons(request) && wait(request)==XC_OK,"C preferences pending observation");
   heldNames=xc_request_take_lexicons(request);
   check(heldNames && !xc_request_take_lexicons(request) && xc_lexicons_configured(heldNames) && std::string(xc_lexicons_greek(heldNames))=="DictionaryOne" &&
         xc_lexicons_hebrew(heldNames) && !*xc_lexicons_hebrew(heldNames) && !xc_lexicons_hebrew_morph(heldNames),"C nullable/empty/copy/take-once names"); xc_request_release(request);
   check(xc_worker_read_lexical(worker,tab,"OriginalBible","Gen1:1","xiphos.url?action=showMorph&type=Greek&value=alpha&passage=John3:16",XIPHOS_LEXICON_DIALOG,&request)==XC_OK &&
         !xc_request_take_lexical_entry(request) && wait(request)==XC_OK && !xc_request_take_dictionary_entry(request),"C lexical pending/type result");
   auto *lexical=xc_request_take_lexical_entry(request);
   check(lexical && !xc_request_take_lexical_entry(request) && xc_lexical_entry_kind(lexical)==XIPHOS_LEXICON_MORPHOLOGY &&
         std::string(xc_lexical_entry_module(lexical))=="OriginalBible" && std::string(xc_lexical_entry_type(lexical))=="Greek" &&
         std::string(xc_lexical_entry_value(lexical))=="alpha" && std::string(xc_lexical_entry_passage(lexical))=="John3:16" &&
         std::string(xc_lexical_entry_source_reference(lexical))=="Gen1:1" && std::string(xc_lexical_entry_dictionary_module(lexical))=="DictionaryTwo" &&
         !std::string(xc_lexical_entry_dictionary_key(lexical)).empty() && std::string(xc_lexical_entry_dictionary_html(lexical)).find("café")!=std::string::npos,"C lexical owned metadata/dictionary");
   xc_request_release(request); xc_lexical_entry_release(lexical);
   check(xc_worker_set_lexicon_preferences(worker,tab,nullptr,&request)==XC_INVALID_ARGUMENT && !request,"C null names reject");
   check(xc_worker_set_lexicon_preferences(worker,tab,&names,nullptr)==XC_INVALID_ARGUMENT,"C null settings output reject");
   for (const char *bad : std::initializer_list<const char *>{nullptr,""})
    check(xc_worker_read_lexical(worker,tab,nullptr,nullptr,bad,XIPHOS_LEXICON_MAIN,&request)==XC_INVALID_ARGUMENT && !request,"C empty lexical URI reject");
   check(xc_worker_read_lexical(worker,tab,nullptr,nullptr,uri,9,&request)==XC_INVALID_ARGUMENT && !request,"C invalid lexical origin reject");
   check(xc_worker_read_lexical(worker,tab,nullptr,nullptr,uri,XIPHOS_LEXICON_MAIN,nullptr)==XC_INVALID_ARGUMENT,"C null lookup output reject");
   check(xc_worker_read_lexical(worker,tab,"Missing","John3:16","xiphos.url?action=showMorph&type=Greek&value=alpha",XIPHOS_LEXICON_MAIN,&request)==XC_OK && wait(request)==XC_MODULE_NOT_FOUND,"C missing lexical source status"); xc_request_release(request);
   check(xc_worker_history(worker,tab,&request)==XC_OK && wait(request)==XC_OK,"C queue survives lookup errors");
   auto *history=xc_request_take_history(request); check(history && xc_history_count(history)==0,"C lookup leaves history unchanged"); xc_history_release(history); xc_request_release(request);
   check(xc_worker_read_lexical(worker,tab,nullptr,nullptr,uri,XIPHOS_LEXICON_MAIN,&acceptedRequest)==XC_OK,"C accepted lexical request before close");
   check(xc_worker_close_tab(worker,tab,&request)==XC_OK && wait(request)==XC_OK,"C lexical tab close"); xc_request_release(request);
   check(xc_worker_lexicon_preferences(worker,tab,&request)==XC_OK && wait(request)==XC_INVALID_ARGUMENT,"C closed preferences query rejects"); xc_request_release(request);
   xc_worker_close(worker);
  }
  check(wait(acceptedRequest)==XC_OK,"C accepted lexical request after worker close");
  auto *heldLookup=xc_request_take_lexical_entry(acceptedRequest); xc_request_release(acceptedRequest);
  check(heldLookup && std::string(xc_lexical_entry_dictionary_html(heldLookup)).find("café")!=std::string::npos &&
        std::string(xc_lexicons_greek(heldNames))=="DictionaryOne","C names/lookup survive worker and request close");
  xc_lexicons_release(heldNames); xc_lexical_entry_release(heldLookup);
  check(!xc_request_take_lexicons(nullptr) && !xc_request_take_lexical_entry(nullptr) && !xc_lexicons_configured(nullptr) && !xc_lexicons_greek(nullptr) &&
        xc_lexical_entry_kind(nullptr)==-1 && !xc_lexical_entry_dictionary_html(nullptr),"C NULL handles safe");
  xc_lexicons_release(nullptr); xc_lexical_entry_release(nullptr);
  std::cout << "Shared lexical preferences/context/lookup checks passed\n";
 } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
