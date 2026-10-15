// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/presentation.hpp"
#include <cstring>
namespace xiphos { namespace core {
bool regular_commentary(const char *type, const char *driver) {
  return type && !std::strcmp(type, "Commentaries") && (!driver || std::strcmp(driver, "RawFiles"));
}
bool note_numbers_from_preferences(const EngineOptionPreferences &values) {
  const auto found = values.find("XrefNoteNumbers");
  return found == values.end() || found->second != "Off";
}
ModulePresentationOptions presentation_options_from_preferences(const EngineOptionPreferences &values, bool regular) {
  const auto found = values.find("Commentary by Chapter");
  ModulePresentationOptions result;
  auto content = [&](const char *name) {
    const auto value = values.find(name);
    if (value == values.end() || value->second.empty()) return ContentPreference::unknown;
    return value->second == "On" ? ContentPreference::on : ContentPreference::off;
  };
  const auto spacing = values.find("Doublespace");
  result.double_space = spacing != values.end() && spacing->second == "On";
  result.image_content = content("Image Content");
  result.respect_font_faces = content("Respect Font Faces");
  const auto heading = values.find("Headings");
  result.headings = heading == values.end() || heading->second != "Off";
  result.commentary_chapter_available = regular;
  result.commentary_by_chapter = regular && (found == values.end() || found->second != "Off");
  return result;
}
}}
