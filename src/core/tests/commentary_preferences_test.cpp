// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <rawcom.h>
#include <rawfiles.h>
#include <rawtext.h>
#include <swmgr.h>
#include <swmodule.h>
#include <swconfig.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
using namespace sword;
using namespace xiphos::core;
constexpr auto TEXT_MODS = "Biblical Texts", COMM_MODS = "Commentaries", DICT_MODS = "Lexicons / Dictionaries", BOOK_MODS = "Generic Books";
enum { TEXT_TYPE, COMMENTARY_TYPE, PERCOM_TYPE, DICTIONARY_TYPE, BOOK_TYPE, PRAYERLIST_TYPE };
struct FrozenBackend { SWMgr &manager; SWModule *get_SWModule(const char *name) { return manager.getModule(name); } int module_type(const char *); };
struct CurrentBackend { SWMgr &manager; SWModule *get_SWModule(const char *name) { return manager.getModule(name); } int module_type(const char *); };
#include "legacy_module_type.inc"
#include "legacy_module_type_actual.inc"
#include "legacy_commentary_mode.inc"
#include "legacy_commentary_mode_actual.inc"
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture root required"); const std::filesystem::path root(argv[1]);
    std::filesystem::create_directories(root / "mods.d");
    for (const auto &id : {"Notes", "Personal", "Text"}) {
      const auto data = root / id; std::filesystem::create_directories(data);
      const bool personal = std::string(id) == "Personal", text = std::string(id) == "Text";
      check((personal ? RawFiles::createModule(data.string().c_str()) : text ? RawText::createModule(data.string().c_str())
                                                                        : RawCom::createModule(data.string().c_str())) == 0, "create original module");
      std::ofstream config(root / "mods.d" / (std::string(id) + ".conf"));
      config << '[' << id << "]\nDataPath=./" << id << "/\nModDrv=" << (personal ? "RawFiles" : text ? "RawText" : "RawCom")
             << "\nEncoding=UTF-8\nVersification=KJV\n";
    }
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    Reader reader(root.string()); StudySession session(reader);
    SWMgr baseline(root.string().c_str(), true, nullptr, false, false);
    FrozenBackend frozen{baseline}; CurrentBackend actual{baseline};
    for (const auto &module : reader.modules()) {
      check(frozen.module_type(module.id.c_str()) == actual.module_type(module.id.c_str()), "complete current/frozen module classification agrees");
      check(module.regular_commentary == (frozen.module_type(module.id.c_str()) == COMMENTARY_TYPE), "copied regular commentary metadata matches legacy");
    }
    check(session.module_presentation_options("Notes").commentary_by_chapter &&
          !session.module_presentation_options("Personal").commentary_by_chapter &&
          !session.module_presentation_options("Text").commentary_by_chapter, "exact regular/personal/Bible mode defaults");
    const auto profile = root / "modops.conf";
    for (const auto &value : {"", "On", "Off", "on", "OFF", " Off", "unexpected"}) {
      { std::ofstream file(profile); file << "[Notes]\nCommentary by Chapter=" << value << "\n[Personal]\nCommentary by Chapter=" << value << "\n[Text]\nCommentary by Chapter=" << value << '\n'; }
      SWConfig config(profile.string().c_str()); config.load();
      const auto loaded = read_module_option_values(profile.string());
      for (const auto &module : reader.modules()) {
        const auto expected = frozen_commentary_mode(&frozen, module.id.c_str(), config);
        check(expected == actual_commentary_mode(&actual, module.id.c_str(), config) &&
              expected == presentation_options_from_preferences(loaded.at(module.id), module.regular_commentary).commentary_by_chapter,
              "chapter preference agrees with frozen/current assignment and file semantics");
      }
    }
    { std::ofstream file(profile); file << "[Notes]\nCommentary by Chapter=Off\nFootnotes=Off\nCustom=café\n[Personal]\nCommentary by Chapter=On\n[Uninstalled]\nCommentary by Chapter=Off\nKeep=value\n"; }
    session.import_module_preferences(profile.string());
    check(!session.module_presentation_options("Notes").commentary_by_chapter && !session.module_engine_options("Notes")->footnotes,
          "single import commits engine and presentation settings");
    session.set_commentary_chapter_mode("Notes", true);
    check(session.module_presentation_options("Notes").commentary_by_chapter && session.history().entries().empty(), "mode update does not visit Bible history");
    try { session.set_commentary_chapter_mode("Personal", true); throw std::runtime_error("personal mode enabled"); } catch (const std::invalid_argument &) {}
    try { session.import_module_preferences((root / "missing").string()); throw std::runtime_error("missing import accepted"); } catch (const std::invalid_argument &) {}
    check(session.module_presentation_options("Notes").commentary_by_chapter && !session.module_engine_options("Notes")->footnotes,
          "failed import preserves both settings maps");
    session.save_module_preferences(profile.string());
    SWConfig saved(profile.string().c_str()); saved.load();
    check(saved["Notes"]["Commentary by Chapter"] == "On" && saved["Notes"]["Custom"] == "café" &&
          saved["Personal"]["Commentary by Chapter"] == "On" && saved["Uninstalled"]["Keep"] == "value" &&
          saved["Uninstalled"]["Commentary by Chapter"] == "Off", "combined save preserves nonregular/uninstalled/unknown values");
    StudyWorker worker(root.string()); const auto tab = worker.create_tab().get(), other = worker.create_tab().get();
    worker.set_commentary_chapter_mode(tab, "Notes", false).get();
    check(!worker.module_presentation_options(tab, "Notes").get().commentary_by_chapter &&
          worker.module_presentation_options(other, "Notes").get().commentary_by_chapter, "FIFO mode preference isolation");
    worker.import_module_preferences(tab, profile.string()).get();
    check(worker.module_presentation_options(tab, "Notes").get().commentary_by_chapter, "queued combined profile import");
    xc_worker *bridge = nullptr; xc_request *request = nullptr;
    auto wait = [](xc_request *r) {
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      xc_status status;
      while ((status = xc_request_poll(r)) == XC_PENDING) {
        check(std::chrono::steady_clock::now() < deadline, "presentation request timeout");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      return status;
    };
    check(xc_worker_open(root.string().c_str(), &bridge) == XC_OK, "C worker opened");
    check(xc_worker_modules(bridge, &request) == XC_OK && wait(request) == XC_OK, "C catalog ready");
    check(xc_request_commentary_chapter_mode(request) == -1, "wrong-kind scalar rejected");
    auto *catalog = xc_request_take_catalog(request); xc_request_release(request);
    for (size_t i = 0; i < xc_catalog_count(catalog); ++i)
      check((xc_catalog_is_regular_commentary(catalog, i) != 0) == (std::string(xc_catalog_id(catalog, i)) == "Notes"), "copied C commentary classification");
    xc_catalog_release(catalog);
    check(xc_worker_create_tab(bridge, &request) == XC_OK && wait(request) == XC_OK, "C tab ready");
    const auto ctab = xc_request_tab(request); xc_request_release(request);
    check(xc_worker_module_presentation_options(bridge, ctab, "Notes", &request) == XC_OK &&
          xc_request_commentary_chapter_mode(request) == -1 && wait(request) == XC_OK, "pending scalar requires poll");
    check(xc_request_commentary_chapter_mode(request) == 1 && xc_request_commentary_chapter_mode(request) == 1, "typed ready scalar repeats");
    xc_request_release(request);
    check(xc_worker_set_commentary_chapter_mode(bridge, ctab, "Notes", 2, &request) == XC_INVALID_ARGUMENT && !request, "nonboolean input rejects before queue");
    check(xc_worker_set_commentary_chapter_mode(bridge, ctab, "Personal", 1, &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT,
          "personal commentary mode update rejected asynchronously"); xc_request_release(request);
    check(xc_worker_set_commentary_chapter_mode(bridge, ctab, "Notes", 0, &request) == XC_OK, "mode mutation queued");
    xc_request_release(request); // observation does not cancel the accepted edit
    check(xc_worker_module_presentation_options(bridge, ctab, "Notes", &request) == XC_OK, "FIFO mode snapshot queued");
    xc_worker_close(bridge);
    check(wait(request) == XC_OK && xc_request_commentary_chapter_mode(request) == 0, "mode edit survives dropped observer and worker drain");
    xc_request_release(request);
    check(xc_request_commentary_chapter_mode(nullptr) == -1 && !xc_catalog_is_regular_commentary(nullptr, 0) &&
          !xc_module_is_regular_commentary(nullptr, 0), "null presentation guards");
    std::cout << "Commentary module preference characterization checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
