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
ParsedReferenceList StudySession::reference_targets(const std::string &module, const std::string &expression,
                                                    const std::string &context) {
  // Parsing depends on module versification/context, not rendering options.
  return reader_.reference_targets(module, expression, context);
}
StudyNote StudySession::study_note(const std::string &module, const std::string &reference,
                                  const std::string &identifier) {
  const auto options = module_engine_options(module);
  return options ? reader_.study_note(module, reference, identifier, *options)
                 : reader_.study_note(module, reference, identifier);
}
StudyNote StudySession::study_note_metadata(const std::string &module, const std::string &reference,
                                            const std::string &identifier) {
  const auto options = module_engine_options(module);
  return options ? reader_.study_note_metadata(module, reference, identifier, *options)
                 : reader_.study_note_metadata(module, reference, identifier);
}
Entry StudySession::read_entry(const std::string &module, const std::string &reference) {
  const auto options = module_engine_options(module);
  return options ? reader_.read(module, reference, *options) : reader_.read(module, reference);
}
ModulePresentationOptions StudySession::module_presentation_options(const std::string &id) const {
  require_module(id);
  const auto found = presentation_options_.find(id);
  if (found != presentation_options_.end()) return found->second;
  for (const auto &module : reader_.modules())
    if (module.id == id) return presentation_options_from_preferences({}, module.regular_commentary);
  throw std::out_of_range("Module is not installed");
}
void StudySession::set_commentary_chapter_mode(const std::string &id, bool enabled) {
  require_module(id);
  for (const auto &module : reader_.modules()) {
    if (module.id != id) continue;
    if (!module.regular_commentary) throw std::invalid_argument("Chapter mode requires a regular commentary");
    auto options = module_presentation_options(id);
    options.commentary_by_chapter = enabled;
    presentation_options_[id] = options;
    return;
  }
}
Chapter StudySession::read_commentary_chapter(const std::string &module, const std::string &reference,
                                              const DisplayOptions &display) {
  const auto options = module_engine_options(module);
  return options ? reader_.read_commentary_chapter(module, reference, display, *options)
                 : reader_.read_commentary_chapter(module, reference, display);
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
  ModulePresentationPreferences presentation;
  for (const auto &module : reader_.modules()) {
    const auto options = module_engine_options(module.id);
    if (!options) throw std::invalid_argument("Enable module preferences before saving");
    settings.emplace(module.id, *options);
    if (module.regular_commentary) presentation.emplace(module.id, module_presentation_options(module.id));
  }
  xiphos::core::save_module_preferences(path, settings, presentation);
}
void StudySession::import_module_preferences(const std::string &path) {
  const auto loaded = read_module_option_values(path);
  ModuleEnginePreferences replacement;
  ModulePresentationPreferences presentation;
  for (const auto &module : reader_.modules()) {
    const auto found = loaded.find(module.id);
    const auto values = found == loaded.end() ? EngineOptionPreferences{} : found->second;
    replacement.emplace(module.id, engine_options_from_preferences(values));
    presentation.emplace(module.id, presentation_options_from_preferences(values, module.regular_commentary));
  }
  // Commit only after all file reads, decoding and allocations succeed.
  module_options_.swap(replacement);
  presentation_options_.swap(presentation);
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
StudyPassage StudySession::read_passage(const std::string &module, const std::string &commentary,
                                        const std::string &reference) {
  if (reference.empty() || reference.find('\0') != std::string::npos)
    throw std::invalid_argument("Study passage requires a nonempty reference without NUL");
  auto require_type = [&](const std::string &id, const char *type) {
    require_module(id);
    for (const auto &installed : reader_.modules())
      if (installed.id == id && installed.type != type)
        throw std::invalid_argument("Module has the wrong study-pane type");
  };
  require_type(module, "Biblical Texts");
  if (!commentary.empty()) require_type(commentary, "Commentaries");
  StudyPassage result{module, commentary, read_entry(module, reference), std::nullopt};
  if (!commentary.empty()) result.commentary = read_entry(commentary, result.text.reference);
  return result;
}
StudyPassage StudySession::visit_passage(const std::string &module, const std::string &commentary,
                                         const std::string &reference) {
  auto result = read_passage(module, commentary, reference);
  // Render both panes before committing history. Preserve the characterized
  // reference-only deduplication, including its first recorded module pair.
  history_.visit({result.text.reference, module, commentary});
  return result;
}
StudyPassage StudySession::visit_linked_passage(const std::string &bible, const std::string &commentary,
                                                const PassageLink &target) {
  require_module(target.module);
  for (const auto &module : reader_.modules()) {
    if (module.id != target.module) continue;
    if (module.type == "Biblical Texts") return visit_passage(target.module, commentary, target.reference);
    if (module.type == "Commentaries") return visit_passage(bible, target.module, target.reference);
  }
  throw std::invalid_argument("Link target is not a Bible or commentary");
}
StudyPassage StudySession::navigate_passage(int direction) {
  const auto &location = history_.destination(direction);
  // Old single-entry clients can have standalone commentary locations. They
  // have no paired commentary; retain that history compatibility in the core.
  const bool bible = [&] {
    for (const auto &module : reader_.modules())
      if (module.id == location.module) return module.type == "Biblical Texts";
    return false;
  }();
  auto result = !bible && location.commentary.empty()
      ? StudyPassage{location.module, "", read_entry(location.module, location.reference), std::nullopt}
      : read_passage(location.module, location.commentary, location.reference);
  history_.move(direction);
  return result;
}
StudyPassage StudySession::navigate_passage_chapter(const std::string &module, const std::string &commentary,
                                                    const std::string &reference, int direction) {
  return visit_passage(module, commentary, reader_.adjacent_chapter(module, reference, direction));
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
