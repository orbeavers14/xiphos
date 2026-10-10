// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/engine_options.hpp"
#include "xiphos/core/c_api.h"
#include <rawtext.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace xiphos::core;
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
unsigned flags(const EngineOptions &o) {
  const std::vector<bool EngineOptions::*> fields = {&EngineOptions::strongs, &EngineOptions::morphs,
    &EngineOptions::footnotes, &EngineOptions::greekaccents, &EngineOptions::lemmas,
    &EngineOptions::scripturerefs, &EngineOptions::hebrewpoints, &EngineOptions::hebrewcant,
    &EngineOptions::italic_headings, &EngineOptions::words_in_red, &EngineOptions::xlit,
    &EngineOptions::enumerated, &EngineOptions::glosses, &EngineOptions::morphseg,
    &EngineOptions::transliteration, &EngineOptions::variants_primary,
    &EngineOptions::variants_secondary, &EngineOptions::variants_all};
  unsigned result = 0;
  for (size_t i = 0; i < fields.size(); ++i) if (o.*fields[i]) result |= 1u << i;
  return result;
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture path required");
    const auto root = std::filesystem::path(argv[1]);
    std::filesystem::create_directories(root);
    check(sword::RawText::createModule(root.string().c_str()) == 0, "create metadata fixture");
    sword::ConfigEntMap config; // Outlives the module using this config.
    sword::RawText module(root.string().c_str());
    module.setConfig(&config);
    const auto *key = module.getKey(); const std::string reference = module.getKeyText();
    struct Case { const char *filter; unsigned expected; bool headings; };
    const unsigned variants = XC_ENGINE_VARIANTS_PRIMARY | XC_ENGINE_VARIANTS_SECONDARY | XC_ENGINE_VARIANTS_ALL;
    // Frozen eligibility oracle from c9976a8 menu_popup.c, including aliases
    // used by legacy menus rather than only names available in this local SDK.
    const std::vector<Case> cases = {
      {"GBFRedLetterWords", XC_ENGINE_WORDS_IN_RED, false}, {"OSISRedLetterWords", XC_ENGINE_WORDS_IN_RED, false},
      {"GBFStrongs", XC_ENGINE_STRONGS, false}, {"ThMLStrongs", XC_ENGINE_STRONGS, false}, {"OSISStrongs", XC_ENGINE_STRONGS, false},
      {"GBFMorph", XC_ENGINE_MORPHOLOGY, false}, {"ThMLMorph", XC_ENGINE_MORPHOLOGY, false}, {"OSISMorph", XC_ENGINE_MORPHOLOGY, false},
      {"GBFFootnotes", XC_ENGINE_FOOTNOTES, false}, {"ThMLFootnotes", XC_ENGINE_FOOTNOTES, false}, {"OSISFootnotes", XC_ENGINE_FOOTNOTES, false},
      {"UTF8GreekAccents", XC_ENGINE_GREEK_ACCENTS, false}, {"ThMLLemma", XC_ENGINE_LEMMAS, false}, {"OSISLemma", XC_ENGINE_LEMMAS, false},
      {"ThMLScripref", XC_ENGINE_CROSS_REFERENCES, false}, {"OSISScripref", XC_ENGINE_CROSS_REFERENCES, false},
      {"UTF8HebrewPoints", XC_ENGINE_HEBREW_VOWELS, false}, {"UTF8Cantillation", XC_ENGINE_HEBREW_CANTILLATION, false},
      {"ThMLHeadings", XC_ENGINE_ITALIC_HEADINGS, true}, {"OSISHeadings", XC_ENGINE_ITALIC_HEADINGS, true},
      {"ThMLVariants", variants, false}, {"OSISVariants", variants, false},
      {"OSISXlit", XC_ENGINE_TRANSLITERATED_FORMS, false}, {"OSISEnum", XC_ENGINE_ENUMERATIONS, false},
      {"OSISGlosses", XC_ENGINE_GLOSSES, false}, {"OSISRuby", XC_ENGINE_GLOSSES, false},
      {"OSISMorphSegmentation", XC_ENGINE_MORPHEME_SEGMENTATION, false},
      {"UnknownFilter", 0, false}, {"osisstrongs", 0, false}, {"OSISStrongs ", 0, false}
    };
    check(flags(module_options(module).declared) == 0 && !module_options(module).headings, "missing declarations empty");
    unsigned combined = 0; bool headings = false;
    for (const auto &test : cases) {
      config.clear(); config.insert(std::make_pair(sword::SWBuf("GlobalOptionFilter"), sword::SWBuf(test.filter)));
      auto result = module_options(module);
      check(flags(result.declared) == test.expected && result.headings == test.headings, "legacy alias eligibility");
      config.insert(std::make_pair(sword::SWBuf("GlobalOptionFilter"), sword::SWBuf(test.filter)));
      check(flags(module_options(module).declared) == test.expected, "duplicate declarations do not add features");
      combined |= test.expected; headings = headings || test.headings;
    }
    config.clear();
    for (const auto &test : cases) config.insert(std::make_pair(sword::SWBuf("GlobalOptionFilter"), sword::SWBuf(test.filter)));
    const auto result = module_options(module);
    check(flags(result.declared) == combined && result.headings == headings, "combined declared features");
    check(!result.declared.transliteration, "global transliteration is separate");
    check(module.getKey() == key && reference == module.getKeyText(), "metadata probes preserve passage/key");
    config.clear(); config.insert(std::make_pair(sword::SWBuf("LocalOptionFilter"), sword::SWBuf("OSISStrongs")));
    check(flags(module_options(module).declared) == 0, "only global declarations match legacy checks");
    std::cout << "Module option eligibility passed (30 declaration cases, duplicate/mixed/absent declarations)\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
