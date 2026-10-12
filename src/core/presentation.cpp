// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/presentation.hpp"
#include <cstring>
namespace xiphos { namespace core {
bool regular_commentary(const char *type, const char *driver) {
  return type && !std::strcmp(type, "Commentaries") && (!driver || std::strcmp(driver, "RawFiles"));
}
ModulePresentationOptions presentation_options_from_preferences(const EngineOptionPreferences &values, bool regular) {
  const auto found = values.find("Commentary by Chapter");
  ModulePresentationOptions result;
  result.commentary_by_chapter = regular && (found == values.end() || found->second != "Off");
  return result;
}
}}
