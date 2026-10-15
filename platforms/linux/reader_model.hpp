// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/c_api.h"
#include <cstdint>
#include <string>
#include <vector>

namespace xiphos::linux_preview {
struct Module { std::string id, description, type; bool dictionary = false; };
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
  bool follow(const std::string &source, const std::string &uri);
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
  enum class Operation { catalog, tab, preferences, passage, history, link, refresh, clear, presentation, bible_chapter, commentary_chapter, choices, dictionary, dictionary_choices, dictionary_history, dictionary_clear };
  bool accept(xc_status status, xc_request *request, Operation operation);
  void history();
  void load_chapters(bool commentary = false);
  void load_choices();
  xc_worker *worker_ = nullptr;
  xc_request *request_ = nullptr;
  uint64_t tab_ = 0;
  Operation operation_ = Operation::catalog;
  bool ready_ = false, closed_ = false, back_ = false, forward_ = false;
  std::vector<Module> modules_;
  Passage passage_;
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
