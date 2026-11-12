// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "reader_model.hpp"
#include <map>
#include "recovery_model.hpp"
#include "rich_editor.hpp"
#include <gtk/gtk.h>
#include <memory>
#include <webkit/webkit.h>
namespace xiphos::linux_preview {
// Presentation-only host. ReaderModel owns submissions, copied results and tab state.
class StudyUI {
  friend struct StudyUISmoke;
public:
  StudyUI(GtkWindow *parent, bool software);
  ~StudyUI();
  GtkWidget *book_pane();
  GtkWidget *parallel_pane();
  void study_parallel_selection();
  GtkWidget *devotional_pane();
  GtkWidget *reading_style_menu();
  std::string reading_style_css(const std::string &module, const std::string &selector = "body") const;
  unsigned style_revision() const { return style_revision_; }
  std::function<void()> show_search, show_parallel;
  GtkWidget *notes_pane();
  GtkWidget *editor_pane();
  bool confirm_close();
  std::vector<RecoveryDraft> recovery_drafts(const std::string &key) const;
  bool recover_draft(const RecoveryDraft &draft);
  void cancel_close() { discard_on_close_ = false; }
  std::function<void(bool)> on_close_decision;
  std::string capture_state(const std::string &reference_draft = "") const;
  static std::optional<std::string> reference_draft(const std::string &state);
  static bool validate_state(const std::string &state);
  bool restore_state(const std::string &state);
  GtkWidget *search_pane();
  void tick(ReaderModel &model);
  void close();
private:
  std::vector<std::pair<int, std::string>> restore_sources_;
  GtkWindow *parent_;
  ReaderModel *model_ = nullptr;
  bool software_, updating_ = false, closed_ = false, populated_ = false;
  GtkWidget *notes_box_ = nullptr, *editor_box_ = nullptr, *bookmark_rows_ = nullptr, *bookmark_fields_[6] = {}, *bookmark_unset_[6] = {}, *bookmark_parent_ = nullptr, *bookmark_position_ = nullptr;
  GtkWidget *import_folder_ = nullptr, *import_policy_ = nullptr, *import_review_ = nullptr, *note_remove_ = nullptr;
  GtkWidget *note_text_ = nullptr, *note_generic_ = nullptr, *note_label_ = nullptr, *repository_status_ = nullptr, *remove_confirm_ = nullptr;
  std::unique_ptr<RichEditor> rich_editor_;
  GtkWidget *editor_stack_ = nullptr, *format_value_ = nullptr;
  bool rich_mode_ = false;
  GtkWidget *editor_text_ = nullptr, *editor_module_ = nullptr, *editor_reference_ = nullptr, *editor_label_ = nullptr, *editor_preview_box_ = nullptr;
  WebKitWebView *editor_preview_ = nullptr;
  GtkWidget *close_dialog_ = nullptr;
  bool discard_on_close_ = false;
  unsigned catalog_version_ = 0;
  unsigned bookmarks_version_ = 0, note_version_ = 0, editor_version_ = 0;
  std::string decoration_identity_[3];
  void update_repositories();
  void select_bookmark();
  GtkWidget *parallel_box_ = nullptr, *parallel_modules_ = nullptr, *parallel_reference_ = nullptr, *parallel_sets_ = nullptr, *parallel_name_ = nullptr, *parallel_confirm_ = nullptr, *parallel_default_ = nullptr;
  WebKitWebView *parallel_view_ = nullptr;
  struct ReadingStyle { unsigned font = 0, size = 18; std::string foreground, background; };
  std::map<std::string, ReadingStyle> reading_styles_;
  GtkWidget *style_module_ = nullptr, *style_font_ = nullptr, *style_size_ = nullptr;
  GtkWidget *style_foreground_ = nullptr, *style_background_ = nullptr, *style_status_ = nullptr;
  std::vector<std::string> style_modules_;
  unsigned style_revision_ = 0;
  static bool valid_reading_color(const std::string &color);
  void load_reading_style();
  GtkWidget *parallel_layout_ = nullptr, *parallel_visibility_ = nullptr;
  GtkWidget *parallel_previous_verse_ = nullptr, *parallel_next_verse_ = nullptr;
  std::vector<std::string> parallel_order_;
  std::map<std::vector<std::string>, std::vector<bool>> parallel_hidden_;
  bool parallel_needs_render_ = false;
  void step_parallel_verse(int direction);
  unsigned parallel_revision_ = 0, requested_comparison_ = 0;
  GtkWidget *compare_result_ = nullptr;
  unsigned word_revision_ = 0;
  GtkWidget *devotional_module_ = nullptr, *devotional_date_ = nullptr, *devotional_status_ = nullptr;
  WebKitWebView *devotional_view_ = nullptr;
  std::vector<std::string> devotional_modules_;
  unsigned devotional_revision_ = 0;
  void read_devotional(bool today);
  GtkWidget *word_label_ = nullptr, *word_choices_ = nullptr, *word_box_ = nullptr;
  void search_selected_word();
  void update_selected_word();
  std::string parallel_profile_identity_;
  void update_parallel();
  GtkWidget *book_box_ = nullptr, *search_box_ = nullptr;
  GtkWidget *book_module_, *book_key_, *book_read_, *book_contents_, *book_more_, *book_label_, *book_status_, *book_buttons_[4], *book_options_[4];
  GtkWidget *mixed_, *first_key_, *last_key_, *list_label_;
  GtkWidget *search_modules_, *search_text_, *search_mode_, *search_scope_, *range_module_, *range_, *case_, *annotations_, *annotation_[3];
  GtkWidget *search_start_, *search_stop_, *search_clear_, *search_reset_, *search_import_, *saved_modules_, *saved_ranges_, *apply_modules_, *apply_range_;
  GtkWidget *results_, *search_status_, *progress_, *export_html_, *export_all_, *export_scripture_, *export_context_, *export_images_, *export_partial_, *copy_, *save_, *open_;
  WebKitWebView *book_view_, *preview_view_;
  GtkNativeDialog *dialog_ = nullptr;
  GtkWindow *image_window_ = nullptr;
  std::vector<std::string> books_, search_ids_;
  std::string book_identity_, contents_identity_, preview_identity_, lists_identity_, pending_export_;
  unsigned results_revision_ = 0, export_revision_ = 0, image_revision_ = 0, parsed_revision_ = 0;
  bool export_save_ = false, pending_export_html_ = false;
  std::vector<size_t> selection() const;
  GtkWidget *view(WebKitWebView **out, bool book);
  void start_search();
  void export_results(bool save);
  void pick(int kind); // 0 saved searches, 1 restore book, 2 save export
  void show_image(const Image &image);
};
}
