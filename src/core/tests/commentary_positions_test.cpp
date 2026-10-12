// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/commentary.hpp"
#include <rawcom.h>
#include <swmgr.h>
#include <swmodule.h>
#include <versekey.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "legacy_commentary_positions.inc"
#include "legacy_commentary_positions_actual.inc"
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture path required");
    const std::filesystem::path root(argv[1]);
    std::filesystem::create_directories(root / "mods.d");
    const auto data = root / "data";
    std::filesystem::create_directories(data);
    check(sword::RawCom::createModule(data.string().c_str()) == 0, "create sparse original commentary");
    {
      sword::RawCom writer(data.string().c_str());
      writer.setKey("John 3:16"); writer.setEntry("Original linked commentary section.");
      sword::VerseKey source("John 3:16");
      writer.setKey("John 3:17"); writer.linkEntry(&source);
      writer.setKey("John 3:19"); writer.setEntry("<q who=\"Jesus\">Original second section.</q><note type=\"study\" n=\"1\">Original commentary note.</note>");
      source.setText("John 3:19"); writer.setKey("John 3:20"); writer.linkEntry(&source);
      writer.setKey("John 4:2"); writer.setEntry("Original next chapter section.");
      writer.setKey("John 21:25"); writer.setEntry("Original last book section.");
      std::ofstream config(root / "mods.d/notes.conf");
      config << "[Notes]\nDataPath=./data/\nModDrv=RawCom\nSourceType=OSIS\nEncoding=UTF-8\nVersification=KJV\nGlobalOptionFilter=OSISFootnotes\nGlobalOptionFilter=OSISRedLetterWords\n";
    }
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    sword::SWMgr manager(root.string().c_str(), true, nullptr, false, false);
    auto *module = manager.getModule("Notes");
    check(module != nullptr, "load commentary");
    auto *key = dynamic_cast<sword::VerseKey *>(module->getKey());
    check(key != nullptr, "verse commentary key");
    for (const auto &reference : {"John 3:16", "John 3:17", "John 3:18", "John 4:1", "John 21:25", "Genesis 1:1"}) {
      module->setKey(reference);
      const auto frozen = frozen_commentary_positions(*module);
      module->setKey(reference);
      const auto actual = actual_commentary_positions(*module);
      module->setKey(reference);
      key = dynamic_cast<sword::VerseKey *>(module->getKey());
      check(key != nullptr, "reacquire replaced SDK key");
      key->setAutoNormalize(false); key->setIntros(false);
      module->setSkipConsecutiveLinks(false);
      const std::string before = module->getKeyText();
      auto *identity = module->getKey();
      const auto extracted = xiphos::core::commentary_positions(*module, reference);
      check(extracted == frozen && extracted == actual, "shared positions match frozen and current legacy frontier");
      check(identity == module->getKey() && before == module->getKeyText() && !module->isSkipConsecutiveLinks() &&
            !key->isAutoNormalize() && !key->isIntros(), "probe restores key identity, flags and skip links");
    }
    const auto positions = xiphos::core::commentary_positions(*module, "John 3:17");
    check(positions == std::vector<std::string>({"John 3:1", "John 3:16", "John 3:19"}),
          "linked verses skipped and sparse positions preserved");
    try { xiphos::core::commentary_positions(*module, ""); throw std::runtime_error("empty reference accepted"); }
    catch (const std::invalid_argument &) {}
    std::cout << "Commentary iterator characterization checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
