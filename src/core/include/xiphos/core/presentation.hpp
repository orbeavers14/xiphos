// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "engine_options.hpp"
namespace xiphos { namespace core {
enum class ContentPreference : int { unknown = -1, off = 0, on = 1 };
struct ModulePresentationOptions {
  bool commentary_by_chapter = false;
  bool headings = true;
  ContentPreference image_content = ContentPreference::unknown;
  ContentPreference respect_font_faces = ContentPreference::unknown;
  bool commentary_chapter_available = false; // Copied classification for selective persistence.
};
using ModulePresentationPreferences = std::map<std::string, ModulePresentationOptions>;
// Legacy backend distinguishes RawFiles personal commentary from regular
// commentary even though both have SWORD's Commentaries type.
bool regular_commentary(const char *type, const char *driver);
// main_new_globals: chapter mode only for regular commentary; Headings/chapter
// default on except exact Off. Content fields default unknown for missing/empty,
// otherwise on only for exact On. Unknown does not overwrite saved declarations.
ModulePresentationOptions presentation_options_from_preferences(const EngineOptionPreferences &values,
                                                                  bool regular);
}}
