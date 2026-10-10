// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include "xiphos/core/reader.hpp"
#include <rawtext.h>
#include <rawld.h>
#include <swmgr.h>
#include <swmodule.h>
#include <markupfiltmgr.h>
#include <versekey.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
extern "C" int c_header_test(void);
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture path required");
    const auto root = std::filesystem::path(argv[1]);
    // Use a generated module with original test prose; no network, proprietary
    // Bible content, or dependence on the developer's installed library.
    std::filesystem::create_directories(root / "mods.d");
    std::filesystem::create_directories(root / "modules/texts/rawtext/test");
    auto data = (root / "modules/texts/rawtext/test").string();
    check(sword::RawText::createModule(data.c_str()) == 0, "create fixture");
    {
      sword::RawText writer(data.c_str());
      writer.setKey("John 3:16");
      writer.setEntry("<verse osisID=\"John.3.16\">Original fixture: café &amp; peace.</verse>");
      writer.setKey("John 3:17");
      writer.setEntry("<verse osisID=\"John.3.17\">Second fixture verse.</verse>");
    }
    {
      std::ofstream config(root / "mods.d/test.conf");
      config << "[ParityTest]\nDataPath=./modules/texts/rawtext/test/\n"
                "ModDrv=RawText\nSourceType=OSIS\nEncoding=UTF-8\n"
                "Lang=en\nDescription=Original parity fixture\nVersification=KJV\n";
    }
    // An alternate versification must drive chapter lengths too; use an
    // isolated root so the primary discovery/adapter fixture stays unchanged.
    const auto alternate = root / "alternate";
    std::filesystem::create_directories(alternate / "mods.d");
    {
      std::ofstream config(alternate / "mods.d/alternate.conf");
      config << "[Alternate]\nDataPath=" << data << "\nModDrv=RawText\n"
                "SourceType=OSIS\nEncoding=UTF-8\nVersification=Vulg\n";
    }
    {
      xiphos::core::Reader reader(alternate.string());
      const auto snapshot = reader.read_chapter("Alternate", "Psalms 9:1");
      sword::SWMgr baseline(alternate.string().c_str(), true,
                            new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
      auto *alt_module = baseline.getModule("Alternate");
      alt_module->setKey("Psalms 9:1");
      auto *key = dynamic_cast<sword::VerseKey *>(alt_module->getKey());
      check(key && std::string(key->getVersificationSystem()) == "Vulg", "alternate versification loaded");
      check(snapshot.verses.size() == static_cast<size_t>(key->getVerseMax()), "alternate chapter length");
      sword::VerseKey kjv;
      kjv.setText("Psalms 9:1");
      check(key->getVerseMax() != kjv.getVerseMax(), "fixture exercises different verse counts");
    }
    check(c_header_test(), "C header and symbols");
    xc_reader *r = nullptr;
    check(xc_open(root.string().c_str(), &r) == XC_OK && r, "open fixture");
    check(xc_module_count(r) == 1, "explicit root must not augment home modules");
    check(std::strcmp(xc_module_id(r, 0), "ParityTest") == 0, "module id");
    check(std::strcmp(xc_module_language(r, 0), "en") == 0, "language");
    check(std::strcmp(xc_module_type(r, 0), "Biblical Texts") == 0, "type");
    check(xc_module_description(r, 1) == nullptr, "bounds");
    check(xc_module_count(nullptr) == 0 && xc_module_id(nullptr, 0) == nullptr, "null reader");
    xc_entry *entry = nullptr;
    check(xc_read(r, "ParityTest", "John 3:16", &entry) == XC_OK, "read verse");
    const std::string actual = xc_entry_html(entry);
    check(actual.find("café") != std::string::npos && actual.find("peace") != std::string::npos, "UTF-8 text");
    check(actual.find("<verse") == std::string::npos, "OSIS rendered to XHTML");
    check(std::string(xc_entry_reference(entry)).find("3:16") != std::string::npos, "canonical reference");
    // Compare against the old backend's exact setKey/renderText sequence.
    sword::SWMgr legacy(root.string().c_str(), true,
                        new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
    auto *module = legacy.getModule("ParityTest");
    check(module != nullptr, "legacy module");
    module->setKey("John 3:16");
    check(actual == module->renderText().c_str(), "legacy rendering equivalence");
    check(xiphos::core::render_entry(*module, "John 3:17").find("Second fixture") != std::string::npos, "shared legacy seam");
    check(std::string(module->getKeyText()).find("3:17") != std::string::npos, "legacy key mutation preserved");
    xc_entry *second = nullptr;
    check(xc_read(r, "ParityTest", "John 3:17", &second) == XC_OK, "second read");
    check(std::string(xc_entry_html(entry)) == actual, "result survives later read");
    xc_entry_release(second);
    check(xc_read(r, "Missing", "John 3:16", &second) == XC_MODULE_NOT_FOUND && !second, "missing module");
    check(xc_read(r, "ParityTest", "", &second) == XC_INVALID_ARGUMENT && !second, "empty reference");
    check(xc_read(nullptr, "ParityTest", "John 3:16", &second) == XC_INVALID_ARGUMENT, "null read");
    check(xc_read(r, "ParityTest", "John 3:16", nullptr) == XC_INVALID_ARGUMENT, "null out");
    xc_chapter *chapter = nullptr;
    check(xc_read_chapter(r, "ParityTest", "John 3:16", &chapter) == XC_OK, "read chapter");
    module->setKey("John 3:16");
    auto *legacy_key = dynamic_cast<sword::VerseKey *>(module->getKey());
    check(legacy_key && xc_chapter_count(chapter) == static_cast<size_t>(legacy_key->getVerseMax()), "engine chapter length");
    check(std::string(xc_chapter_title(chapter)) == "John 3", "chapter title");
    for (size_t i = 0; i < xc_chapter_count(chapter); ++i) {
      legacy_key->setVerse(static_cast<int>(i + 1));
      check(std::string(xc_chapter_verse_reference(chapter, i)) == module->getKeyText(), "chapter ordered references");
      check(std::string(xc_chapter_verse_html(chapter, i)) == module->renderText().c_str(), "every verse matches legacy rendering");
    }
    check(std::string(xc_chapter_verse_html(chapter, 15)) == actual, "chapter anchor text");
    check(std::string(xc_chapter_verse_html(chapter, 16)).find("Second fixture") != std::string::npos, "chapter following text");
    check(std::string(xc_chapter_verse_html(chapter, 0)).empty(), "missing verses retained");
    check(xc_chapter_verse_html(chapter, xc_chapter_count(chapter)) == nullptr, "chapter bounds");
    xc_chapter *failed = nullptr;
    check(xc_read_chapter(r, "Missing", "John 3:16", &failed) == XC_MODULE_NOT_FOUND && !failed, "missing chapter module");
    check(xc_read_chapter(r, "ParityTest", "", &failed) == XC_INVALID_ARGUMENT && !failed, "empty chapter reference");
    check(xc_read_chapter(nullptr, "ParityTest", "John 3", &failed) == XC_INVALID_ARGUMENT, "null chapter reader");
    check(xc_read_chapter(r, "ParityTest", "John 3", nullptr) == XC_INVALID_ARGUMENT, "null chapter output");
    xc_session *chapters = nullptr;
    check(xc_session_open(r, &chapters) == XC_OK, "chapter navigation session");
    check(xc_session_navigate_chapter(chapters, "ParityTest", "John 21:25", 1, &second) == XC_OK, "next book chapter");
    check(std::string(xc_entry_reference(second)) == "Acts 1:1", "next book verse one");
    xc_entry_release(second);
    check(xc_session_navigate_chapter(chapters, "ParityTest", "John 1:10", -1, &second) == XC_OK, "previous book chapter");
    check(std::string(xc_entry_reference(second)) == "Luke 24:1", "previous book final chapter");
    xc_entry_release(second);
    check(xc_session_navigate_chapter(chapters, "ParityTest", "Genesis 1:1", -1, &second) == XC_INVALID_ARGUMENT && !second, "Bible start does not wrap");
    check(xc_session_navigate_chapter(chapters, "ParityTest", "Revelation 22:1", 1, &second) == XC_INVALID_ARGUMENT && !second, "Bible end does not wrap");
    check(xc_session_navigate_chapter(chapters, "ParityTest", "John 3:16", 0, &second) == XC_INVALID_ARGUMENT && !second, "invalid chapter direction");
    check(xc_session_history_count(chapters) == 2, "failed chapter navigation preserves history");
    check(xc_session_navigate(chapters, -1, &second) == XC_OK && std::string(xc_entry_reference(second)) == "Acts 1:1", "chapter moves participate in history");
    xc_entry_release(second);
    xc_session_close(chapters);
    xc_session *tab = nullptr, *other = nullptr;
    check(xc_session_open(r, &tab) == XC_OK, "open session");
    check(xc_session_open(r, &other) == XC_OK, "open independent session");
    check(xc_session_history_count(tab) == 0, "new history empty");
    check(xc_session_navigate(tab, -1, &second) == XC_INVALID_ARGUMENT && !second, "empty back");
    check(xc_session_visit(tab, "ParityTest", "John 3:16", &second) == XC_OK, "session first");
    xc_entry_release(second);
    check(xc_session_visit(tab, "ParityTest", "John 3:17", &second) == XC_OK, "session second");
    xc_entry_release(second);
    check(xc_session_can_back(tab) && !xc_session_can_forward(tab), "history buttons");
    check(xc_session_history_count(other) == 0 && !xc_session_can_back(other), "tab isolation");
    check(xc_session_visit(tab, "Missing", "John 3:16", &second) == XC_MODULE_NOT_FOUND && !second, "failed visit");
    check(xc_session_history_count(tab) == 2 && xc_session_can_back(tab), "failed visit unchanged");
    check(xc_session_navigate(tab, 0, &second) == XC_INVALID_ARGUMENT && !second, "invalid direction");
    check(xc_session_navigate(tab, -1, &second) == XC_OK, "session back");
    check(std::string(xc_entry_html(second)) == actual, "back restores first verse");
    xc_entry_release(second);
    check(!xc_session_can_back(tab) && xc_session_can_forward(tab), "back cursor");
    check(xc_session_visit(tab, "ParityTest", "John 3:16", &second) == XC_OK, "duplicate visit");
    xc_entry_release(second);
    check(xc_session_history_count(tab) == 2 && xc_session_can_forward(tab), "duplicate keeps forward");
    xc_close(r);
    check(xc_session_navigate(tab, 1, &second) == XC_OK, "session survives reader close");
    check(std::string(xc_entry_html(second)).find("Second fixture") != std::string::npos, "forward text");
    xc_session_clear_history(tab);
    check(!xc_session_can_back(tab) && !xc_session_can_forward(tab) && xc_session_history_count(tab) == 0, "clear history");
    xc_session_close(tab);
    xc_session_close(other);
    check(std::string(xc_entry_html(second)).find("Second fixture") != std::string::npos, "result survives session close");
    xc_entry_release(second);
    xc_session_close(nullptr);
    check(xc_session_open(nullptr, &tab) == XC_INVALID_ARGUMENT && !tab, "null session reader");
    check(xc_session_history_count(nullptr) == 0 && xc_session_module(nullptr) == nullptr, "null session snapshot");
    check(std::string(xc_chapter_verse_html(chapter, 15)) == actual, "chapter survives reader close");
    xc_chapter_release(chapter);
    xc_chapter_release(nullptr);
    check(xc_chapter_count(nullptr) == 0 && xc_chapter_reference(nullptr) == nullptr, "null chapter snapshot");
    const auto dictroot = root / "dictionary";
    std::filesystem::create_directories(dictroot / "mods.d");
    std::filesystem::create_directories(dictroot / "data");
    check(sword::RawLD::createModule((dictroot / "data").string().c_str()) == 0, "create dictionary");
    {
      std::ofstream config(dictroot / "mods.d/dict.conf");
      config << "[DictionaryTest]\nDataPath=./data/\nModDrv=RawLD\nDescription=Test dictionary\n";
    }
    check(xc_open(dictroot.string().c_str(), &r) == XC_OK, "open dictionary");
    check(xc_module_count(r) == 1, "dictionary discovered");
    check(xc_read(r, "DictionaryTest", "peace", &second) == XC_INVALID_ARGUMENT && !second,
          "unsupported key type returns error");
    check(xc_read_chapter(r, "DictionaryTest", "peace", &failed) == XC_INVALID_ARGUMENT && !failed, "dictionary chapter rejected");
    xc_close(r);
    check(std::string(xc_entry_html(entry)) == actual, "result survives reader close");
    xc_entry_release(entry);
    xc_entry_release(nullptr);
    xc_close(nullptr);
    check(xc_open((root / "missing").string().c_str(), &r) == XC_INVALID_ARGUMENT && !r, "bad root");
    std::filesystem::create_directories(root / "empty/mods.d");
    check(xc_open((root / "empty").string().c_str(), &r) == XC_OK, "empty library opens");
    check(xc_module_count(r) == 0, "empty library");
    xc_close(r);
    std::cout << "Core/C ABI/legacy rendering checks passed\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
