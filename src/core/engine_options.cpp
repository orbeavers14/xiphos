// SPDX-License-Identifier: GPL-2.0-or-later
// Extracted from Xiphos global_ops.cc; Copyright (C) 2000-2026 Xiphos Developer Team
#include "xiphos/core/engine_options.hpp"
#include <swmgr.h>
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
} }
