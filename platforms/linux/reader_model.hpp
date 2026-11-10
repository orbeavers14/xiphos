// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/c_api.h"
#include <cstdint>
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace xiphos::linux_preview {
struct Module { std::string id, description, type; bool dictionary = false; unsigned declared_flags = 0; bool regular_commentary = false, headings = false; unsigned lexicon_roles = 0; std::string language; bool book = false, search = false; };
struct Preview { std::string module, reference, html, expression; size_t count = 0; bool limited = false; unsigned skipped = 0; };
using Lexicons = std::array<std::optional<std::string>, 5>;
struct Presentation { bool chapter = false, headings = true, double_space = false; int images = -1, font_faces = -1, note_numbers = -1; };
struct LexicalPresentation { std::string css; bool requested = false, applied = false; };
struct Dictionary { std::string module, key, html, source_module, source_reference; std::optional<std::string> anchor; uint64_t anchor_revision = 0; };
// Compare document fields separately: concatenating anchor and revision can
// confuse ("a1", 2) with ("a", 12), suppressing a required repeated scroll.
inline bool same_dictionary_presentation(const Dictionary &left, const Dictionary &right) {
  return left.module == right.module && left.key == right.key && left.html == right.html &&
    left.anchor == right.anchor && left.anchor_revision == right.anchor_revision;
}
struct ReferenceChoice { std::string label, target; };
struct Chapter { std::string html, anchor, title; LexicalPresentation lexical; };
struct Passage { std::string bible, commentary, reference, bible_html, commentary_html; LexicalPresentation bible_lexical, commentary_lexical; std::string anchor_module; std::optional<std::string> anchor; uint64_t anchor_revision = 0; };
struct BookNode { uint64_t offset = 0; std::string path, title; bool present = false, children = false; };
struct BookDocument {
  std::string module, html, css, issue; BookNode nodes[5];
  int images = -1, fonts = -1, spacing = -1, numbers = -1;
  std::optional<std::string> anchor; uint64_t anchor_revision = 0;
};
struct BookContents { std::string module; BookNode parent, up; std::vector<BookNode> nodes; std::optional<uint64_t> after; };
struct Image { std::string module, source; std::vector<unsigned char> bytes; unsigned revision = 0; };
struct SearchHit {
	std::string module, reference;
	size_t group = 0, row = 0;
	int kind = 0;
};
struct SearchGroup { std::string module; xc_status status = XC_OK; xc_search_info info{}; };
struct SearchInput {
  std::vector<std::string> modules; std::string expression, range_module, range;
  int mode = XC_SEARCH_PHRASE; xc_search_scope_kind scope = XC_SEARCH_SCOPE_ALL;
  bool case_sensitive = false; std::optional<xc_search_text_options> annotations;
  bool mixed = false;
  std::optional<std::pair<std::string, std::string>> interval;
};
struct SearchResults {
  std::vector<SearchHit> hits; std::vector<SearchGroup> groups;
  xc_search_generation_info generation{}; xc_search_scope_info scope{};
  bool cancelled = false, limited = false; unsigned revision = 0;
};
struct SearchExport { std::string text, html; int disposition = -1; unsigned revision = 0; };
struct SavedSearchList { std::optional<std::string> label, value; size_t effective = 0; };
struct RestoredWindow {
	uint64_t tab = 0;
	std::string key, module, reference, commentary, dictionary, dictionary_key, host;
	bool parallel_showing = false;
};
struct BookmarkItem {
	int kind = 0;
	std::optional<size_t> parent;
	size_t position = 0;
	std::array<std::optional<std::string>, 8> fields;
};
struct BookmarkRepository {
	std::string path;
	std::optional<std::string> revision;
	std::vector<BookmarkItem> items;
	unsigned version = 0;
};
struct BookmarkImport {
	xc_andbible_counts source{};
	xc_bookmark_import_counts review{};
	std::vector<std::string> collisions;
	bool ready = false;
	unsigned version = 0;
};
struct AnnotationItem {
	std::string label, note;
	bool visible = false;
};
struct AnnotationRepository {
	std::string path;
	std::optional<std::string> revision;
	std::vector<AnnotationItem> items;
};
struct NoteDraft {
	std::string path, module, reference, label, text;
	std::optional<std::string> revision;
	bool generic = false;
	unsigned version = 0;
};
struct EditorDraft {
	std::string path, module, reference, text;
	std::optional<std::string> revision;
	bool new_section = false;
	unsigned version = 0;
};
struct DecoratedReading {
	std::string html, css, module, reference;
	bool commentary = false, chapter = false;
	unsigned version = 0;
};
struct ParallelCell {
	std::string reference, html;
};
struct ParallelColumn {
	std::string requested, module;
	int status = 0;
	bool rtl = false;
};
struct ParallelRow {
	std::string reference;
	std::vector<ParallelCell> cells;
};
struct ParallelReading {
	std::string reference, title;
	std::vector<ParallelColumn> columns;
	std::vector<ParallelRow> rows;
	unsigned revision = 0, selection_revision = 0;
	bool annotations_incomplete = false;
	size_t invalid_colors = 0;
};
struct ParallelSet {
	std::string key, name;
	std::vector<std::string> modules;
};
struct ParallelProfile {
	std::string path;
	std::optional<std::string> revision;
	std::vector<ParallelSet> sets;
};
// UI-thread serial submissions and nonblocking polling only. All engine policy
// and history stay on the core's one FIFO worker. No user-profile discovery.
struct RecoveryDraft;
class ReaderModel {
public:
  explicit ReaderModel(const char *root);
  // Borrow the process worker; the host outlives all models and owns shutdown.
  explicit ReaderModel(xc_worker &shared_worker);
  ReaderModel(xc_worker &shared_worker, const RestoredWindow &restored);
  uint64_t tab_id() const { return tab_; }
  ~ReaderModel();
  ReaderModel(const ReaderModel &) = delete;
  ReaderModel &operator=(const ReaderModel &) = delete;
  void tick();
  bool reload_modules();
  unsigned content_version() const { return content_version_; }
  unsigned catalog_version() const { return catalog_version_; }
  bool open_bookmark(size_t index);
  bool open_bookmarks(const std::string &path, bool create = false);
  bool edit_bookmark(int action, int kind, std::optional<size_t> target, std::optional<size_t> parent,
		     size_t position, const std::array<std::optional<std::string>, 6> &fields);
  bool preview_andbible(const std::string &path);
  bool merge_bookmarks(const std::string &folder, int policy);
  bool export_bookmarks(const std::string &destination);
  const BookmarkImport &bookmark_import() const { return bookmark_import_; }
  const BookmarkRepository &bookmarks() const { return bookmarks_; }
  bool open_annotations(const std::string &path, bool create = false);
  bool begin_note(bool generic);
  bool save_note(const std::string &text, bool remove = false);
  const AnnotationRepository &annotations() const { return annotations_; }
  const NoteDraft &note_draft() const { return note_draft_; }
  bool decorate_reading(bool commentary = false);
  const DecoratedReading &decorated(bool commentary = false) const { return decorated_[commentary ? 1 : 0]; }
  bool open_editor(const std::string &path, bool create = false);
  bool open_module_editor(const std::string &module, const std::string &reference, bool new_section = false);
  bool change_editor(const std::string &text);
  bool editor_undo(bool redo = false);
  bool save_editor(bool erase = false);
  bool save_editor_as(const std::string &path);
  bool discard_editor();
  bool editor_dirty() const;
  std::string editor_saved_text() const;
  bool recover_draft(const RecoveryDraft &draft);
  bool format_editor(size_t start, size_t length, int action, int value, const std::string &text = "");
  const EditorDraft &editor() const { return editor_; }
  bool decorate_parallel();
  bool visit_parallel(const std::vector<std::string> &modules, const std::string &reference);
  bool navigate_parallel(int direction);
  bool new_parallel_profile(const std::string &path);
  bool load_parallel_profile(const std::string &path);
  bool edit_parallel_set(int action, const std::string &key, const std::string &replacement,
			 const std::vector<std::string> &modules, bool make_default = false);
  bool open_parallel_set(const std::string &key, const std::string &reference);
  const ParallelReading &parallel() const { return parallel_; }
  const ParallelProfile &parallel_profile() const { return parallel_profile_; }
  bool choose_book(const std::string &module);
  bool read_book_key(const std::string &key);
  bool read_book_node(uint64_t offset);
  bool navigate_book(int direction);
  bool browse_book(uint64_t parent = 0, bool more = false);
  bool restore_book(const std::string &path);
  bool book_option(int option, bool enabled); // images, fonts, spacing, numbers
  bool refresh_book();
  bool close_book();
  bool follow_book(const std::string &uri);
  const BookDocument &book() const { return book_; }
  const BookContents &book_contents() const { return book_contents_; }
  const Image &image() const { return image_; }
  bool search(const SearchInput &input);
  bool cancel_search();
  bool follow_search_link(const std::string &uri);
  bool edit_search_list(const std::string &path, bool range, const std::string &label,
			const std::string &value, bool remove = false);
  xc_search_progress search_progress() const;
  bool reset_search_annotations();
  bool clear_search_scope();
  bool preview_search_hit(size_t index);
  bool activate_search_hit(size_t index);
  bool export_search(bool html, bool all, const std::vector<size_t> &selection,
                     bool scripture, bool context, bool images, bool partial);
  bool import_search_lists(const std::string &path);
  bool parse_search_modules(const std::string &text);
  const SearchResults &search_results() const { return search_results_; }
  int search_preview_kind() const { return search_preview_source_.kind; }
  const Preview &search_preview() const { return search_preview_; }
  const SearchExport &search_export() const { return search_export_; }
  const std::vector<SavedSearchList> &saved_lists(bool ranges) const { return saved_lists_[ranges ? 1 : 0]; }
  const std::vector<std::string> &parsed_search_modules() const { return parsed_search_modules_; }
  unsigned parsed_search_revision() const { return parsed_search_revision_; }

  bool visit(const std::string &bible, const std::string &commentary, const std::string &reference);
  bool navigate(int direction);
  bool choose_commentary(const std::string &module);
  bool set_lexicons(const Lexicons &names);
  bool lexicon_profile(const std::string &path, bool save);
  const Lexicons &lexicons() const { return lexicons_; }
  bool set_cross_reference_list(bool enabled);
  bool cross_reference_list() const { return cross_reference_list_; }
  bool hover(const std::string &uri);
  bool preview_busy() const { return hover_request_ != nullptr; }
  bool show_references();
  bool show_note();
  bool close_preview();
  const Preview &note() const { return note_; }
  const Preview &references() const { return references_; }
  const Preview &information() const { return information_; }
  bool showing_references() const { return showing_references_; }
  bool set_filter(unsigned flag, bool enabled);
  bool select_variant(int choice);
  bool set_double_space(bool commentary, bool enabled);
  bool set_note_numbers(bool commentary, bool enabled);
  bool set_content(bool commentary, bool font_faces, bool enabled);
  bool set_commentary_headings(bool enabled);
  bool reading_profile(const std::string &path, bool save);
  unsigned engine_flags() const { return engine_flags_; }
  bool engine_configured() const { return engine_configured_; }
  const Module *module(const std::string &id) const;
  const Presentation &presentation(bool commentary = false) const { return presentations_[commentary ? 1 : 0]; }
  const std::string &notice() const { return notice_; }
  bool lookup_dictionary(const std::string &module, const std::string &key, bool record = false);
  bool step_dictionary(int direction);
  bool navigate_dictionary_history(int direction);
  bool clear_dictionary_history();
  bool close_dictionary();
  const Dictionary &dictionary() const { return dictionary_; }
  const std::vector<std::string> &dictionary_keys() const { return dictionary_keys_; }
  bool dictionary_can_back() const { return dictionary_back_; }
  bool dictionary_can_forward() const { return dictionary_forward_; }
  size_t dictionary_history_count() const { return dictionary_history_count_; }
  bool navigate_chapter(int direction);
  bool clear_history();
  bool set_display(bool chapter, unsigned flags);
  bool set_commentary_chapter(bool enabled);
  bool refresh();
  bool follow(const std::string &source, const std::string &uri, const std::string &context = "");
  // Stop submissions/drop observation, then transfer shutdown ownership. The
  // caller MUST close the returned worker on a background thread (drain/join).
  xc_worker *detach();
  bool busy() const { return request_ != nullptr; }
  bool ready() const { return ready_ && !closed_; }
  bool can_back() const { return back_; }
  bool can_forward() const { return forward_; }
  const std::vector<Module> &modules() const { return modules_; }
  const Passage &passage() const { return passage_; }
  const Chapter &bible_chapter() const { return bible_chapter_; }
  const Chapter &commentary_chapter() const { return commentary_chapter_; }
  bool show_chapter() const { return show_chapter_; }
  bool show_commentary_chapter() const { return show_commentary_chapter_; }
  unsigned display_flags() const { return display_flags_; }
  const std::vector<ReferenceChoice> &choices(int kind) const { return choices_[kind]; }
  const std::string &choice_label(int kind) const { return choice_labels_[kind]; }
  const std::string &error() const { return error_; }
  unsigned revision() const { return revision_; }
private:
  enum class Operation { editor_bind,
			 editor_release,
			 parallel_decoration,
			 andbible,
			 import_review,
			 bookmarks_export,
			 catalog_refresh,
			 restored_parallel,
			 bookmarks,
			 annotations,
			 note_target,
			 note_saved,
			 editor_file,
			 editor_module,
			 decorated,
			 parallel,
			 parallel_profile,
			 search_keyed_preview,
			 search_link_validate,
			 book_document,
			 book_uri_document,
			 book_children,
			 book_key,
			 book_option,
			 book_link,
			 search_batch,
			 search_preview,
			 search_export,
			 search_reset,
			 search_scope,
			 search_lists,
			 search_names,
			 catalog,
			 tab,
			 preferences,
			 passage,
			 history,
			 link,
			 refresh,
			 clear,
			 presentation,
			 bible_chapter,
			 commentary_chapter,
			 choices,
			 dictionary,
			 dictionary_choices,
			 dictionary_history,
			 dictionary_clear,
			 engine_options,
			 bible_options,
			 commentary_options,
			 saved,
			 enable_lexicons,
			 lexicons,
			 lexicon_changed,
			 reference_preference,
			 reference_preference_changed,
			 references,
			 entry,
			 lexical_layout,
			 note_numbering,
			 heading_note_links };
  bool activate_target(const SearchHit &hit);
  void restore_next();
  std::optional<RestoredWindow> restoring_;
  unsigned restore_stage_ = 0;
  bool take_repository(xc_request *request);
  bool take_extended(xc_request *request);
  bool take_image(xc_request *request);
  unsigned catalog_version_ = 0, content_version_ = 0;
  bool pending_editor_write_ = false;
  BookmarkRepository bookmarks_;
  BookmarkImport bookmark_import_;
  std::string import_xml_, import_path_;
  std::optional<std::string> import_revision_;
  AnnotationRepository annotations_;
  NoteDraft note_draft_;
  EditorDraft editor_, pending_editor_;
  xc_editor_buffer *editor_buffer_ = nullptr;
  DecoratedReading decorated_[2];
  bool pending_decorated_commentary_ = false;
  std::string pending_repository_path_;
  ParallelReading parallel_;
  ParallelProfile parallel_profile_;
  std::string pending_parallel_profile_;
  BookDocument book_;
  BookContents book_contents_;
  Image image_;
  bool append_book_children_ = false;
  uint64_t pending_book_parent_ = 0;
  std::optional<std::string> pending_book_anchor_;
  uint64_t book_anchor_revision_ = 0;
  BookNode pending_book_up_;
  SearchHit search_preview_source_, pending_search_source_;
  std::string pending_search_uri_;
  SearchResults search_results_;
  Preview search_preview_;
  SearchExport search_export_;
  std::vector<SavedSearchList> saved_lists_[2];
  std::vector<std::string> parsed_search_modules_;
  unsigned parsed_search_revision_ = 0;
  bool accept(xc_status status, xc_request *request, Operation operation);
  void history();
  bool take_preview(xc_request *request, bool hover);
  void poll_hover();
  void load_chapters(bool commentary = false);
  void load_choices();
  xc_worker *worker_ = nullptr;
  bool owns_worker_ = true;
  xc_request *request_ = nullptr;
  uint64_t tab_ = 0;
  Operation operation_ = Operation::catalog;
  bool ready_ = false, closed_ = false, back_ = false, forward_ = false;
  std::vector<Module> modules_;
  Passage passage_;
  xc_request *hover_request_ = nullptr;
  Lexicons lexicons_;
  Preview note_, references_, information_;
  bool showing_references_ = false, cross_reference_list_ = true;
  unsigned engine_flags_ = 0;
  bool engine_configured_ = false;
  Presentation presentations_[2];
  std::string notice_, pending_module_, dictionary_source_module_, dictionary_source_reference_;
  Dictionary dictionary_;
  std::vector<std::string> dictionary_keys_;
  bool dictionary_back_ = false, dictionary_forward_ = false;
  size_t dictionary_history_count_ = 0;
  uint64_t anchor_revision_ = 0;
  uint64_t dictionary_anchor_revision_ = 0;
  Chapter bible_chapter_, commentary_chapter_;
  bool show_chapter_ = false, show_commentary_chapter_ = false;
  unsigned display_flags_ = XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED;
  std::vector<ReferenceChoice> choices_[3];
  std::string choice_labels_[3];
  std::string error_;
  unsigned revision_ = 0;
};
}
