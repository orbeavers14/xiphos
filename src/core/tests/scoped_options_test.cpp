// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <rawtext.h>
#include <markupfiltmgr.h>
#include <swmgr.h>
#include <swoptfilter.h>
#include "xiphos/core/engine_manager.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <array>
#include <stdexcept>
using namespace xiphos::core;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
std::map<std::string, std::string> values(sword::SWMgr &manager) {
  std::map<std::string, std::string> result;
  for (const auto &name : manager.getGlobalOptions()) {
    const auto *value = manager.getGlobalOption(name.c_str());
    if (value) result.emplace(name.c_str(), value);
  }
  return result;
}
std::map<sword::SWOptionFilter *, std::string> filter_values(EngineManager &manager) {
  std::map<sword::SWOptionFilter *, std::string> result;
  for (auto *filter : manager.option_filters()) {
    const auto *value = filter->getOptionValue();
    if (value) result.emplace(filter, value);
  }
  return result;
}
xc_status wait(xc_request *request) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  xc_status status;
  while ((status = xc_request_poll(request)) == XC_PENDING) {
    check(std::chrono::steady_clock::now() < deadline, "C option request timeout");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return status;
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture root required");
    const auto root = std::filesystem::path(argv[1]);
    std::filesystem::create_directories(root / "mods.d");
    std::filesystem::create_directories(root / "data");
    const auto data = (root / "data").string();
    check(sword::RawText::createModule(data.c_str()) == 0, "create option fixture");
    {
      sword::RawText writer(data.c_str());
      writer.setKey("John 3:16");
      writer.setEntry("<verse osisID=\"John.3.16\"><q who=\"Jesus\">Original red-letter fixture.</q> "
                      "<w lemma=\"strong:G3056\" morph=\"robinson:N-NSM\">word</w>"
                      "<note type=\"study\" n=\"a\">Original note.</note></verse>");
      writer.setKey("John 3:17"); writer.setEntry("<verse osisID=\"John.3.17\"><q who=\"Jesus\">Second original fixture.</q></verse>");
      std::ofstream config(root / "mods.d/options.conf");
      config << "[OptionTest]\nDataPath=./data/\nModDrv=RawText\nSourceType=OSIS\nEncoding=UTF-8\n"
                "GlobalOptionFilter=OSISRedLetterWords\nGlobalOptionFilter=OSISStrongs\n"
                "GlobalOptionFilter=OSISMorph\nGlobalOptionFilter=OSISFootnotes\n";
    }
    const std::string path = root.string();
    EngineOptions off, on;
    on.words_in_red = on.strongs = on.morphs = on.footnotes = true;
    std::string expected_off, expected_on, default_text;
    std::array<std::string, 16> expected;
    bool expected_transliteration = false;
    {
      // Only one active engine at a time: unrelated SWMgr concurrency is unproven.
      EngineManager baseline(path.c_str(), true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
      auto *module = baseline.getModule("OptionTest");
      check(module != nullptr, "load option fixture");
      const auto initial = values(baseline);
      expected_transliteration = initial.count("Transliteration") != 0;
      check(initial.count("Words of Christ in Red") && initial.count("Strong's Numbers") &&
            initial.count("Morphological Tags") && initial.count("Footnotes"), "fixture registers all filters");
      default_text = render_entry(*module, "John 3:16");
      // Original policy calls, not the new helper, provide every rendered oracle.
      for (unsigned bits = 0; bits < expected.size(); ++bits) {
        baseline.setGlobalOption("Words of Christ in Red", bits & 1 ? "On" : "Off");
        baseline.setGlobalOption("Strong's Numbers", bits & 2 ? "On" : "Off");
        baseline.setGlobalOption("Morphological Tags", bits & 4 ? "On" : "Off");
        baseline.setGlobalOption("Footnotes", bits & 8 ? "On" : "Off");
        expected[bits] = render_entry(*module, "John 3:16");
      }
      expected_off = expected.front(); expected_on = expected.back();
      check(expected_on != expected_off && expected_on.find("3056") != std::string::npos &&
            expected_off.find("3056") == std::string::npos, "options change actual engine output");
      const auto before = filter_values(baseline);
      try { ScopedEngineOptions scope(baseline, off); throw std::runtime_error("injected render failure"); }
      catch (const std::runtime_error &) {}
      check(filter_values(baseline) == before, "scope restores all filter values after exception");
      {
        ScopedEngineOptions outer(baseline, off);
        const auto outer_values = filter_values(baseline);
        { ScopedEngineOptions inner(baseline, on); }
        check(filter_values(baseline) == outer_values, "nested scopes restore enclosing settings");
      }
      check(filter_values(baseline) == before, "outer scope restores initial settings");
    }
    {
      Reader reader(path);

      for (unsigned bits = 0; bits < expected.size(); ++bits) {
        EngineOptions choices;
        choices.words_in_red = bits & 1; choices.strongs = bits & 2;
        choices.morphs = bits & 4; choices.footnotes = bits & 8;
        check(reader.read("OptionTest", "John 3:16", choices).html == expected[bits], "all 16 rendered filter combinations match oracle");
        check(reader.read("OptionTest", "John 3:16").html == default_text, "every combination restores default output");
      }
      check(reader.read("OptionTest", "John 3:16", off).html == expected_off, "scoped Off rendered oracle");
      check(reader.read("OptionTest", "John 3:16", on).html == expected_on, "scoped On rendered oracle");
      const auto restored_text = reader.read("OptionTest", "John 3:16").html;

      check(restored_text == default_text, "unscoped read unchanged after options");
      const auto chapter = reader.read_chapter("OptionTest", "John 3:16", {}, on);
      check(chapter.verses.at(15).html == expected_on, "chapter uses same scoped engine filters");
      check(reader.read("OptionTest", "John 3:16").html == default_text, "chapter scope restores defaults");
      StudySession first(reader), second(reader);
      first.set_engine_options(on); second.set_engine_options(off);
      check(first.visit("OptionTest", "John 3:16").html == expected_on, "first tab On");
      first.visit("OptionTest", "John 3:17");
      check(second.visit("OptionTest", "John 3:16").html == expected_off, "second tab Off");
      check(first.navigate(-1).html == expected_on, "history navigation uses first tab settings");
      check(second.read_chapter("OptionTest", "John 3:16").verses.at(15).html == expected_off, "second tab chapter settings");
      first.clear_engine_options();
      check(first.visit("OptionTest", "John 3:16").html == default_text, "clear options returns to defaults");
    }
    {
      StudyWorker worker(path);
      const auto first = worker.create_tab().get(), second = worker.create_tab().get();
      auto accepted = worker.set_engine_options(first, on);
      on.words_in_red = on.strongs = on.morphs = on.footnotes = false;
      accepted.get();
      worker.set_engine_options(second, off).get();
      // Enqueue opposite settings before observing either result.
      auto first_read = worker.visit(first, "OptionTest", "John 3:16");
      auto second_read = worker.visit(second, "OptionTest", "John 3:16");
      check(first_read.get().html == expected_on && second_read.get().html == expected_off, "worker copies and isolates tab settings");
      check(worker.read_chapter(first, "OptionTest", "John 3:16").get().verses.at(15).html == expected_on, "worker tab chapter settings");
      const auto count = worker.history(first).get().entries.size();
      worker.clear_engine_options(first).get();
      check(worker.history(first).get().entries.size() == count, "option changes do not add history");
      check(worker.visit(first, "OptionTest", "John 3:16").get().html == default_text, "worker clear restores default output");
      worker.close_tab(first).get();
      bool rejected = false;
      try { worker.set_engine_options(first, off).get(); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "closed tab rejects option updates");
      check(worker.visit(second, "OptionTest", "John 3:16").get().html == expected_off, "queue continues after bad option tab");
    }
    {
      xc_worker *worker = nullptr;
      check(xc_worker_open(path.c_str(), &worker) == XC_OK, "C option worker");
      xc_request *request = nullptr;
      check(xc_worker_modules(worker, &request) == XC_OK && wait(request) == XC_OK, "C module features request");
      auto *catalog = xc_request_take_catalog(request);
      check(catalog && xc_catalog_declared_engine_flags(catalog, 0) ==
            (XC_ENGINE_WORDS_IN_RED | XC_ENGINE_STRONGS | XC_ENGINE_MORPHOLOGY | XC_ENGINE_FOOTNOTES), "C OSIS module feature mask");
      check((xc_catalog_transliteration_available(catalog, 0) != 0) == expected_transliteration, "C SDK transliteration capability matches manager");
      check(!xc_catalog_has_headings(catalog, 0) && !xc_catalog_declared_engine_flags(catalog, 100) &&
            !xc_catalog_transliteration_available(nullptr, 0), "C feature bounds/missing headings");
      xc_request_release(request);
      check(xc_worker_create_tab(worker, &request) == XC_OK && wait(request) == XC_OK, "C first tab");
      const auto first = xc_request_tab(request); xc_request_release(request);
      check(xc_worker_create_tab(worker, &request) == XC_OK && wait(request) == XC_OK, "C second tab");
      const auto second = xc_request_tab(request); xc_request_release(request);
      check(xc_worker_set_engine_options(worker, second, 0, &request) == XC_OK && wait(request) == XC_OK, "C explicit zero options");
      xc_request_release(request);
      for (unsigned bits = 0; bits < expected.size(); ++bits) {
        unsigned flags = (bits & 1 ? XC_ENGINE_WORDS_IN_RED : 0) |
                         (bits & 2 ? XC_ENGINE_STRONGS : 0) |
                         (bits & 4 ? XC_ENGINE_MORPHOLOGY : 0) |
                         (bits & 8 ? XC_ENGINE_FOOTNOTES : 0);
        check(xc_worker_set_engine_options(worker, first, flags, &request) == XC_OK, "C option flags accepted");
        flags = 0; // Caller mutation cannot alter the queued settings.
        check(wait(request) == XC_OK, "C options ready"); xc_request_release(request);
        check(xc_worker_visit(worker, first, "OptionTest", "John 3:16", &request) == XC_OK && wait(request) == XC_OK, "C configured visit");
        auto *entry = xc_request_take_entry(request);
        check(entry && xc_entry_html(entry) == expected[bits], "C all 16 options render equivalently");
        xc_entry_release(entry); xc_request_release(request);
        check(xc_worker_visit(worker, second, "OptionTest", "John 3:16", &request) == XC_OK && wait(request) == XC_OK, "C isolated second visit");
        entry = xc_request_take_entry(request);
        check(entry && xc_entry_html(entry) == expected_off, "C tab settings do not bleed");
        xc_entry_release(entry); xc_request_release(request);
      }
      check(xc_worker_read_tab_entry(worker, first, "OptionTest", "John 3:16", &request) == XC_OK && wait(request) == XC_OK, "C history-free configured refresh");
      auto *refreshed = xc_request_take_entry(request);
      check(refreshed && xc_entry_html(refreshed) == expected_on, "C configured refresh output");
      xc_entry_release(refreshed); xc_request_release(request);
      check(xc_worker_read_tab_entry(worker, first, "OptionTest", "John 3:17", &request) == XC_OK && wait(request) == XC_OK, "C noncurrent snapshot request");
      xc_request_release(request);
      check(xc_worker_history(worker, first, &request) == XC_OK && wait(request) == XC_OK, "C history after refresh");
      auto *unchanged = xc_request_take_history(request);
      check(unchanged && xc_history_count(unchanged) == 1 && !xc_history_can_back(unchanged) && !xc_history_can_forward(unchanged), "C refresh preserves history");
      xc_history_release(unchanged); xc_request_release(request);
      check(xc_worker_read_tab_chapter(worker, first, "OptionTest", "John 3:16", XC_CHAPTER_VERSE_NUMBERS, &request) == XC_OK && wait(request) == XC_OK, "C configured tab chapter");
      auto *chapter = xc_request_take_chapter(request);
      check(chapter && xc_chapter_verse_html(chapter, 15) == expected_on, "C tab chapter output");
      check(xc_request_take_chapter(request) == nullptr, "C tab chapter transfer once");
      xc_request_release(request);
      check(xc_worker_read_chapter(worker, "OptionTest", "John 3:16", 0, &request) == XC_OK && wait(request) == XC_OK, "C unconfigured chapter still available");
      auto *plain = xc_request_take_chapter(request);
      check(plain && xc_chapter_verse_html(plain, 15) == default_text, "C non-tab chapter defaults unchanged");
      xc_chapter_release(plain); xc_request_release(request);
      request = nullptr;
      check(xc_worker_set_engine_options(worker, first, 1u << 30, &request) == XC_INVALID_ARGUMENT && !request, "C unknown engine bits rejected");
      check(xc_worker_read_tab_chapter(worker, first, "OptionTest", "John 3:16", 1u << 30, &request) == XC_INVALID_ARGUMENT && !request, "C unknown display bits rejected");
      check(xc_worker_set_engine_options(worker, first, 0, nullptr) == XC_INVALID_ARGUMENT, "C missing observer rejected");
      check(xc_worker_clear_engine_options(worker, first, &request) == XC_OK, "C clear accepted");
      xc_request_release(request); // Drop observation; accepted clear must execute.
      check(xc_worker_visit(worker, first, "OptionTest", "John 3:16", &request) == XC_OK && wait(request) == XC_OK, "C visit after dropped clear");
      auto *entry = xc_request_take_entry(request);
      check(entry && xc_entry_html(entry) == default_text, "C clear returns to defaults");
      xc_entry_release(entry); xc_request_release(request);
      check(xc_worker_close_tab(worker, first, &request) == XC_OK && wait(request) == XC_OK, "C close configured tab");
      xc_request_release(request);
      check(xc_worker_set_engine_options(worker, first, 0, &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT, "C closed tab failure at poll");
      xc_request_release(request);
      xc_worker_close(worker);
      check(xc_catalog_declared_engine_flags(catalog, 0) & XC_ENGINE_STRONGS, "C feature snapshot survives worker close");
      xc_catalog_release(catalog);
      check(xc_chapter_verse_html(chapter, 15) == expected_on, "C chapter survives worker close");
      xc_chapter_release(chapter);
    }
    std::cout << "Scoped engine output/tab isolation checks passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
