// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/engine_options.hpp"
#include <swconfig.h>
#include <fstream>
#include <sys/stat.h>
#include <stdexcept>
namespace xiphos { namespace core {
EngineOptionPreferences engine_options_to_preferences(const EngineOptions &options) {
  return {
    {"Strong's Numbers", options.strongs ? "On" : "Off"},
    {"Morphological Tags", options.morphs ? "On" : "Off"},
    {"Footnotes", options.footnotes ? "On" : "Off"},
    {"Greek Accents", options.greekaccents ? "On" : "Off"},
    {"Lemmas", options.lemmas ? "On" : "Off"},
    {"Cross-references", options.scripturerefs ? "On" : "Off"},
    {"Hebrew Vowel Points", options.hebrewpoints ? "On" : "Off"},
    {"Hebrew Cantillation", options.hebrewcant ? "On" : "Off"},
    {"Italic Headings", options.italic_headings ? "On" : "Off"},
    {"Words of Christ in Red", options.words_in_red ? "On" : "Off"},
    {"Transliterated Forms", options.xlit ? "On" : "Off"},
    {"Enumerations", options.enumerated ? "On" : "Off"},
    {"Glosses", options.glosses ? "On" : "Off"},
    {"Morpheme Segmentation", options.morphseg ? "On" : "Off"},
    {"Transliteration", options.transliteration ? "On" : "Off"},
    {"Primary Reading", options.variants_primary ? "On" : "Off"},
    {"Secondary Reading", options.variants_secondary ? "On" : "Off"},
    {"All Readings", options.variants_all ? "On" : "Off"}
  };
}
ModuleOptionValues read_module_option_values(const std::string &path) {
  if (path.empty()) throw std::invalid_argument("Preference file path is empty");
#ifdef _WIN32
  struct _stat64 info;
  if (_stat64(path.c_str(), &info) != 0 || (info.st_mode & _S_IFMT) != _S_IFREG)
#else
  struct stat info;
  if (::stat(path.c_str(), &info) != 0 || (info.st_mode & S_IFMT) != S_IFREG)
#endif
    throw std::invalid_argument("Preference path must be an existing regular file");
  // SWConfig silently accepts missing/unreadable files as empty configuration.
  // A deliberate import must report failure instead of resetting to defaults.
  std::ifstream input(path.c_str(), std::ios::binary);
  if (!input) throw std::invalid_argument("Preference file cannot be read");
  // Check read access before allowing SWConfig's permissive empty defaults.
  input.peek();
  if (input.bad()) throw std::invalid_argument("Preference file cannot be read");
  sword::SWConfig configuration(path.c_str());
  configuration.load();
  ModuleOptionValues result;
  for (const auto &section : configuration.getSections()) {
    if (section.first == "_ConfOrder" || section.first == "_ConfComments")
      continue; // Reserved SDK bookkeeping, not module preferences.
    EngineOptionPreferences values;
    for (const auto &item : section.second)
      values.emplace(item.first.c_str(), item.second.c_str());
    result.emplace(section.first.c_str(), std::move(values));
  }
  return result;
}
ModuleEnginePreferences read_module_engine_preferences(const std::string &path) {
  ModuleEnginePreferences result;
  for (const auto &module : read_module_option_values(path))
    result.emplace(module.first, engine_options_from_preferences(module.second));
  return result;
}

} }
