// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/engine_options.hpp"
#include <swconfig.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
using namespace sword;
using gchar = char;
constexpr int FALSE = 0, TRUE = 1;
struct GLOBAL_OPS {
  int strongs;
  int morphs;
  int footnotes;
  int greekaccents;
  int lemmas;
  int scripturerefs;
  int hebrewpoints;
  int hebrewcant;
  int italic_headings;
  int words_in_red;
  int xlit;
  int enumerated;
  int glosses;
  int morphseg;
  int transliteration;
  int variants_primary;
  int variants_secondary;
  int variants_all;
  int headings;
  int xrefnotenumbers;
  int display_chapter_N;
};
struct { const char *gSwordDir; } settings;
#define g_new0(type, count) static_cast<type *>(std::calloc(count, sizeof(type)))
#define g_free std::free
char *g_strdup_printf(const char *format, const char *argument) {
  const int length = std::snprintf(nullptr, 0, format, argument);
  if (length < 0) throw std::runtime_error("Cannot format fixture path");
  auto *result = static_cast<char *>(std::malloc(size_t(length) + 1));
  if (!result) throw std::bad_alloc();
  std::snprintf(result, size_t(length) + 1, format, argument);
  return result;
}
int gui_of2tf(const char *value) { return !std::strcmp(value, "On"); }
#include "legacy_engine_preferences.inc"
#include "legacy_preferences_adapter.inc"
int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("Fixture directory required");
    std::filesystem::create_directories(argv[1]);
    settings.gSwordDir = argv[1];
    const std::string path = std::string(argv[1]) + "/modops.conf";
    const std::vector<std::string> keys = {
"Strong's Numbers","Morphological Tags","Footnotes","Greek Accents","Lemmas","Cross-references","Hebrew Vowel Points","Hebrew Cantillation","Italic Headings","Words of Christ in Red","Transliterated Forms","Enumerations","Glosses","Morpheme Segmentation","Transliteration","Primary Reading","Secondary Reading","All Readings"};
    const std::vector<std::string> values = {"", "On", "Off", "on", "OFF", "Latin", "unexpected"};
    size_t checked = 0;
    auto check = [&](const xiphos::core::EngineOptionPreferences &input) {
      SWConfig config(path.c_str());
      config.getSections().clear();
      for (const auto &item : input) config["OptionTest"][item.first.c_str()] = item.second.c_str();
      config.save();
      // Both original and current adapters reload the actual SWORD config.
      auto *expected = legacy_new_globals("OptionTest");
      auto *actual = main_new_globals("OptionTest");
      SWConfig loaded(path.c_str()); loaded.load();
      xiphos::core::EngineOptionPreferences copied;
      for (const auto &item : loaded["OptionTest"])
        copied.emplace(item.first.c_str(), item.second.c_str());
      const auto core = xiphos::core::engine_options_from_preferences(copied);
      const auto imported = xiphos::core::read_module_engine_preferences(path);
      const auto found = imported.find("OptionTest");
      const auto from_file = found == imported.end()
          ? xiphos::core::engine_options_from_preferences({}) : found->second;
      if (expected->strongs != actual->strongs || bool(expected->strongs) != core.strongs || bool(expected->strongs) != from_file.strongs) throw std::runtime_error("Preference diverged: strongs");
      if (expected->morphs != actual->morphs || bool(expected->morphs) != core.morphs || bool(expected->morphs) != from_file.morphs) throw std::runtime_error("Preference diverged: morphs");
      if (expected->footnotes != actual->footnotes || bool(expected->footnotes) != core.footnotes || bool(expected->footnotes) != from_file.footnotes) throw std::runtime_error("Preference diverged: footnotes");
      if (expected->greekaccents != actual->greekaccents || bool(expected->greekaccents) != core.greekaccents || bool(expected->greekaccents) != from_file.greekaccents) throw std::runtime_error("Preference diverged: greekaccents");
      if (expected->lemmas != actual->lemmas || bool(expected->lemmas) != core.lemmas || bool(expected->lemmas) != from_file.lemmas) throw std::runtime_error("Preference diverged: lemmas");
      if (expected->scripturerefs != actual->scripturerefs || bool(expected->scripturerefs) != core.scripturerefs || bool(expected->scripturerefs) != from_file.scripturerefs) throw std::runtime_error("Preference diverged: scripturerefs");
      if (expected->hebrewpoints != actual->hebrewpoints || bool(expected->hebrewpoints) != core.hebrewpoints || bool(expected->hebrewpoints) != from_file.hebrewpoints) throw std::runtime_error("Preference diverged: hebrewpoints");
      if (expected->hebrewcant != actual->hebrewcant || bool(expected->hebrewcant) != core.hebrewcant || bool(expected->hebrewcant) != from_file.hebrewcant) throw std::runtime_error("Preference diverged: hebrewcant");
      if (expected->italic_headings != actual->italic_headings || bool(expected->italic_headings) != core.italic_headings || bool(expected->italic_headings) != from_file.italic_headings) throw std::runtime_error("Preference diverged: italic_headings");
      if (expected->words_in_red != actual->words_in_red || bool(expected->words_in_red) != core.words_in_red || bool(expected->words_in_red) != from_file.words_in_red) throw std::runtime_error("Preference diverged: words_in_red");
      if (expected->xlit != actual->xlit || bool(expected->xlit) != core.xlit || bool(expected->xlit) != from_file.xlit) throw std::runtime_error("Preference diverged: xlit");
      if (expected->enumerated != actual->enumerated || bool(expected->enumerated) != core.enumerated || bool(expected->enumerated) != from_file.enumerated) throw std::runtime_error("Preference diverged: enumerated");
      if (expected->glosses != actual->glosses || bool(expected->glosses) != core.glosses || bool(expected->glosses) != from_file.glosses) throw std::runtime_error("Preference diverged: glosses");
      if (expected->morphseg != actual->morphseg || bool(expected->morphseg) != core.morphseg || bool(expected->morphseg) != from_file.morphseg) throw std::runtime_error("Preference diverged: morphseg");
      if (expected->transliteration != actual->transliteration || bool(expected->transliteration) != core.transliteration || bool(expected->transliteration) != from_file.transliteration) throw std::runtime_error("Preference diverged: transliteration");
      if (expected->variants_primary != actual->variants_primary || bool(expected->variants_primary) != core.variants_primary || bool(expected->variants_primary) != from_file.variants_primary) throw std::runtime_error("Preference diverged: variants_primary");
      if (expected->variants_secondary != actual->variants_secondary || bool(expected->variants_secondary) != core.variants_secondary || bool(expected->variants_secondary) != from_file.variants_secondary) throw std::runtime_error("Preference diverged: variants_secondary");
      if (expected->variants_all != actual->variants_all || bool(expected->variants_all) != core.variants_all || bool(expected->variants_all) != from_file.variants_all) throw std::runtime_error("Preference diverged: variants_all");
      if (expected->headings != actual->headings) throw std::runtime_error("Display policy changed: headings");
      if (expected->xrefnotenumbers != actual->xrefnotenumbers) throw std::runtime_error("Display policy changed: xrefnotenumbers");
      if (expected->display_chapter_N != actual->display_chapter_N) throw std::runtime_error("Display policy changed: display_chapter_N");
      std::free(expected); std::free(actual); ++checked;
    };
    check({});
    for (const auto &key : keys)
      for (const auto &value : values) check({{key, value}, {"Unknown Setting", "On"}});
    for (unsigned mask = 0; mask < 8; ++mask)
      check({{"Primary Reading", mask & 1 ? "On" : "Off"},
             {"Secondary Reading", mask & 2 ? "On" : "Off"},
             {"All Readings", mask & 4 ? "On" : "Off"}});
    for (unsigned sample = 0; sample < 128; ++sample) {
      xiphos::core::EngineOptionPreferences input;
      for (size_t i = 0; i < keys.size(); ++i)
        input[keys[i]] = values[(sample * 13 + i * 17 + sample / 7) % values.size()];
      input["Headings"] = input["XrefNoteNumbers"] = input["Display Chapter N"] = sample & 1 ? "Off" : "On";
      check(input);
    }
    std::cout << "Legacy/shared preference characterization passed (" << checked << " cases)\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
