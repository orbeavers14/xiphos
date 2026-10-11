// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reference_list.hpp"
#include <rawtext.h>
#include <versekey.h>
#include <listkey.h>
#include <filesystem>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <iostream>
#include <stdexcept>
using namespace sword;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
// Minimal ownership-compatible list shim: original adapter logic is compiled
// unchanged, without adding GTK/GLib dependencies to native core tests.
struct GList { std::vector<std::string> values; };
GList *g_list_append(GList *list, char *value) {
  std::unique_ptr<char, decltype(&std::free)> owned(value, &std::free);
  std::unique_ptr<GList> created;
  if (!list) { created = std::make_unique<GList>(); list = created.get(); }
  list->values.emplace_back(value ? value : "");
  if (created) return created.release();
  return list;
}
#define TRUE true
#define XI_message(message) ((void)0)
namespace frozen {
struct BackEnd {
  SWModule &module;
  SWModule *get_SWModule(const char *name) { return name && !std::strcmp(name, "ListTest") ? &module : nullptr; }
  GList *parse_verse_list(const char *, const char *, char *);
};
#include "legacy_reference_list.inc"
}
namespace actual {
struct BackEnd {
  SWModule &module;
  SWModule *get_SWModule(const char *name) { return name && !std::strcmp(name, "ListTest") ? &module : nullptr; }
  GList *parse_verse_list(const char *, const char *, char *);
};
#include "legacy_reference_adapter.inc"
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture path required");
    const std::filesystem::path path(argv[1]); std::filesystem::create_directories(path);
    check(RawText::createModule(path.string().c_str()) == 0, "create reference-list module");
    RawText module(path.string().c_str());
    auto *key = dynamic_cast<VerseKey *>(module.getKey()); check(key != nullptr, "verse-key fixture");
    frozen::BackEnd baseline{module}; actual::BackEnd adapter{module};
    unsigned cases = 0;
    for (const auto *canon : {"KJV", "KJVA"}) {
      key->setVersificationSystem(canon);
      for (const auto *context : {"John 3:16", "Genesis 1:1"}) {
        for (const auto *input : {"John 3:16", "John.3.16", "John 3:16-18", "John 3:36-4:2",
                                 "John 3:16; 17; 4:1-2", "John 3:16; John 3:16", "John 3",
                                 "Jude", "16-18;20", "John 3:16; not-a-reference; John 3:17",
                                 "", "not-a-reference", "Gen 1:1-Rev 22:21", "Tob.1.1-Tob.1.3"}) {
          key->setText("John 3:17");
          const std::string before = key->getText();
          const auto beforeAuto = key->isAutoNormalize(), beforeIntros = key->isIntros();
          const auto shared = xiphos::core::parse_reference_list(module, input, context);
          check(before == key->getText() && beforeAuto == key->isAutoNormalize() && beforeIntros == key->isIntros(),
                "shared parsing preserves module key position and flags");
          std::unique_ptr<GList> expected(baseline.parse_verse_list("ListTest", input, const_cast<char *>(context)));
          const auto expectedValues = expected ? expected->values : std::vector<std::string>();
          const std::string expectedPosition = key->getText();
          std::unique_ptr<GList> result(adapter.parse_verse_list("ListTest", input, const_cast<char *>(context)));
          const auto actualValues = result ? result->values : std::vector<std::string>();
          check(shared.references == expectedValues && actualValues == expectedValues && expectedPosition == key->getText(),
                "shared parser and actual legacy adapter match frozen fork behavior");
          if (std::string(input) == "Gen 1:1-Rev 22:21")
            check(shared.limited && shared.references.size() == 2000, "existing per-range guard retained and reported");
          ++cases;
        }
      }
    }
    key->setVersificationSystem("KJVA");
    const auto alternate = xiphos::core::parse_reference_list(module, "Tob.1.1-Tob.1.3", "John 3:16");
    check(alternate.references.size() == 3 && alternate.references[0].find("Tobit") != std::string::npos,
          "module's alternate canon expands deuterocanonical references");
    bool rejected = false;
    try { xiphos::core::parse_reference_list(module, "John3:16", ""); }
    catch (const std::invalid_argument &) { rejected = true; }
    check(rejected, "empty context rejected");
    check(adapter.parse_verse_list("Missing", "John3:16", const_cast<char *>("John3:16")) == nullptr &&
          adapter.parse_verse_list("ListTest", nullptr, const_cast<char *>("John3:16")) == nullptr &&
          adapter.parse_verse_list("ListTest", "John3:16", nullptr) == nullptr, "legacy missing/null input is safe");
    std::cout << "Shared/actual/frozen reference-list checks passed (" << cases << " cases across two canons)\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
