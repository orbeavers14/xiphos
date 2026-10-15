// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/preferences.hpp"
#include "preference_file.hpp"
#include <swconfig.h>
#include <fstream>
#include <stdexcept>
namespace xiphos::core {
void save_module_preferences(const std::string &path, const ModuleEnginePreferences &settings,
                              const ModulePresentationPreferences &presentation) {
  detail::replace_preference_file(path, [&](const std::filesystem::path &file, const std::string &original, bool) {
    {
      std::ofstream snapshot(file, std::ios::binary);
      snapshot.exceptions(std::ios::badbit | std::ios::failbit);
      snapshot << original;
      snapshot.close();
    }
    const auto temporary_path = file.u8string();
    sword::SWConfig configuration(temporary_path.c_str());
    configuration.load();
    for (const auto &module : settings)
      for (const auto &preference : engine_options_to_preferences(module.second))
        configuration[module.first.c_str()][preference.first.c_str()] = preference.second.c_str();
    for (const auto &module : presentation) {
      configuration[module.first.c_str()]["Doublespace"] = module.second.double_space ? "On" : "Off";
      configuration[module.first.c_str()]["Headings"] = module.second.headings ? "On" : "Off";
      auto write_content = [&](const char *key, ContentPreference value) {
        if (value == ContentPreference::unknown) return; // No discovery or explicit setting yet.
        if (value != ContentPreference::on && value != ContentPreference::off)
          throw std::invalid_argument("Invalid content presentation preference");
        configuration[module.first.c_str()][key] = value == ContentPreference::on ? "On" : "Off";
      };
      write_content("Image Content", module.second.image_content);
      write_content("Respect Font Faces", module.second.respect_font_faces);
      if (module.second.commentary_chapter_available)
        configuration[module.first.c_str()]["Commentary by Chapter"] = module.second.commentary_by_chapter ? "On" : "Off";
    }
    configuration.save();
    // SWORD save has no failure result. Verify actual written bytes and decoding
    // before replacing the user's file, rather than treating save() as success.
    (void)detail::read_preference_file(file);
    sword::SWConfig verified(temporary_path.c_str()); verified.load();
    auto expected = configuration.getSections();
    auto actual = verified.getSections();
    // SWConfig adds its own ordering metadata on reload. It changes when new
    // keys are written, plus comment-spacing metadata. Compare every actual
    // preference section/value, excluding only those two reserved SDK indices.
    expected.erase("_ConfOrder"); actual.erase("_ConfOrder");
    expected.erase("_ConfComments"); actual.erase("_ConfComments");
    if (actual != expected)
      throw std::runtime_error("Written preference configuration failed verification");
  });
}
void save_module_engine_preferences(const std::string &path, const ModuleEnginePreferences &settings) {
  save_module_preferences(path, settings, {});
}

}
