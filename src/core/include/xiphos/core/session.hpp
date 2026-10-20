// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "history.hpp"
#include "dictionary_history.hpp"
#include "reader.hpp"
#include "presentation.hpp"
#include "search_export.hpp"
#include "html_source.hpp"
#include <optional>
#include <map>
namespace xiphos::core {
using BookFollowLink = std::variant<BookNote, BookImageResource>;
struct BookSelectionState {
  BookSelectionTarget requested;
  BookTree tree;
  std::optional<BookDocument> document; // Present only after atomic document selection.
};
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
  // Search updates only this session's complete last-result scope, never reading/history.
  SearchResult search_module(SearchQuery query,const SearchLimits &limits = {},std::shared_ptr<SearchControl> control = {});
  // Up to 32 validated queries. Output budgets span all groups; execution and
  // complete next scope continue when the copied prefix budget is exhausted.
  // Per-module errors are owned group statuses; fatal failure publishes no state.
  SearchBatchResult search_batch(std::vector<SearchQuery> queries,const SearchBatchLimits &limits = {},std::shared_ptr<SearchControl> control = {});
  SearchGenerationInfo search_generation() const;
  // Policy applies to future generations only; no mutation of current results.
  void set_search_retention_limits(SearchRetentionLimits limits);
  SearchRetentionLimits search_retention_limits() const {return search_retention_limits_;}
  SearchGenerationRead read_search_generation(std::uint64_t id,std::size_t group,SearchReferenceSource source=SearchReferenceSource::complete,const SearchLimits &limits = {}) const;
  SearchTextExport export_search_all_text(SearchAllTextExportRequest request,std::shared_ptr<SearchControl> control = {});
  SearchTextExport export_search_visible_text(SearchTextExportRequest request,std::shared_ptr<SearchControl> control = {});
  SearchScopeInfo last_search_scope() const;
  void clear_search_scope();
  SearchTextState search_text_options() const {return {search_text_options_};}
  void clear_search_text_options(){search_text_options_.reset();}
  // Migration opt-in for consumers that copy/apply snapshot HTML and CSS together.
  // Not a saved study preference. Existing consumers retain plain snapshots.
  void set_lexical_layout_enabled(bool enabled) { lexical_layout_enabled_ = enabled; }
  // Separate migration opt-in: applies SDK labels and owned per-document note
  // counters only for configured module engine state. Not a saved preference.
  void set_note_numbering_enabled(bool enabled) { note_numbering_enabled_ = enabled; }
  // Default-off migration capability. Requires available numbering/source capture;
  // repairs only heading candidates identical outside their note URI identifiers.
  // No saved preference or history visit; raw Reader note sources stay unchanged.
  void set_heading_note_links_enabled(bool enabled) { heading_note_links_enabled_ = enabled; }
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
  // Raw book source/HTML with existing effective engine options. No visit or
  // book-specific presentation/history/persistence policy is introduced yet.
  BookEntry read_book(const std::string &module, std::uint64_t offset);
  BookDocument read_book_document(const std::string &module, std::uint64_t offset);
  // Publish only after target validation and configured entry rendering succeed.
  // Read-only book/notes/restore-resolution calls do not update this state.
  BookEntry select_book(const BookSelectionTarget &target);
  BookDocument select_book_document(const BookSelectionTarget &target);
  std::optional<BookDocument> refresh_book_document();
  std::optional<BookDocument> navigate_book_document(BookNavigation direction);
  std::optional<BookDocument> restore_book_document_selection(const SavedBookSelection &saved, BookRestoreMode mode);
  BookNote book_document_note_link(const std::string &uri, bool render_body = true);
  BookImageTarget book_image_link(const std::string &uri);
  BookImageResource read_book_image_link(const std::string &uri);
  BookFollowLink follow_book_link(const std::string &uri);
  // Missing selection/relation is a no-op; relationship lookup and reading policy stay in core.
  std::optional<BookEntry> navigate_book(BookNavigation direction);
  // No saved module is a successful no-op, retaining any existing selection.
  std::optional<BookEntry> restore_book_selection(const SavedBookSelection &saved, BookRestoreMode mode);
  const std::optional<BookSelectionState> &book_selection() const { return book_selection_; }
  BookNote book_note(const std::string &module, std::uint64_t offset, const std::string &identifier, bool render_body = true);
  BookNote book_note_link_at(const std::string &module, std::uint64_t offset, const std::string &uri, bool render_body = true);
  ReferenceList read_reference_list(const std::string &module, const std::string &expression,
                                    const std::string &context);
  ParsedReferenceList reference_targets(const std::string &module, const std::string &expression,
                                         const std::string &context);
  StudyNote study_note(const std::string &module, const std::string &reference,
                       const std::string &identifier);
  StudyNote study_note_metadata(const std::string &module, const std::string &reference,
                                 const std::string &identifier);
  Entry read_commentary_entry(const std::string &module, const std::string &reference);
  Entry read_entry(const std::string &module, const std::string &reference);
  // Configured HTML/CSS source without publishing automatic image/font choices.
  // Same renderer/cleanup/notes as live reads, no history or selection changes.
  // Owned source only: no HTML document/assets/output-size contract implied.
  // Bounded body/CSS source; no profile discovery publication or output writes.
  EntryHtmlSource capture_entry_html(const std::string &module,const std::string &reference,const HtmlSourceLimits &limits = {});
  Entry read_entry_snapshot(const std::string &module,const std::string &reference);
  Entry read_commentary_entry_snapshot(const std::string &module,const std::string &reference);
  StudyPassage read_passage(const std::string &module, const std::string &commentary,
                            const std::string &reference);
  StudyPassage visit_passage(const std::string &module, const std::string &commentary,
                             const std::string &reference);
  StudyPassage visit_linked_passage(const std::string &bible, const std::string &commentary,
                                    const PassageLink &target);
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
  ModulePresentationOptions module_presentation_options(const std::string &module) const;
  void set_headings(const std::string &module, bool enabled);
  void set_note_numbers(const std::string &module, bool enabled);
  void set_double_space(const std::string &module, bool enabled);
  void set_image_content(const std::string &module, bool enabled);
  void set_respect_font_faces(const std::string &module, bool enabled);
  void set_commentary_chapter_mode(const std::string &module, bool enabled);
  Chapter read_commentary_chapter(const std::string &module, const std::string &reference,
                                  const DisplayOptions &display = {});
  Chapter read_chapter(const std::string &module, const std::string &reference,
                       const DisplayOptions &display = {});
  const History &history() const { return history_; }
  void clear_history() { history_.clear(); }
private:
  std::shared_ptr<const detail::SearchScope> search_scope_;
  std::shared_ptr<const detail::SearchGeneration> search_generation_;
  std::uint64_t search_generation_serial_=0;
  SearchRetentionLimits search_retention_limits_;
  std::optional<SearchTextOptions> search_text_options_;
  std::optional<BookSelectionState> book_selection_;
  Reader &reader_;
  std::optional<LexiconPreferences> lexicons_;
  std::optional<EngineOptions> options_;
  bool module_preferences_ = false;
  bool lexical_layout_enabled_ = false;
  bool note_numbering_enabled_ = false;
  bool heading_note_links_enabled_ = false;
  bool xrefs_in_verse_list_ = true; // Legacy misc/xrefsinverselist default.
  std::map<std::string, EngineOptions> module_options_;
  ModulePresentationPreferences presentation_options_;
  void require_module(const std::string &module) const;
  std::optional<BookSelectionTarget> book_navigation_target(BookNavigation direction);
  std::vector<std::vector<std::string>> heading_note_html(const std::string &module,
      const std::vector<Entry *> &entries, const ModulePresentationOptions &options,
      const std::optional<EngineOptions> &engine);
  Entry read_entry_impl(const std::string &module,const std::string &reference,bool commentary,bool publish_presentation,const std::size_t *max_html_bytes=nullptr);
  Entry present_content(const std::string &module, Entry entry,bool publish_presentation=true);
  Chapter present_content(const std::string &module, Chapter chapter, const DisplayOptions &display, bool commentary);
  LexicalEntry lookup_lexical(const std::string &module, const std::string &reference,
                            const std::string &uri, int origin, bool click);
  History history_;
  DictionaryHistory dictionary_history_;
};
}
