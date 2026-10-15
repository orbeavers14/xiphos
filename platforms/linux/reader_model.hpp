// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/c_api.h"
#include <cstdint>
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace xiphos::linux_preview {
struct Module { std::string id, description, type; bool dictionary = false; unsigned declared_flags = 0; bool regular_commentary = false, headings = false; unsigned lexicon_roles = 0; };
struct Preview { std::string module, reference, html, expression; size_t count = 0; bool limited = false; unsigned skipped = 0; };
using Lexicons = std::array<std::optional<std::string>, 5>;
struct Presentation { bool chapter = false, headings = true; int images = -1, font_faces = -1; };
struct Dictionary { std::string module, key, html; };
struct ReferenceChoice { std::string label, target; };
struct Chapter { std::string html, anchor, title; };
struct Passage { std::string bible, commentary, reference, bible_html, commentary_html; };
// UI-thread serial submissions and nonblocking polling only. All engine policy
// and history stay on the core's one FIFO worker. No user-profile discovery.
class ReaderModel {
public:
  explicit ReaderModel(const char *root);
  ~ReaderModel();
  ReaderModel(const ReaderModel &) = delete;
  ReaderModel &operator=(const ReaderModel &) = delete;
  void tick();
  bool visit(const std::string &bible, const std::string &commentary, const std::string &reference);
  bool navigate(int direction);
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
  enum class Operation { catalog, tab, preferences, passage, history, link, refresh, clear, presentation, bible_chapter, commentary_chapter, choices, dictionary, dictionary_choices, dictionary_history, dictionary_clear, engine_options, bible_options, commentary_options, saved, enable_lexicons, lexicons, lexicon_changed, reference_preference, reference_preference_changed, references };
  bool accept(xc_status status, xc_request *request, Operation operation);
  void history();
  bool take_preview(xc_request *request, bool hover);
  void poll_hover();
  void load_chapters(bool commentary = false);
  void load_choices();
  xc_worker *worker_ = nullptr;
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
  std::string notice_;
  Dictionary dictionary_;
  std::vector<std::string> dictionary_keys_;
  bool dictionary_back_ = false, dictionary_forward_ = false;
  size_t dictionary_history_count_ = 0;
  Chapter bible_chapter_, commentary_chapter_;
  bool show_chapter_ = false, show_commentary_chapter_ = false;
  unsigned display_flags_ = XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED;
  std::vector<ReferenceChoice> choices_[3];
  std::string choice_labels_[3];
  std::string error_;
  unsigned revision_ = 0;
};
}
