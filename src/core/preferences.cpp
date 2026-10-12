// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/preferences.hpp"
#include "preference_file.hpp"
#include <swconfig.h>
#include <fstream>
#include <stdexcept>
namespace xiphos::core {
void save_module_engine_preferences(const std::string &path, const ModuleEnginePreferences &settings) {
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
}
