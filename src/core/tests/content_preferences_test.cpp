// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/preferences.hpp"
#include <rawtext.h>
#include <swconfig.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace sword;
using namespace xiphos::core;
using gchar = char;
constexpr int FALSE = 0, TRUE = 1, COMMENTARY_TYPE = 1;
struct GLOBAL_OPS {
  int strongs, morphs, lemmas, transliteration, italic_headings, xlit, enumerated, morphseg;
  int variants_all, variants_primary, variants_secondary, words_in_red, headings, footnotes;
  int scripturerefs, xrefnotenumbers, display_chapter_N, greekaccents, hebrewpoints, hebrewcant;
  int glosses, verse_per_line, image_content, respect_font_faces, commentary_by_chapter, doublespace;
};
struct { const char *gSwordDir; } settings;
struct Backend { int type = COMMENTARY_TYPE; int module_type(const char *) { return type; } } backend_value;
Backend *backend = &backend_value;
int main_check_for_option(const char *, const char *, const char *) { return FALSE; }
#define g_new0(type, count) static_cast<type *>(std::calloc(count, sizeof(type)))
#define g_free std::free
char *g_strdup_printf(const char *format, const char *argument) {
  const int size = std::snprintf(nullptr, 0, format, argument);
  if (size < 0) throw std::runtime_error("Cannot format generated profile path");
  auto *result = static_cast<char *>(std::malloc(size_t(size) + 1));
  if (!result) throw std::bad_alloc();
  std::snprintf(result, size_t(size) + 1, format, argument); return result;
}
int gui_of2tf(const char *value) { return !std::strcmp(value, "On"); }
#include "legacy_content_preferences.inc"
#include "legacy_content_preferences_actual.inc"
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char **argv) {
  try {
    check(argc == 2, "generated content profile directory required");
    const std::filesystem::path root(argv[1]); std::filesystem::create_directories(root / "mods.d");
    const auto directory = root.string(); settings.gSwordDir = directory.c_str();
    const auto profile = root / "modops.conf";
    const std::vector<std::string> values = {"<missing>", "", "On", "Off", "on", "OFF", "unexpected", "café", " On ", " Off "};
    unsigned comparisons = 0;
    for (const auto &image : values) for (const auto &font : values) for (bool regular : {false, true}) {
      { std::ofstream file(profile); file << "[Text]\n";
        if (image != "<missing>") file << "Image Content=" << image << '\n';
        if (font != "<missing>") file << "Respect Font Faces=" << font << '\n';
        file << "Headings=Off\nStrong's Numbers=On\nCommentary by Chapter=Off\n";
      }
      backend_value.type = regular ? COMMENTARY_TYPE : 0;
      auto *old = frozen_content_globals("Text"), *current = current_content_globals("Text");
      const auto loaded = read_module_option_values(profile.string());
      const auto options = presentation_options_from_preferences(loaded.at("Text"), regular);
      check(old->image_content == current->image_content && old->image_content == static_cast<int>(options.image_content) &&
            old->respect_font_faces == current->respect_font_faces && old->respect_font_faces == static_cast<int>(options.respect_font_faces),
            "content decoding agrees with complete frozen/current initializers");
      check(old->headings == current->headings && old->headings == options.headings && old->strongs == current->strongs,
            "adapted initializer retains adjacent rendering preferences");
      std::free(old); std::free(current); ++comparisons;
    }
    std::filesystem::create_directories(root / "data");
    check(RawText::createModule((root / "data").string().c_str()) == 0, "create original content profile module");
    { RawText writer((root / "data").string().c_str()); writer.setKey("John 3:16"); writer.setEntry("Original content preference body.");
      std::ofstream file(root / "mods.d/text.conf"); file << "[Text]\nDataPath=./data/\nModDrv=RawText\nEncoding=UTF-8\n"; }
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    { std::ofstream file(profile); file << "[Text]\nImage Content=On\nRespect Font Faces=Off\nCustom=café\n[Uninstalled]\nImage Content=Off\nRespect Font Faces=On\nKeep=value\n"; }
    Reader reader(root.string()); StudySession session(reader);
    session.import_module_preferences(profile.string());
    auto options = session.module_presentation_options("Text");
    check(options.image_content == ContentPreference::on && options.respect_font_faces == ContentPreference::off,
          "combined import owns tri-state content preferences");
    session.save_module_preferences(profile.string());
    const auto saved = read_module_option_values(profile.string());
    check(saved.at("Text").at("Image Content") == "On" && saved.at("Text").at("Respect Font Faces") == "Off" &&
          saved.at("Text").at("Custom") == "café" && saved.at("Uninstalled").at("Respect Font Faces") == "On",
          "explicit known settings round trip with unrelated/unavailable values intact");
    ModulePresentationPreferences unknown; unknown.emplace("Text", ModulePresentationOptions{});
    save_module_preferences(profile.string(), {}, unknown);
    check(read_module_option_values(profile.string()).at("Text").at("Image Content") == "On",
          "undiscovered content leaves existing target declaration intact");
    unknown.at("Text").image_content = static_cast<ContentPreference>(2);
    bool invalid = false;
    try { save_module_preferences(profile.string(), {}, unknown); } catch (const std::invalid_argument &) { invalid = true; }
    check(invalid && read_module_option_values(profile.string()).at("Text").at("Image Content") == "On", "invalid enum fails before replacing target");
    StudyWorker worker(root.string()); const auto first = worker.create_tab().get(), second = worker.create_tab().get();
    worker.import_module_preferences(first, profile.string()).get();
    check(worker.module_presentation_options(first, "Text").get().image_content == ContentPreference::on &&
          worker.module_presentation_options(second, "Text").get().image_content == ContentPreference::unknown && worker.history(first).get().entries.empty(),
          "queued content import is tab-local and does not visit history");
    bool failed = false;
    try { worker.import_module_preferences(first, (root / "missing.conf").string()).get(); } catch (const std::invalid_argument &) { failed = true; }
    check(failed && worker.module_presentation_options(first, "Text").get().respect_font_faces == ContentPreference::off,
          "failed combined import retains content settings");
    std::cout << "Content preference frozen/current checks passed: " << comparisons << " combinations\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
