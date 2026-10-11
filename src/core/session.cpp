// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include <stdexcept>
#include "xiphos/core/preferences.hpp"
#include "xiphos/core/dictionary.hpp"
namespace xiphos::core {
void StudySession::set_lexicon_preferences(const LexiconPreferences &preferences) {
  (void)borrow_lexicon_preferences(preferences);
  auto replacement = std::make_optional(preferences);
  lexicons_.swap(replacement);
}
LexicalEntry StudySession::visit_lexical(const std::string &module, const std::string &reference,
                                      const std::string &uri, int origin) {
  return lookup_lexical(module, reference, uri, origin, true);
}
LexicalEntry StudySession::read_lexical(const std::string &module, const std::string &reference,
                                     const std::string &uri, int origin) {
  return lookup_lexical(module, reference, uri, origin, false);
}
LexicalEntry StudySession::lookup_lexical(const std::string &module, const std::string &reference,
                                        const std::string &uri, int origin, bool click) {
  if (!lexicons_) throw std::invalid_argument("Configure lexicons before lexical lookup");
  auto command = lexical_link(uri, module);
  // Legacy morphology uses the caller's committed source context; a query
  // passage is retained as metadata rather than replacing that context.
  const auto &context = reference;
  const auto selected = reader_.select_lexicon(command, context, origin, *lexicons_);
  // Main-pane clicks uppercase and record the requested key; information
  // dialogs and hover previews keep the raw key and have no dictionary visit.
  const bool main_click = click && origin == XIPHOS_LEXICON_MAIN;
  const auto key = main_click ? dictionary_click_key(command.value) : command.value;
  auto entry = read_dictionary(selected, key);
  if (main_click) dictionary_history_.visit({selected, key});
  return {std::move(command), context, std::move(entry)};
}
DictionaryEntry StudySession::visit_dictionary(const std::string &module, const std::string &key) {
  auto entry = read_dictionary(module, key);
  dictionary_history_.visit({module, key});
  return entry;
}
DictionaryEntry StudySession::navigate_dictionary_history(int direction) {
  const auto target = dictionary_history_.destination(direction);
  auto entry = read_dictionary(target.module, target.key);
  dictionary_history_.move(direction);
  return entry;
}
DictionaryEntry StudySession::navigate_dictionary(const std::string &module, const std::string &key, int direction) {
  const auto options = module_engine_options(module);
  return options ? reader_.navigate_dictionary(module, key, direction, *options)
                 : reader_.navigate_dictionary(module, key, direction);
}
DictionaryEntry StudySession::read_dictionary(const std::string &module, const std::string &key) {
  const auto options = module_engine_options(module);
  return options ? reader_.read_dictionary(module, key, *options) : reader_.read_dictionary(module, key);
}
ReferenceList StudySession::read_reference_list(const std::string &module, const std::string &expression,
                                              const std::string &context) {
  const auto options = module_engine_options(module);
  return options ? reader_.read_reference_list(module, expression, context, *options)
                 : reader_.read_reference_list(module, expression, context);
}
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
void StudySession::select_module_variant(const std::string &module, int choice) {
  const auto options = module_engine_options(module);
  if (!options) throw std::invalid_argument("Enable module preferences before selecting a variant");
  bool declared = false;
  for (const auto &installed : reader_.modules())
    if (installed.id == module) declared = installed.options.declared.variants_primary;
  if (!declared) throw std::invalid_argument("Module does not declare a textual variant filter");
  set_module_engine_options(module, select_variant_reading(*options, choice));
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
