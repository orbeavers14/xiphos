// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include "xiphos/core/engine_manager.hpp"
#include <markupfiltmgr.h>
#include <rawld.h>
#include <rawtext.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
xc_status wait(xc_request *request) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  xc_status status;
  while ((status = xc_request_poll(request)) == XC_PENDING) {
    check(std::chrono::steady_clock::now() < deadline, "C dictionary request deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return status;
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "dictionary fixture root required"); const std::filesystem::path root(argv[1]);
    std::filesystem::create_directories(root / "mods.d");
    const std::map<std::string,std::string> features{{"DictionaryOne","GreekDef"},{"DictionaryTwo","HebrewDef"},
      {"MorphGreek","GreekParse"},{"MorphHebrew","HebrewParse"},{"GenericDictionary",""},
      {"OriginalDevotional","DailyDevotion"},{"CaseFeature","greekdef"},{"MultipleFeatures","GreekParse"}};
    for (const auto &fixture : features) {
      const auto *id=fixture.first.c_str();
      std::filesystem::create_directories(root / id);
      const auto path = (root / id / "lexicon").string();
      check(sword::RawLD::createModule(path.c_str()) == 0, "create original dictionary");
      sword::RawLD writer(path.c_str());
      writer.setKey("alpha"); writer.setEntry((std::string("<p>Original ") + id + " café <hi type=\"italic\">alpha</hi>.</p>").c_str());
      writer.setKey("café"); writer.setEntry("<p>Original Unicode key definition.</p>");
      writer.setKey("G03056"); writer.setEntry("<q who=\"Jesus\">Original keyed word definition.</q>");
      std::ofstream config(root / "mods.d" / (std::string(id) + ".conf"));
      config << '[' << id << "]\nDataPath=./" << id << "/lexicon\nModDrv=RawLD\nSourceType=OSIS\nEncoding=UTF-8\n"
                "Description=Original dictionary fixture\nLang=en\nGlobalOptionFilter=OSISRedLetterWords\n";
      if (!fixture.second.empty()) config << "Feature=" << fixture.second << '\n';
      if (fixture.first=="MultipleFeatures") config << "Feature=HebrewDef\n";
    }
    std::filesystem::create_directories(root / "bible");
    check(sword::RawText::createModule((root / "bible").string().c_str()) == 0, "create non-dictionary fixture");
    { sword::RawText writer((root / "bible").string().c_str()); writer.setKey("John3:16"); writer.setEntry("Original Bible fixture."); }
    { std::ofstream config(root / "mods.d/bible.conf"); config << "[OriginalBible]\nDataPath=./bible/\nModDrv=RawText\nSourceType=OSIS\nEncoding=UTF-8\nFeature=GreekDef\n"; }
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    const auto path = root.string();
    std::map<std::string, DictionaryEntry> expected;
    const std::vector<std::string> keys = {"alpha", "ALPHA", "café", "G03056", "G3056", "between", "zzzz"};
    std::string plainWord, redWord;
    std::map<std::string,unsigned> expectedRoles;
    std::map<std::string,bool> expectedDictionary;
    {
      EngineManager legacy(path.c_str(), true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
      for (const auto &item : legacy.Modules) {
        auto &m=*item.second; const auto *feature=m.getConfigEntry("Feature");
        const bool dictionary=std::string(m.getType())=="Lexicons / Dictionaries" && (!feature || std::string(feature)!="DailyDevotion");
        unsigned roles=0;
        // The legacy preferences comparator tests exact first Feature equality.
        if (dictionary && feature) {
          if (std::string(feature)=="GreekDef") roles=XIPHOS_LEXICON_GREEK_DEF;
          if (std::string(feature)=="HebrewDef") roles=XIPHOS_LEXICON_HEBREW_DEF;
          if (std::string(feature)=="GreekParse") roles=XIPHOS_LEXICON_GREEK_PARSE;
          if (std::string(feature)=="HebrewParse") roles=XIPHOS_LEXICON_HEBREW_PARSE;
        }
        expectedRoles[m.getName()]=roles; expectedDictionary[m.getName()]=dictionary;
      }
      auto *module = legacy.getModule("DictionaryOne"); check(module != nullptr, "load original dictionary");
      for (const auto &key : keys) {
        // Same setKey/renderText sequence as BackEnd::get_render_text.
        const auto html = render_entry(*module, key.c_str());
        expected.emplace(key, DictionaryEntry{"DictionaryOne", module->getKeyText(), html});
      }
      legacy.setGlobalOption("Words of Christ in Red", "Off"); plainWord = render_entry(*module, "G03056");
      legacy.setGlobalOption("Words of Christ in Red", "On"); redWord = render_entry(*module, "G03056");
      check(plainWord != redWord && redWord.find("wordsOfJesus") != std::string::npos, "dictionary options affect actual output");
    }
    DictionaryEntry owned;
    {
      Reader reader(path);
      for (const auto &m : reader.modules()) check(m.dictionary==expectedDictionary.at(m.id) && m.lexicon_roles==expectedRoles.at(m.id),
        "core dictionary/role metadata equals legacy type and first Feature comparator");
      for (const auto &key : keys) {
        const auto result = reader.read_dictionary("DictionaryOne", key);
        check(result.module == "DictionaryOne" && result.key == expected.at(key).key && result.html == expected.at(key).html,
              "dictionary key/render behavior equals direct legacy SDK calls");
      }
      StudySession first(reader), second(reader);
      auto options = engine_options_from_preferences({}); options.words_in_red = true;
      first.set_module_engine_options("DictionaryOne", options); options.words_in_red = false;
      second.set_module_engine_options("DictionaryOne", options);
      first.visit("OriginalBible", "John3:16");
      check(first.read_dictionary("DictionaryOne", "G03056").html == redWord && second.read_dictionary("DictionaryOne", "G03056").html == plainWord,
            "dictionary output uses each tab's module options");
      check(first.history().entries().size() == 1 && second.history().entries().empty(), "dictionary previews do not visit Bible history");
      check(first.read_dictionary("DictionaryTwo", "alpha").html.find("DictionaryTwo") != std::string::npos, "dictionary module context is explicit");
      bool rejected = false; try { reader.read_dictionary("DictionaryOne", ""); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "empty dictionary key rejected");
      rejected = false; try { reader.read_dictionary("OriginalBible", "John3:16"); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "verse module cannot masquerade as dictionary");
      rejected = false; try { reader.read_dictionary("Missing", "alpha"); } catch (const std::out_of_range &) { rejected = true; }
      check(rejected, "missing dictionary rejected");
      owned = reader.read_dictionary("DictionaryOne", "alpha");
    }
    check(owned.html.find("café") != std::string::npos, "dictionary snapshot owns Unicode content after reader teardown");
    std::future<DictionaryEntry> accepted;
    {
      StudyWorker worker(path); const auto tab = worker.create_tab().get();
      worker.enable_module_preferences(tab).get();
      std::string key = "alpha"; accepted = worker.read_dictionary(tab, "DictionaryOne", key); key.clear();
      bool rejected = false; try { (void)worker.read_dictionary(tab, "Missing", "alpha").get(); } catch (const std::out_of_range &) { rejected = true; }
      check(rejected && worker.history(tab).get().entries.empty(), "dictionary worker error preserves history and queue");
      worker.close_tab(tab).get(); rejected = false;
      try { (void)worker.read_dictionary(tab, "DictionaryOne", "alpha").get(); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "closed dictionary tab rejected");
    }
    check(accepted.get().html == owned.html, "copied dictionary request/result survives worker shutdown");
    {
      xc_reader *reader=nullptr; check(xc_open(path.c_str(),&reader)==XC_OK,"C synchronous dictionary catalog");
      for (std::size_t i=0;i<xc_module_count(reader);++i) {
        const std::string id=xc_module_id(reader,i);
        check((xc_module_is_dictionary(reader,i)!=0)==expectedDictionary.at(id) && xc_module_lexicon_roles(reader,i)==expectedRoles.at(id),"C synchronous dictionary roles");
      }
      check(!xc_module_is_dictionary(reader,xc_module_count(reader)) && !xc_module_lexicon_roles(reader,xc_module_count(reader)) &&
            !xc_module_is_dictionary(nullptr,0) && !xc_module_lexicon_roles(nullptr,0),"C synchronous metadata bounds"); xc_close(reader);
    }
    xc_dictionary_entry *retained = nullptr;
    xc_request *acceptedRequest = nullptr;
    {
      xc_worker *worker = nullptr; xc_request *request = nullptr;
      check(xc_worker_open(path.c_str(), &worker) == XC_OK, "C dictionary worker");
      check(xc_worker_modules(worker,&request)==XC_OK && wait(request)==XC_OK,"C dictionary catalog");
      auto *catalog=xc_request_take_catalog(request); xc_request_release(request);
      for (std::size_t i=0;i<xc_catalog_count(catalog);++i) {
        const std::string id=xc_catalog_id(catalog,i);
        check((xc_catalog_is_dictionary(catalog,i)!=0)==expectedDictionary.at(id) && xc_catalog_lexicon_roles(catalog,i)==expectedRoles.at(id),"C dictionary catalog roles");
      }
      check(!xc_catalog_is_dictionary(catalog,xc_catalog_count(catalog)) && !xc_catalog_lexicon_roles(catalog,xc_catalog_count(catalog)) &&
            !xc_catalog_is_dictionary(nullptr,0) && !xc_catalog_lexicon_roles(nullptr,0),"C catalog metadata bounds"); xc_catalog_release(catalog);
      check(xc_worker_create_tab(worker, &request) == XC_OK && wait(request) == XC_OK, "C dictionary tab");
      const auto tab = xc_request_tab(request);
      check(!xc_request_take_dictionary_entry(request), "C tab result cannot be dictionary");
      xc_request_release(request);
      check(xc_worker_set_module_engine_options(worker, tab, "DictionaryOne", XC_ENGINE_WORDS_IN_RED, &request) == XC_OK &&
            wait(request) == XC_OK, "C dictionary settings"); xc_request_release(request);
      std::string key = "G03056";
      check(xc_worker_read_dictionary(worker, tab, "DictionaryOne", key.c_str(), &request) == XC_OK, "C dictionary submission"); key.clear();
      check(!xc_request_take_dictionary_entry(request) && wait(request) == XC_OK && !xc_request_take_entry(request) &&
            !xc_request_take_note(request), "C dictionary pending/type separation");
      retained = xc_request_take_dictionary_entry(request);
      check(retained && !xc_request_take_dictionary_entry(request) && std::string(xc_dictionary_entry_module(retained)) == "DictionaryOne" &&
            !std::string(xc_dictionary_entry_key(retained)).empty() && std::string(xc_dictionary_entry_html(retained)) == redWord,
            "C dictionary copied arguments, settings and take-once ownership"); xc_request_release(request);
      for (const char *id : std::initializer_list<const char *>{nullptr, ""}) {
        check(xc_worker_read_dictionary(worker, tab, id, "alpha", &request) == XC_INVALID_ARGUMENT && !request, "C invalid dictionary module");
        check(xc_worker_read_dictionary(worker, tab, "DictionaryOne", id, &request) == XC_INVALID_ARGUMENT && !request, "C invalid dictionary key");
      }
      check(xc_worker_read_dictionary(worker, tab, "DictionaryOne", "alpha", nullptr) == XC_INVALID_ARGUMENT, "C missing dictionary output");
      check(xc_worker_read_dictionary(worker, tab, "Missing", "alpha", &request) == XC_OK && wait(request) == XC_MODULE_NOT_FOUND &&
            !xc_request_take_dictionary_entry(request), "C missing dictionary status"); xc_request_release(request);
      check(xc_worker_read_dictionary(worker, tab, "OriginalBible", "John3:16", &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT,
            "C verse module type rejected"); xc_request_release(request);
      check(xc_worker_history(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "C queue continues after dictionary failures");
      auto *history = xc_request_take_history(request); check(history && xc_history_count(history) == 0, "C dictionary preserves history");
      xc_history_release(history); xc_request_release(request);
      check(xc_worker_read_dictionary(worker, tab, "DictionaryOne", "alpha", &acceptedRequest) == XC_OK, "C retained dictionary observer");
      check(xc_worker_close_tab(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "C dictionary close tab"); xc_request_release(request);
      check(xc_worker_read_dictionary(worker, tab, "DictionaryOne", "alpha", &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT,
            "C closed dictionary tab rejected"); xc_request_release(request);
      xc_worker_close(worker);
    }
    check(wait(acceptedRequest) == XC_OK, "C accepted dictionary observed after worker close");
    auto *afterClose = xc_request_take_dictionary_entry(acceptedRequest); xc_request_release(acceptedRequest);
    check(afterClose && std::string(xc_dictionary_entry_html(afterClose)).find("café") != std::string::npos &&
          std::string(xc_dictionary_entry_html(retained)) == redWord, "C dictionary handles survive worker/request teardown");
    xc_dictionary_entry_release(afterClose); xc_dictionary_entry_release(retained);
    check(!xc_request_take_dictionary_entry(nullptr) && !xc_dictionary_entry_module(nullptr) &&
          !xc_dictionary_entry_key(nullptr) && !xc_dictionary_entry_html(nullptr), "C NULL dictionary handles safe");
    xc_dictionary_entry_release(nullptr);
    std::cout << "Shared dictionary key/render/context/lifetime checks passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
