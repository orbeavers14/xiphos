// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include <stdexcept>
#include "xiphos/core/preferences.hpp"
namespace xiphos::core {
StudyNote StudySession::study_note(const std::string &module, const std::string &reference,
                                  const std::string &identifier) {
  const auto options = module_engine_options(module);
  return options ? reader_.study_note(module, reference, identifier, *options)
                 : reader_.study_note(module, reference, identifier);
}
Entry StudySession::read_entry(const std::string &module, const std::string &reference) {
  const auto options = module_engine_options(module);
  return options ? reader_.read(module, reference, *options) : reader_.read(module, reference);
}
Chapter StudySession::read_chapter(const std::string &module, const std::string &reference,
                                  const DisplayOptions &display) {
  const auto options = module_engine_options(module);
  return options ? reader_.read_chapter(module, reference, display, *options)
                  : reader_.read_chapter(module, reference, display);
}
void StudySession::require_module(const std::string &module) const {
  for (const auto &installed : reader_.modules()) if (installed.id == module) return;
  throw std::out_of_range("Module is not installed");
}
void StudySession::save_module_preferences(const std::string &path) const {
  ModuleEnginePreferences settings;
  for (const auto &module : reader_.modules()) {
    const auto options = module_engine_options(module.id);
    if (!options) throw std::invalid_argument("Enable module preferences before saving");
    settings.emplace(module.id, *options);
  }
  save_module_engine_preferences(path, settings);
}
void StudySession::import_module_preferences(const std::string &path) {
  const auto loaded = read_module_engine_preferences(path);
  ModuleEnginePreferences replacement;
  for (const auto &module : reader_.modules()) {
    const auto found = loaded.find(module.id);
    replacement.emplace(module.id, found == loaded.end()
        ? engine_options_from_preferences({}) : found->second);
  }
  // Commit only after all file reads, decoding and allocations succeed.
  module_options_.swap(replacement);
  options_.reset();
  module_preferences_ = true;
}
void StudySession::set_module_engine_options(const std::string &module, const EngineOptions &options) {
  require_module(module);
  module_options_[module] = options;
}
void StudySession::clear_module_engine_options(const std::string &module) {
  require_module(module);
  module_options_.erase(module);
}
std::optional<EngineOptions> StudySession::module_engine_options(const std::string &module) const {
  require_module(module);
  const auto found = module_options_.find(module);
  if (found != module_options_.end()) return found->second;
  if (options_) return options_;
  if (module_preferences_) return engine_options_from_preferences({});
  return std::nullopt;
}
Entry StudySession::visit(const std::string &module, const std::string &reference) {
  auto entry = read_entry(module, reference);
  history_.visit({entry.reference, module, ""});
  return entry;
}
Entry StudySession::navigate_chapter(const std::string &module, const std::string &reference,
                                     int direction) {
  return visit(module, reader_.adjacent_chapter(module, reference, direction));
}
Entry StudySession::navigate(int direction) {
  const auto &location = history_.destination(direction);
  auto entry = read_entry(location.module, location.reference);
  history_.move(direction);
  return entry;
}
}
