// SPDX-License-Identifier: GPL-2.0-or-later
// Extracted from Xiphos global_ops.cc; Copyright (C) 2000-2026 Xiphos Developer Team
#include "xiphos/core/engine_options.hpp"
#include "xiphos/core/engine_manager.hpp"
#include <swoptfilter.h>
#include <set>
#include <stdexcept>
namespace xiphos { namespace core {
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
