// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "reader_model.hpp"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
namespace xiphos::linux_preview {
// Presentation-only host. ReaderModel owns submissions, copied results and tab state.
class StudyUI {
  friend struct StudyUISmoke;
public:
  StudyUI(GtkWindow *parent, bool software);
  ~StudyUI();
  GtkWidget *book_pane();
  GtkWidget *search_pane();
  void tick(ReaderModel &model);
  void close();
private:
  GtkWindow *parent_;
  ReaderModel *model_ = nullptr;
  bool software_, updating_ = false, closed_ = false, populated_ = false;
  GtkWidget *book_box_ = nullptr, *search_box_ = nullptr;
  GtkWidget *book_module_, *book_key_, *book_read_, *book_contents_, *book_more_, *book_label_, *book_status_, *book_buttons_[4], *book_options_[4];
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
