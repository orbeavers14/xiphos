// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "history.hpp"
#include "dictionary_history.hpp"
#include "reader.hpp"
#include <optional>
#include <map>
namespace xiphos::core {
// One instance per study tab/window. Calls and lifetime must be serialized.
// The reader must outlive this C++ session; C ABI sessions retain their reader.
// Owned synchronized pane snapshot. Commentary uses the normalized Bible
// reference; each module still resolves that reference in its own versification.
struct StudyPassage {
  std::string module, commentary_module;
  Entry text;
  std::optional<Entry> commentary;
};
class StudySession {
public:
  explicit StudySession(Reader &reader) : reader_(reader) {}
  // Explicit copied lexicons; existing tabs remain unconfigured until set.
  void set_lexicon_preferences(const LexiconPreferences &preferences);
  const std::optional<LexiconPreferences> &lexicon_preferences() const { return lexicons_; }
  void set_cross_reference_preview_list(bool enabled) { xrefs_in_verse_list_ = enabled; }
  bool cross_reference_preview_list() const { return xrefs_in_verse_list_; }
  // Reference is the committed source context; URI passage remains metadata.
  LexicalEntry visit_lexical(const std::string &module, const std::string &reference, const std::string &uri, int origin);
  LexicalEntry read_lexical(const std::string &module, const std::string &reference,
                          const std::string &uri, int origin);
  // Render with this tab's settings without moving or adding history.
  // Explicit recorded visit; retains requested module/key, without case policy.
  DictionaryEntry visit_dictionary(const std::string &module, const std::string &key);
  DictionaryEntry navigate_dictionary_history(int direction);
  const DictionaryHistory &dictionary_history() const { return dictionary_history_; }
  void clear_dictionary_history() { dictionary_history_.clear(); }
  DictionaryEntry navigate_dictionary(const std::string &module, const std::string &key, int direction);
  DictionaryEntry read_dictionary(const std::string &module, const std::string &key);
  ReferenceList read_reference_list(const std::string &module, const std::string &expression,
                                    const std::string &context);
  ParsedReferenceList reference_targets(const std::string &module, const std::string &expression,
                                         const std::string &context);
  StudyNote study_note(const std::string &module, const std::string &reference,
                       const std::string &identifier);
  StudyNote study_note_metadata(const std::string &module, const std::string &reference,
                                 const std::string &identifier);
  Entry read_entry(const std::string &module, const std::string &reference);
  StudyPassage read_passage(const std::string &module, const std::string &commentary,
                            const std::string &reference);
  StudyPassage visit_passage(const std::string &module, const std::string &commentary,
                             const std::string &reference);
  StudyPassage navigate_passage(int direction);
  StudyPassage navigate_passage_chapter(const std::string &module, const std::string &commentary,
                                        const std::string &reference, int direction);
  Entry visit(const std::string &module, const std::string &reference);
  Entry navigate(int direction);
  Entry navigate_chapter(const std::string &module, const std::string &reference, int direction);
  // Presentation changes do not add history entries. All future tab reads use
  // copied settings. Clearing this layer retains explicit module overrides
  // and enabled module defaults; otherwise it restores unconfigured SDK state.
  void set_engine_options(const EngineOptions &options) { options_ = options; }
  void clear_engine_options() { options_.reset(); }
  // Opt-in module preferences use legacy defaults without reading user files.
  // Explicit module overrides win over tab-wide overrides; then opt-in defaults.
  // Existing sessions remain unconfigured until explicitly enabled or set.
  void enable_module_preferences() { module_preferences_ = true; }
  // Explicit read-only import replaces this tab's engine preferences for all
  // installed modules. Missing sections use legacy defaults. Failed imports
  // preserve settings/history; other tabs and source bytes are untouched.
  void import_module_preferences(const std::string &path);
  // Saves explicit effective options, never guesses unconfigured SDK state.
  void save_module_preferences(const std::string &path) const;
  // Requires configured settings and a declared variant filter; no history visit.
  void select_module_variant(const std::string &module, int choice);
  void set_module_engine_options(const std::string &module, const EngineOptions &options);
  void clear_module_engine_options(const std::string &module);
  std::optional<EngineOptions> module_engine_options(const std::string &module) const;
  Chapter read_commentary_chapter(const std::string &module, const std::string &reference,
                                  const DisplayOptions &display = {});
  Chapter read_chapter(const std::string &module, const std::string &reference,
                       const DisplayOptions &display = {});
  const History &history() const { return history_; }
  void clear_history() { history_.clear(); }
private:
  Reader &reader_;
  std::optional<LexiconPreferences> lexicons_;
  std::optional<EngineOptions> options_;
  bool module_preferences_ = false;
  bool xrefs_in_verse_list_ = true; // Legacy misc/xrefsinverselist default.
  std::map<std::string, EngineOptions> module_options_;
  void require_module(const std::string &module) const;
  LexicalEntry lookup_lexical(const std::string &module, const std::string &reference,
                            const std::string &uri, int origin, bool click);
  History history_;
  DictionaryHistory dictionary_history_;
};
}
