// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/headings.hpp"
#include <rawcom.h>
#include <rawtext.h>
#include <swmgr.h>
#include <swmodule.h>
#include <markupfiltmgr.h>
#include <utf8html.h>
#include <glib.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
using namespace sword;
struct GLOBAL_OPS { bool strongs = false, lemmas = false, morphs = false; };
namespace ModuleCache {
struct CacheVerse {
  std::string html;
  void SetHeader(const char *text) { html = text; }
  void AppendHeader(const char *text) { html += text; }
};
}
struct FrozenHeadingBackend { SWModule *display_mod; char *get_entry_attribute(const char *, const char *, const char *, bool = true); };
struct CurrentHeadingBackend { SWModule *display_mod; char *get_entry_attribute(const char *, const char *, const char *, bool = true); };
const char *block_render(const char *) { throw std::runtime_error("Block rendering outside this collection seam"); }
GString *CleanupContent(GString *text, GLOBAL_OPS *, const char *, bool reset) {
  if (reset) throw std::runtime_error("Header cleanup reset changed");
  return text; // Postprocessing has its own parity work; test collection only.
}
#include "legacy_heading_collection.inc"
#include "legacy_heading_collection_actual.inc"
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture root required"); const std::filesystem::path root(argv[1]);
    std::filesystem::create_directories(root / "mods.d"); std::filesystem::create_directories(root / "data");
    check(RawCom::createModule((root / "data").string().c_str()) == 0, "create original heading fixture");
    {
      RawCom writer((root / "data").string().c_str());
      writer.setKey("John 3:16");
      writer.setEntry("<div subType=\"x-preverse\"><title type=\"section\">Original heading café.</title><title type=\"section\">Second heading.</title></div>Original entry body.");
      writer.setKey("John 3:17"); writer.setEntry("Original entry without headings.");
      writer.setKey("John 3:18"); writer.setEntry("<title type=\"section\">Interverse title.</title><div subType=\"x-preverse\"><title>Later preverse title.</title></div>Original gap fixture.");
      writer.setKey("John 3:19"); writer.setEntry("<div subType=\"x-preverse\"><title>Original heading-only entry.</title></div>");
      std::ofstream config(root / "mods.d/notes.conf");
      config << "[Notes]\nDataPath=./data/\nModDrv=RawCom\nSourceType=OSIS\nEncoding=UTF-8\nGlobalOptionFilter=OSISHeadings\n";
    }
    std::filesystem::create_directories(root / "text");
    check(RawText::createModule((root / "text").string().c_str()) == 0, "create original heading preview Bible");
    {
      RawText writer((root / "text").string().c_str());
      writer.setKey("John 3:16"); writer.setEntry("Original heading preview Bible.");
      writer.setKey("John 3:17"); writer.setEntry("Original second preview Bible entry.");
      std::ofstream config(root / "mods.d/text.conf");
      config << "[Text]\nDataPath=./text/\nModDrv=RawText\nEncoding=UTF-8\n";
    }
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    SWMgr manager(root.string().c_str(), true, new MarkupFilterMgr(FMT_XHTML), false, false);
    manager.setGlobalOption("Headings", "On"); auto *module = manager.getModule("Notes");
    check(module != nullptr, "load heading fixture");
    GLOBAL_OPS options;
    FrozenHeadingBackend frozen{module}; CurrentHeadingBackend actual{module};
    for (const auto &reference : {"John 3:16", "John 3:17", "John 3:18", "John 3:16"}) {
      module->setKey(reference); ModuleCache::CacheVerse old;
      frozen_cache_header(old, *module, &options, &frozen);
      ModuleCache::CacheVerse current;
      actual_cache_header(current, *module, &options, &actual);
      const std::string before = module->getKeyText(); auto *identity = module->getKey();
      const auto copied = xiphos::core::render_preverse_headings(*module);
      std::string joined; for (const auto &heading : copied) joined += heading;
      check(joined == old.html && joined == current.html, "shared collection matches complete frozen/current dataflow with postprocessors disabled");
      check(identity == module->getKey() && before == module->getKeyText(), "heading collection preserves selected key");
      if (std::string(reference) == "John 3:16") {
        check(copied.size() == 1 && joined.find("Original heading") != std::string::npos && joined.find("Second heading") != std::string::npos,
              "sequential original heading fixture collected");
        check(joined.find("Original entry body") == std::string::npos, "headings do not include verse body");
      } else if (std::string(reference) == "John 3:17") check(copied.empty(), "no stale headings after entry navigation");
      else {
        const auto &attributes = module->getEntryAttributes()["Heading"]["Preverse"];
        const auto zero = attributes.find("0"), later = attributes.find("1");
        if ((zero == attributes.end() || !zero->second.length()) && later != attributes.end() && later->second.length())
          check(copied.empty(), "legacy first-empty-index termination preserves a preverse numbering gap");
      }
    }
    std::cout << "Preverse heading collection characterization checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
