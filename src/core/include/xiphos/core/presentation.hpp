// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "engine_options.hpp"
namespace xiphos { namespace core {
struct ModulePresentationOptions {
  bool commentary_by_chapter = false;
  bool headings = true;
  bool commentary_chapter_available = false; // Copied classification for selective persistence.
};
using ModulePresentationPreferences = std::map<std::string, ModulePresentationOptions>;
// Legacy backend distinguishes RawFiles personal commentary from regular
// commentary even though both have SWORD's Commentaries type.
bool regular_commentary(const char *type, const char *driver);
// main_new_globals: only regular commentary, and any value except exact Off.
ModulePresentationOptions presentation_options_from_preferences(const EngineOptionPreferences &values,
                                                                  bool regular);
}}
