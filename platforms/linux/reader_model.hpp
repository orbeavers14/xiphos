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
struct SearchHit { std::string module, reference; size_t group = 0, row = 0; };
struct SearchGroup { std::string module; xc_status status = XC_OK; xc_search_info info{}; };
struct SearchInput {
  std::vector<std::string> modules; std::string expression, range_module, range;
  int mode = XC_SEARCH_PHRASE; xc_search_scope_kind scope = XC_SEARCH_SCOPE_ALL;
  bool case_sensitive = false; std::optional<xc_search_text_options> annotations;
};
struct SearchResults {
  std::vector<SearchHit> hits; std::vector<SearchGroup> groups;
  xc_search_generation_info generation{}; xc_search_scope_info scope{};
  bool cancelled = false, limited = false; unsigned revision = 0;
};
struct SearchExport { std::string text, html; int disposition = -1; unsigned revision = 0; };
struct SavedSearchList { std::optional<std::string> label, value; size_t effective = 0; };
// UI-thread serial submissions and nonblocking polling only. All engine policy
// and history stay on the core's one FIFO worker. No user-profile discovery.
class ReaderModel {
public:
  explicit ReaderModel(const char *root);
  // Borrow the process worker; the host outlives all models and owns shutdown.
  explicit ReaderModel(xc_worker &shared_worker);
  ~ReaderModel();
  ReaderModel(const ReaderModel &) = delete;
  ReaderModel &operator=(const ReaderModel &) = delete;
  void tick();
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
  enum class Operation { book_document, book_uri_document, book_children, book_key, book_option, book_link, search_batch, search_preview, search_export, search_reset, search_scope, search_lists, search_names, catalog, tab, preferences, passage, history, link, refresh, clear, presentation, bible_chapter, commentary_chapter, choices, dictionary, dictionary_choices, dictionary_history, dictionary_clear, engine_options, bible_options, commentary_options, saved, enable_lexicons, lexicons, lexicon_changed, reference_preference, reference_preference_changed, references, entry, lexical_layout, note_numbering, heading_note_links };
  bool take_extended(xc_request *request);
  bool take_image(xc_request *request);
  BookDocument book_;
  BookContents book_contents_;
  Image image_;
  bool append_book_children_ = false;
  uint64_t pending_book_parent_ = 0;
  std::optional<std::string> pending_book_anchor_;
  uint64_t book_anchor_revision_ = 0;
  BookNode pending_book_up_;
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
