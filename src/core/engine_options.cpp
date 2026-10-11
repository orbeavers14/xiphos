// SPDX-License-Identifier: GPL-2.0-or-later
// Extracted from Xiphos global_ops.cc; Copyright (C) 2000-2026 Xiphos Developer Team
#include "xiphos/core/engine_options.hpp"
#include "xiphos/core/engine_manager.hpp"
#include <swoptfilter.h>
#include <swmodule.h>
#include <set>
#include <stdexcept>
namespace xiphos { namespace core {
EngineOptions engine_options_from_preferences(const EngineOptionPreferences &preferences) {
  auto value = [&](const char *key) -> std::string {
    const auto found = preferences.find(key);
    return found == preferences.end() ? std::string() : found->second;
  };
  EngineOptions o;
  o.strongs = value("Strong's Numbers") == "On";
  o.morphs = value("Morphological Tags") == "On";
  o.lemmas = value("Lemmas") == "On";
  o.transliteration = value("Transliteration") == "On";
  o.italic_headings = value("Italic Headings") == "On";
  o.xlit = value("Transliterated Forms") == "On";
  o.enumerated = value("Enumerations") == "On";
  o.morphseg = value("Morpheme Segmentation") == "On";
  o.variants_all = value("All Readings") == "On";
  o.variants_primary = value("Primary Reading") == "On";
  o.variants_secondary = value("Secondary Reading") == "On";
  o.words_in_red = value("Words of Christ in Red") != "Off";
  o.footnotes = value("Footnotes") != "Off";
  o.scripturerefs = value("Cross-references") != "Off";
  o.greekaccents = value("Greek Accents") != "Off";
  o.hebrewpoints = value("Hebrew Vowel Points") != "Off";
  o.hebrewcant = value("Hebrew Cantillation") != "Off";
  o.glosses = value("Glosses") != "Off";
  if (!o.variants_all && !o.variants_primary && !o.variants_secondary)
    o.variants_primary = true;
  return o;
}
ModuleOptions module_options(const sword::SWModule &module) {
  const auto &config = module.getConfig();
  auto has = [&](const char *filter) { return config.has("GlobalOptionFilter", filter); };
  ModuleOptions result;
  auto &o = result.declared;
  o.words_in_red = has("GBFRedLetterWords") || has("OSISRedLetterWords");
  o.strongs = has("GBFStrongs") || has("ThMLStrongs") || has("OSISStrongs");
  o.morphs = has("GBFMorph") || has("ThMLMorph") || has("OSISMorph");
  o.footnotes = has("GBFFootnotes") || has("ThMLFootnotes") || has("OSISFootnotes");
  o.greekaccents = has("UTF8GreekAccents");
  o.lemmas = has("ThMLLemma") || has("OSISLemma");
  o.scripturerefs = has("ThMLScripref") || has("OSISScripref");
  o.hebrewpoints = has("UTF8HebrewPoints");
  o.hebrewcant = has("UTF8Cantillation");
  result.headings = has("ThMLHeadings") || has("OSISHeadings");
  o.italic_headings = result.headings;
  const bool variants = has("ThMLVariants") || has("OSISVariants");
  o.variants_primary = o.variants_secondary = o.variants_all = variants;
  o.xlit = has("OSISXlit");
  o.enumerated = has("OSISEnum");
  o.glosses = has("OSISGlosses") || has("OSISRuby");
  o.morphseg = has("OSISMorphSegmentation");
  return result;
}
void apply_engine_options(sword::SWMgr &manager, const EngineOptions &options) {
  manager.setGlobalOption("Strong's Numbers", options.strongs ? "On" : "Off");
  manager.setGlobalOption("Morphological Tags", options.morphs ? "On" : "Off");
  manager.setGlobalOption("Footnotes", options.footnotes ? "On" : "Off");
  manager.setGlobalOption("Greek Accents", options.greekaccents ? "On" : "Off");
  manager.setGlobalOption("Lemmas", options.lemmas ? "On" : "Off");
  manager.setGlobalOption("Cross-references", options.scripturerefs ? "On" : "Off");
  manager.setGlobalOption("Hebrew Vowel Points", options.hebrewpoints ? "On" : "Off");
  manager.setGlobalOption("Hebrew Cantillation", options.hebrewcant ? "On" : "Off");
  manager.setGlobalOption("Headings", "On");
  manager.setGlobalOption("Italic Headings", options.italic_headings ? "On" : "Off");
  manager.setGlobalOption("Words of Christ in Red", options.words_in_red ? "On" : "Off");
  manager.setGlobalOption("Transliterated Forms", options.xlit ? "On" : "Off");
  manager.setGlobalOption("Enumerations", options.enumerated ? "On" : "Off");
  manager.setGlobalOption("Glosses", options.glosses ? "On" : "Off");
  manager.setGlobalOption("Morpheme Segmentation", options.morphseg ? "On" : "Off");
  manager.setGlobalOption("Transliteration", options.transliteration ? "Latin" : "Off");
  if (options.variants_primary)
    manager.setGlobalOption("Textual Variants", "Primary Reading");
  else if (options.variants_secondary)
    manager.setGlobalOption("Textual Variants", "Secondary Reading");
  else if (options.variants_all)
    manager.setGlobalOption("Textual Variants", "All Readings");
}
std::vector<sword::SWOptionFilter *> EngineManager::option_filters() {
  std::set<sword::SWOptionFilter *> unique;
  for (const auto &item : optionFilters) if (item.second) unique.insert(item.second);
  // SDKs built without ICU do not initialize the protected transliterator
  // pointer. Read it only when the manager advertises that actual option.
  for (const auto &name : getGlobalOptions())
    if (name == "Transliteration" && transliterator) unique.insert(transliterator);
  return {unique.begin(), unique.end()};
}
ScopedEngineOptions::ScopedEngineOptions(EngineManager &manager, const EngineOptions &options)
    : manager_(manager) {
  // Global getters can return one of several differently initialized filters
  // with the same name. Save each actual filter, including the transliterator.
  for (auto *filter : manager_.option_filters()) {
    const auto *value = filter->getOptionValue();
    if (!value) throw std::runtime_error("Engine option filter has no restorable value");
    saved_.emplace_back(filter, value);
  }
  try { apply_engine_options(manager_, options); }
  catch (...) {
    for (const auto &setting : saved_) setting.first->setOptionValue(setting.second.c_str());
    throw;
  }
}
ScopedEngineOptions::~ScopedEngineOptions() {
  for (const auto &setting : saved_) setting.first->setOptionValue(setting.second.c_str());
}
} }
