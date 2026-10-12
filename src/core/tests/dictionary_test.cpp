// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
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
int main(int argc, char **argv) {
  try {
    check(argc == 2, "dictionary fixture root required"); const std::filesystem::path root(argv[1]);
    std::filesystem::create_directories(root / "mods.d");
    for (const auto *id : {"DictionaryOne", "DictionaryTwo"}) {
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
    }
    std::filesystem::create_directories(root / "bible");
    check(sword::RawText::createModule((root / "bible").string().c_str()) == 0, "create non-dictionary fixture");
    { sword::RawText writer((root / "bible").string().c_str()); writer.setKey("John3:16"); writer.setEntry("Original Bible fixture."); }
    { std::ofstream config(root / "mods.d/bible.conf"); config << "[OriginalBible]\nDataPath=./bible/\nModDrv=RawText\nSourceType=OSIS\nEncoding=UTF-8\n"; }
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    const auto path = root.string();
    std::map<std::string, DictionaryEntry> expected;
    const std::vector<std::string> keys = {"alpha", "ALPHA", "café", "G03056", "G3056", "between", "zzzz"};
    std::string plainWord, redWord;
    {
      EngineManager legacy(path.c_str(), true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
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
    std::cout << "Shared dictionary key/render/context/lifetime checks passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
