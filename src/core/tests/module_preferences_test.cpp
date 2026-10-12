// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <rawtext.h>
#include <markupfiltmgr.h>
#include "xiphos/core/engine_manager.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
using namespace xiphos::core;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
xc_status wait(xc_request *r) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  xc_status status;
  while ((status = xc_request_poll(r)) == XC_PENDING) {
    check(std::chrono::steady_clock::now() < deadline, "request deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return status;
}
int main(int argc, char **argv) {
  try {
    check(argc == 3, "source/destination fixture roots required");
    const std::filesystem::path source(argv[1]), root(argv[2]);
    std::filesystem::create_directories(root);
    std::filesystem::copy(source, root, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
    std::ifstream input(root / "mods.d/options.conf");
    std::string config((std::istreambuf_iterator<char>(input)), {});
    check(config.find("[OptionTest]") == 0, "known option fixture");
    config.replace(0, 12, "[OptionOther]");
    config.replace(config.find("./data/"), 7, "./other-data/");
    std::filesystem::create_directories(root / "other-data");
    std::filesystem::copy(root / "data", root / "other-data", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
    { std::ofstream output(root / "mods.d/other.conf"); output << config
        << "GlobalOptionFilter=UTF8GreekAccents\nGlobalOptionFilter=OSISScripref\nGlobalOptionFilter=UTF8HebrewPoints\nGlobalOptionFilter=UTF8Cantillation\n"; }
    {
      sword::RawText writer((root / "other-data").string().c_str());
      writer.setKey("John 3:18");
      writer.setEntry("<verse osisID=\"John.3.18\">λόγος"
                      "<note type=\"crossReference\" n=\"x\"><reference osisRef=\"John.3.16\">John 3:16</reference></note></verse>");
      writer.setKey("John 3:19");
      writer.setEntry("<verse osisID=\"John.3.19\">Other module entry."
                      "<note type=\"study\" n=\"b\">Original other note café <hi type=\"italic\">body</hi> "
                      "<w lemma=\"strong:G3056\">word</w></note></verse>");
      writer.setKey("John 3:21");
      writer.setEntry("<verse osisID=\"John.3.21\">שָׁל֑וֹם</verse>");
      writer.setKey("John 3:20");
      writer.setEntry("<verse osisID=\"John.3.20\">Original range-reference fixture."
                      "<note type=\"crossReference\" n=\"r\"><reference osisRef=\"John.3.16-John.3.19\">John 3:16–19</reference>; "
                      "<reference osisRef=\"John.3.16\">John 3:16</reference></note></verse>");
    }
    // A copied SWORD catalog cache predates the new module configuration.
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    const auto path = root.string();
    const auto preference_path = (root / "modops.conf").string();
    const std::string preference_bytes = "; Original fixture, preserve bytes and unknown data.\n[OptionTest]\nStrong's Numbers=On\nWords of Christ in Red=Off\nFootnotes=Off\nHeadings=Off\nCustomValue=café\n[Uninstalled]\nMorphological Tags=On\n";
    { std::ofstream output(preference_path, std::ios::binary); output << preference_bytes; }
    EngineOptions on, off;
    on.words_in_red = on.strongs = on.morphs = on.footnotes = true;
    const auto defaults = engine_options_from_preferences({});
    std::array<std::string, 4> hebrew_output;
    std::array<std::string, 4> language_reference_output;
    {
      EngineManager baseline(path.c_str(), true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
      auto *module = baseline.getModule("OptionOther");
      check(module != nullptr, "load language/reference fixture");
      baseline.setGlobalOption("Footnotes", "On");
      for (unsigned bits = 0; bits < language_reference_output.size(); ++bits) {
        // Direct legacy setters provide the rendered oracle, independent of the shared helper.
        baseline.setGlobalOption("Greek Accents", bits & 1 ? "On" : "Off");
        baseline.setGlobalOption("Cross-references", bits & 2 ? "On" : "Off");
        language_reference_output[bits] = render_entry(*module, "John 3:18");
      }
      for (unsigned bits = 0; bits < hebrew_output.size(); ++bits) {
        baseline.setGlobalOption("Hebrew Vowel Points", bits & 1 ? "On" : "Off");
        baseline.setGlobalOption("Hebrew Cantillation", bits & 2 ? "On" : "Off");
        hebrew_output[bits] = render_entry(*module, "John 3:21");
      }
      check(hebrew_output[3].find("שָׁל֑וֹם") != std::string::npos && hebrew_output[0].find("שׁלום") != std::string::npos &&
            hebrew_output[1] != hebrew_output[3] && hebrew_output[2] != hebrew_output[3], "both Hebrew filters change actual output");
      check(language_reference_output[3].find("λόγος") != std::string::npos &&
            language_reference_output[0].find("λογος") != std::string::npos &&
            language_reference_output[1] != language_reference_output[3], "both filters change actual rendered output");
    }
    {
      Reader reader(path);
      for (unsigned bits = 0; bits < language_reference_output.size(); ++bits) {
        auto options = defaults;
        options.greekaccents = bits & 1; options.scripturerefs = bits & 2;
        check(reader.read("OptionOther", "John 3:18", options).html == language_reference_output[bits],
              "all language/reference combinations match legacy engine calls");
      }
      for (unsigned bits = 0; bits < hebrew_output.size(); ++bits) {
        auto options = defaults;
        options.hebrewpoints = bits & 1; options.hebrewcant = bits & 2;
        check(reader.read("OptionOther", "John 3:21", options).html == hebrew_output[bits],
              "all Hebrew filter combinations match direct legacy calls");
      }
      const auto expected_on = reader.read("OptionTest", "John 3:16", on).html;
      const auto expected_off = reader.read("OptionTest", "John 3:16", off).html;
      const auto expected_defaults = reader.read("OptionTest", "John 3:16", defaults).html;
      StudySession first(reader), second(reader);
      check(!first.module_engine_options("OptionTest"), "existing sessions remain unconfigured");
      bool unconfigured_save = false;
      try { first.save_module_preferences((root / "unconfigured.conf").string()); } catch (const std::invalid_argument &) { unconfigured_save = true; }
      check(unconfigured_save && !std::filesystem::exists(root / "unconfigured.conf"), "unconfigured SDK state never guessed on save");
      first.enable_module_preferences();
      check(first.read_entry("OptionTest", "John 3:16").html == expected_defaults, "legacy defaults opt in");
      check(!second.module_engine_options("OptionTest"), "opt in does not affect second tab");
      first.set_engine_options(off);
      first.set_module_engine_options("OptionTest", on);
      check(first.read_entry("OptionTest", "John 3:16").html == expected_on, "module overrides tab");
      check(first.read_entry("OptionOther", "John 3:16").html == expected_off, "other module retains tab fallback");
      first.visit("OptionTest", "John 3:16"); first.visit("OptionOther", "John 3:17");
      const auto before = first.history().entries().size();
      first.clear_engine_options();
      first.set_module_engine_options("OptionOther", off);
      check(first.read_chapter("OptionTest", "John 3:16").verses.at(15).html == expected_on, "configured module chapter");
      first.read_entry("OptionOther", "John 3:16");
      check(first.history().entries().size() == before, "settings and refresh preserve history");
      check(first.navigate(-1).html == expected_on, "Back restores module settings");
      first.navigate(1);
      check(first.history().entries().at(first.history().cursor()).module == "OptionOther", "Forward restores other module");
      first.clear_module_engine_options("OptionTest");
      check(first.read_entry("OptionTest", "John 3:16").html == expected_defaults, "module clear restores enabled defaults");
      check(first.read_entry("OptionOther", "John 3:16").html == expected_off, "module clear retains other override");
      first.import_module_preferences(preference_path);
      check(first.module_engine_options("OptionTest")->strongs && !first.module_engine_options("OptionTest")->words_in_red, "file settings replace prior overrides");
      check(first.module_engine_options("OptionOther")->footnotes && first.module_engine_options("OptionOther")->words_in_red, "absent module section uses legacy defaults");
      check(first.read_entry("OptionTest", "John 3:16").html.find("3056") != std::string::npos, "import renders settings");
      check(!second.module_engine_options("OptionTest"), "import leaves other tab unconfigured");
      const auto saved_path = (root / "session-saved.conf").string();
      first.save_module_preferences(saved_path);
      second.import_module_preferences(saved_path);
      check(second.module_engine_options("OptionTest")->strongs && !second.module_engine_options("OptionTest")->words_in_red, "session save reloads in a separate tab");
      check(first.history().entries().size() == before, "session save does not change history");
      bool failed_import = false;
      try { first.import_module_preferences(path + "/missing.conf"); } catch (const std::invalid_argument &) { failed_import = true; }
      check(failed_import && first.module_engine_options("OptionTest")->strongs && first.history().entries().size() == before, "failed import preserves options/history");
      failed_import = false;
      try { first.import_module_preferences(path); } catch (const std::invalid_argument &) { failed_import = true; }
      check(failed_import, "directory import rejected");
      bool rejected = false;
      try { first.set_module_engine_options("Missing", on); } catch (const std::out_of_range &) { rejected = true; }
      check(rejected && first.history().entries().size() == before, "unknown module does not mutate history");
    }
    {
      StudyWorker worker(path);
      const auto first = worker.create_tab().get(), second = worker.create_tab().get();
      worker.enable_module_preferences(first).get();
      auto accepted = worker.set_module_engine_options(first, "OptionTest", on);
      on.strongs = false; accepted.get();
      check(worker.module_engine_options(first, "OptionTest").get()->strongs, "queued options copied");
      check(!worker.module_engine_options(second, "OptionTest").get(), "worker tab isolation");
      worker.set_module_engine_options(first, "OptionOther", off).get();
      check(worker.read_entry(first, "OptionTest", "John 3:16").get().html.find("3056") != std::string::npos, "worker module on");
      check(worker.read_entry(first, "OptionOther", "John 3:16").get().html.find("3056") == std::string::npos, "worker other module off");
      worker.import_module_preferences(first, preference_path).get();
      check(worker.module_engine_options(first, "OptionTest").get()->strongs && !worker.module_engine_options(first, "OptionTest").get()->words_in_red, "worker import settings");
      check(!worker.module_engine_options(second, "OptionTest").get(), "worker import leaves other tab alone");
      worker.save_module_preferences(first, (root / "worker-saved.conf").string()).get();
      worker.close_tab(first).get();
      bool rejected = false;
      try { (void)worker.module_engine_options(first, "OptionTest").get(); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "closed settings tab rejected");
      check(!worker.module_engine_options(second, "OptionTest").get(), "queue continues after rejected query");
    }
    {
      xc_worker *worker = nullptr; xc_request *r = nullptr;
      check(xc_worker_open(path.c_str(), &worker) == XC_OK, "C worker opens");
      check(xc_worker_create_tab(worker, &r) == XC_OK && wait(r) == XC_OK, "C tab");
      const auto tab = xc_request_tab(r); xc_request_release(r);
      unsigned flags = 99; int configured = 99;
      check(xc_worker_module_engine_options(worker, tab, "OptionTest", &r) == XC_OK && wait(r) == XC_OK, "C SDK query");
      check(xc_request_engine_options(r, &flags, &configured) == XC_OK && flags == 0 && configured == 0, "C SDK defaults distinguished");
      xc_request_release(r);
      check(xc_worker_enable_module_preferences(worker, tab, &r) == XC_OK && wait(r) == XC_OK, "C opt in");
      check(xc_request_engine_options(r, &flags, &configured) == XC_INVALID_ARGUMENT, "wrong request kind rejected"); xc_request_release(r);
      check(xc_worker_module_engine_options(worker, tab, "OptionTest", &r) == XC_OK && wait(r) == XC_OK, "C legacy defaults query");
      check(xc_request_engine_options(r, &flags, &configured) == XC_OK && configured && (flags & XC_ENGINE_FOOTNOTES) && (flags & XC_ENGINE_WORDS_IN_RED) && (flags & XC_ENGINE_VARIANTS_PRIMARY) && !(flags & XC_ENGINE_STRONGS), "C default policy copied"); xc_request_release(r);
      for (unsigned bit = 0; bit < 18; ++bit) {
        const unsigned mask = 1u << bit;
        check(xc_worker_set_module_engine_options(worker, tab, "OptionTest", mask, &r) == XC_OK && wait(r) == XC_OK, "C named module option"); xc_request_release(r);
        check(xc_worker_module_engine_options(worker, tab, "OptionTest", &r) == XC_OK && wait(r) == XC_OK, "C configured query");
        check(xc_request_engine_options(r, &flags, &configured) == XC_OK && configured && flags == mask, "all 18 flags round trip"); xc_request_release(r);
      }
      check(xc_worker_set_module_engine_options(worker, tab, "OptionTest", 1u << 30, &r) == XC_INVALID_ARGUMENT && !r, "unknown bits rejected before queue");
      check(xc_worker_set_module_engine_options(worker, tab, nullptr, 0, &r) == XC_INVALID_ARGUMENT && !r, "null module rejected");
      check(xc_worker_module_engine_options(nullptr, tab, "OptionTest", &r) == XC_INVALID_ARGUMENT && !r, "null worker rejected");
      check(xc_worker_module_engine_options(worker, tab, "Missing", &r) == XC_OK && wait(r) == XC_MODULE_NOT_FOUND, "unknown module rejected on worker"); xc_request_release(r);
      check(xc_worker_clear_module_engine_options(worker, tab, "OptionTest", &r) == XC_OK && wait(r) == XC_OK, "C clear module"); xc_request_release(r);
      check(xc_worker_module_engine_options(worker, tab, "OptionTest", &r) == XC_OK && wait(r) == XC_OK, "query after error");
      xc_request_release(r);
      check(xc_worker_import_module_preferences(worker, tab, preference_path.c_str(), &r) == XC_OK && wait(r) == XC_OK, "C explicit import"); xc_request_release(r);
      check(xc_worker_module_engine_options(worker, tab, "OptionTest", &r) == XC_OK && wait(r) == XC_OK, "C imported options query");
      check(xc_request_engine_options(r, &flags, &configured) == XC_OK && flags & XC_ENGINE_STRONGS && !(flags & XC_ENGINE_WORDS_IN_RED), "C imported flags"); xc_request_release(r);
      check(xc_worker_import_module_preferences(worker, tab, "", &r) == XC_INVALID_ARGUMENT && !r, "C empty import path rejected");
      check(xc_worker_import_module_preferences(worker, tab, (path + "/missing.conf").c_str(), &r) == XC_OK && wait(r) == XC_INVALID_ARGUMENT, "C failed import status"); xc_request_release(r);
      check(xc_worker_module_engine_options(worker, tab, "OptionTest", &r) == XC_OK && wait(r) == XC_OK, "C query after failed import");
      xc_request_release(r);
      const auto save_path = (root / "c-saved.conf").string();
      check(xc_worker_save_module_preferences(worker, tab, save_path.c_str(), &r) == XC_OK && wait(r) == XC_OK, "C explicit save"); xc_request_release(r);
      check(read_module_engine_preferences(save_path).at("OptionTest").strongs, "C save bytes decoded");
      check(xc_worker_save_module_preferences(worker, tab, "", &r) == XC_INVALID_ARGUMENT && !r, "C empty save path rejected");
      check(xc_worker_module_engine_options(worker, tab, "OptionTest", &r) == XC_OK && wait(r) == XC_OK, "query after save");
      xc_worker_close(worker);
      check(xc_request_engine_options(r, &flags, &configured) == XC_OK && configured && (flags & XC_ENGINE_STRONGS), "copied settings survive worker close");
      check(xc_request_engine_options(r, nullptr, &configured) == XC_INVALID_ARGUMENT && !configured, "null output initialized");
      xc_request_release(r);
    }
    std::ifstream preserved(preference_path, std::ios::binary);
    const std::string after((std::istreambuf_iterator<char>(preserved)), {});
    check(after == preference_bytes && !std::filesystem::exists(root / "missing.conf"), "import never writes source or creates missing files");
    std::cout << "Module-scoped session/worker/C preference checks passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
