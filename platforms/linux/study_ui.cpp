// SPDX-License-Identifier: GPL-2.0-or-later
#include "study_ui.hpp"
#include "reader_document.hpp"
#include <cstring>
#include <png.h>
#include <memory>
namespace xiphos::linux_preview {
namespace {
struct BookAnchorState {
  std::optional<std::string> anchor;
  uint64_t revision = 0;
  GWeakRef status;
  explicit BookAnchorState(GObject *label) { g_weak_ref_init(&status, label); }
  ~BookAnchorState() { g_weak_ref_clear(&status); }
};
GtkWidget *box(GtkWidget *parent, bool horizontal = false) {
  auto *w = gtk_box_new(horizontal ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL, 4);
  if (parent) gtk_box_append(GTK_BOX(parent), w); return w;
}
GtkWidget *label(GtkWidget *parent, const char *text) {
  auto *w = gtk_label_new(text); gtk_label_set_wrap(GTK_LABEL(w), TRUE); gtk_box_append(GTK_BOX(parent), w); return w;
}
GtkWidget *entry(GtkWidget *parent, const char *name) {
  auto *w = gtk_entry_new(); gtk_entry_set_placeholder_text(GTK_ENTRY(w), name);
  gtk_accessible_update_property(GTK_ACCESSIBLE(w), GTK_ACCESSIBLE_PROPERTY_LABEL, name, -1);
  gtk_box_append(GTK_BOX(parent), w); return w;
}
GtkWidget *check(GtkWidget *parent, const char *name, bool active = false) {
  auto *w = gtk_check_button_new_with_label(name); gtk_check_button_set_active(GTK_CHECK_BUTTON(w), active); gtk_box_append(GTK_BOX(parent), w); return w;
}
GtkWidget *button(GtkWidget *parent, const char *name, GCallback cb, gpointer data) {
  auto *w = gtk_button_new_with_label(name); gtk_box_append(GTK_BOX(parent), w); g_signal_connect(w, "clicked", cb, data); return w;
}
GtkWidget *choice(GtkWidget *parent, const char *name, const char *const *values = nullptr) {
  auto *w = values ? gtk_drop_down_new_from_strings(values) : gtk_drop_down_new(nullptr, nullptr);
  gtk_accessible_update_property(GTK_ACCESSIBLE(w), GTK_ACCESSIBLE_PROPERTY_LABEL, name, -1); gtk_box_append(GTK_BOX(parent), w); return w;
}
bool active(GtkWidget *w) { return gtk_check_button_get_active(GTK_CHECK_BUTTON(w)); }
std::string text(GtkWidget *w) { return gtk_editable_get_text(GTK_EDITABLE(w)); }
void choices(GtkWidget *w, const std::vector<std::string> &names) {
  auto *list = gtk_string_list_new(nullptr); for (const auto &name : names) gtk_string_list_append(list, name.c_str());
  gtk_drop_down_set_model(GTK_DROP_DOWN(w), G_LIST_MODEL(list)); g_object_unref(list);
}
}
StudyUI::StudyUI(GtkWindow *parent, bool software) : parent_(parent), software_(software) {}
StudyUI::~StudyUI() { close(); }
void StudyUI::close() {
  closed_ = true; model_ = nullptr;
  if (dialog_) { g_signal_handlers_disconnect_by_data(dialog_, this); gtk_native_dialog_hide(dialog_); g_object_unref(dialog_); dialog_ = nullptr; }
  if (image_window_) { gtk_window_destroy(image_window_); image_window_ = nullptr; }
}
GtkWidget *StudyUI::view(WebKitWebView **out, bool book) {
  auto *session = webkit_network_session_new_ephemeral();
  *out = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW, "network-session", session, nullptr)); g_object_unref(session);
  webkit_settings_set_enable_javascript(webkit_web_view_get_settings(*out), FALSE);
  if (software_) webkit_settings_set_hardware_acceleration_policy(webkit_web_view_get_settings(*out), WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);
  g_object_set_data(G_OBJECT(*out), "book", GINT_TO_POINTER(book));
  g_signal_connect(*out, "decide-policy", G_CALLBACK((+[](WebKitWebView *view, WebKitPolicyDecision *decision, WebKitPolicyDecisionType type, gpointer data) -> gboolean {
    auto *self = static_cast<StudyUI *>(data);
    if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION || type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION) {
      auto *action = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));
      const char *uri = webkit_uri_request_get_uri(webkit_navigation_action_get_request(action));
      if (webkit_navigation_action_get_navigation_type(action) == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED) {
        webkit_policy_decision_ignore(decision);
        if (!self->closed_ && self->model_ && uri) {
          if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(view), "book"))) self->model_->follow_book(uri);
          else { const auto &p = self->model_->search_preview(); self->model_->follow(p.module, uri, p.reference); }
        }
        return TRUE;
      }
      if (!uri || (std::string(uri) != "xiphos-preview://study/" && std::string(uri) != "about:blank")) { webkit_policy_decision_ignore(decision); return TRUE; }
    }
    return FALSE;
  })), this);
  gtk_widget_set_vexpand(GTK_WIDGET(*out), TRUE); gtk_widget_set_size_request(GTK_WIDGET(*out), 200, 130);
  webkit_web_view_load_html(*out, document("").c_str(), "xiphos-preview://study/"); return GTK_WIDGET(*out);
}
GtkWidget *StudyUI::book_pane() {
  book_box_ = box(nullptr); book_module_ = choice(book_box_, "General book module");
  g_signal_connect(book_module_, "notify::selected", G_CALLBACK((+[](GObject *w, GParamSpec *, gpointer data) {
    auto *s = static_cast<StudyUI *>(data); auto i = gtk_drop_down_get_selected(GTK_DROP_DOWN(w));
    if (!s->updating_ && s->model_ && i < s->books_.size()) s->model_->choose_book(s->books_[i]);
  })), this);
  auto *bar = box(book_box_, true); book_key_ = entry(bar, "Book key or path"); gtk_widget_set_hexpand(book_key_, TRUE);
  book_read_ = button(bar, "Read", G_CALLBACK((+[](GtkButton *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->model_) s->model_->read_book_key(text(s->book_key_)); })), this);
  g_signal_connect(book_key_, "activate", G_CALLBACK((+[](GtkEntry *, gpointer data) { auto *s = static_cast<StudyUI *>(data); g_signal_emit_by_name(s->book_read_, "clicked"); })), this);
  bar = box(book_box_, true); const char *names[] = {"Parent", "Child", "Previous", "Next"};
  for (int i = 0; i < 4; ++i) {
    book_buttons_[i] = button(bar, names[i], G_CALLBACK((+[](GtkButton *w, gpointer data) {
      auto *s = static_cast<StudyUI *>(data); if (s->model_) s->model_->navigate_book(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "direction")));
    })), this); g_object_set_data(G_OBJECT(book_buttons_[i]), "direction", GINT_TO_POINTER(i));
  }
  bar = box(book_box_, true);
  button(bar, "Contents", G_CALLBACK((+[](GtkButton *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->model_) s->model_->browse_book(); })), this);
  button(bar, "Up a level", G_CALLBACK((+[](GtkButton *, gpointer data) {
    auto *s = static_cast<StudyUI *>(data); if (s->model_ && s->model_->book_contents().up.present) s->model_->browse_book(s->model_->book_contents().up.offset);
  })), this);
  book_more_ = button(bar, "More sections", G_CALLBACK((+[](GtkButton *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->model_) s->model_->browse_book(0, true); })), this);
  bar = box(book_box_, true);
  button(bar, "Restore…", G_CALLBACK((+[](GtkButton *, gpointer data) { static_cast<StudyUI *>(data)->pick(1); })), this);
  button(bar, "Close", G_CALLBACK((+[](GtkButton *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->model_) s->model_->close_book(); })), this);
  book_contents_ = gtk_list_box_new(); gtk_list_box_set_selection_mode(GTK_LIST_BOX(book_contents_), GTK_SELECTION_SINGLE);
  auto *scroll = gtk_scrolled_window_new(); gtk_widget_set_size_request(scroll, -1, 110); gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), book_contents_); gtk_box_append(GTK_BOX(book_box_), scroll);
  gtk_accessible_update_property(GTK_ACCESSIBLE(book_contents_), GTK_ACCESSIBLE_PROPERTY_LABEL, "Book contents: activate to read or browse", -1);
  g_signal_connect(book_contents_, "row-activated", G_CALLBACK((+[](GtkListBox *, GtkListBoxRow *row, gpointer data) {
    auto *s = static_cast<StudyUI *>(data); if (!s->model_) return;
    const auto i = gtk_list_box_row_get_index(row); const auto &nodes = s->model_->book_contents().nodes;
    if (i >= 0 && size_t(i) < nodes.size()) s->model_->read_book_node(nodes[i].offset);
  })), this);
  bar = box(book_box_, true);
  button(bar, "Browse selected", G_CALLBACK((+[](GtkButton *, gpointer data) {
    auto *s = static_cast<StudyUI *>(data); if (!s->model_) return;
    auto *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(s->book_contents_)); if (!row) return;
    auto i = gtk_list_box_row_get_index(row); const auto &nodes = s->model_->book_contents().nodes;
    if (i >= 0 && size_t(i) < nodes.size() && nodes[i].children) s->model_->browse_book(nodes[i].offset);
  })), this);
  button(bar, "Refresh", G_CALLBACK((+[](GtkButton *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->model_) s->model_->refresh_book(); })), this);
  const char *options[] = {"Images", "Font faces", "Double space", "Note numbers"};
  for (int i = 0; i < 4; ++i) {
    if (i % 2 == 0) bar = box(book_box_, true);
    book_options_[i] = check(bar, options[i]); g_object_set_data(G_OBJECT(book_options_[i]), "option", GINT_TO_POINTER(i));
    g_signal_connect(book_options_[i], "toggled", G_CALLBACK((+[](GtkCheckButton *w, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (!s->updating_ && s->model_) s->model_->book_option(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "option")), gtk_check_button_get_active(w)); })), this);
  }
  book_label_ = label(book_box_, ""); gtk_box_append(GTK_BOX(book_box_), view(&book_view_, true)); book_status_ = label(book_box_, "");
  g_object_set_data_full(G_OBJECT(book_view_), "book-anchor-state", new BookAnchorState(G_OBJECT(book_status_)),
      +[](gpointer p) { delete static_cast<BookAnchorState *>(p); });
  g_signal_connect(book_view_, "load-changed", G_CALLBACK(+[](WebKitWebView *view, WebKitLoadEvent event, gpointer) {
    if (event != WEBKIT_LOAD_FINISHED) return;
    auto *state = static_cast<BookAnchorState *>(g_object_get_data(G_OBJECT(view), "book-anchor-state"));
    if (!state || !state->anchor) return;
    const auto script = anchor_scroll_script(*state->anchor);
    auto *revision = new uint64_t(state->revision);
    webkit_web_view_evaluate_javascript(view, script.c_str(), -1, "xiphos-host", nullptr, nullptr,
        +[](GObject *source, GAsyncResult *result, gpointer data) {
          std::unique_ptr<uint64_t> revision(static_cast<uint64_t *>(data));
          GError *error = nullptr;
          auto *value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(source), result, &error);
          auto *current = static_cast<BookAnchorState *>(g_object_get_data(source, "book-anchor-state"));
          if (current && current->revision == *revision && (!value || error || !jsc_value_to_boolean(value))) {
            auto *status = g_weak_ref_get(&current->status);
            if (status) { gtk_label_set_text(GTK_LABEL(status), "Unable to scroll to the selected book section."); g_object_unref(status); }
          }
          g_clear_object(&value); g_clear_error(&error);
        }, revision);
  }), nullptr);
  return book_box_;
}
GtkWidget *StudyUI::search_pane() {
  auto *outer = box(nullptr); auto *scroll = gtk_scrolled_window_new(); gtk_widget_set_vexpand(scroll, TRUE); gtk_box_append(GTK_BOX(outer), scroll);
  search_box_ = box(nullptr); gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), search_box_);
  label(search_box_, "Ordered modules (comma-separated; spaces are literal)"); search_modules_ = entry(search_box_, "Search modules");
  search_import_ = button(search_box_, "Import saved search lists…", G_CALLBACK((+[](GtkButton *, gpointer data) { static_cast<StudyUI *>(data)->pick(0); })), this);
  auto *bar = box(search_box_, true); saved_modules_ = choice(bar, "Saved module list");
  apply_modules_ = button(bar, "Apply modules", G_CALLBACK((+[](GtkButton *, gpointer data) {
    auto *s = static_cast<StudyUI *>(data); if (!s->model_) return; auto i = gtk_drop_down_get_selected(GTK_DROP_DOWN(s->saved_modules_)); const auto &l = s->model_->saved_lists(false);
    if (i < l.size() && l[i].label && l[i].effective < l.size() && l[l[i].effective].value) s->model_->parse_search_modules(*l[l[i].effective].value);
  })), this);
  bar = box(search_box_, true); saved_ranges_ = choice(bar, "Saved search range");
  apply_range_ = button(bar, "Apply range", G_CALLBACK((+[](GtkButton *, gpointer data) {
    auto *s = static_cast<StudyUI *>(data); if (!s->model_) return; auto i = gtk_drop_down_get_selected(GTK_DROP_DOWN(s->saved_ranges_)); const auto &l = s->model_->saved_lists(true);
    if (i < l.size() && l[i].label && l[i].effective < l.size() && l[l[i].effective].value) { gtk_editable_set_text(GTK_EDITABLE(s->range_), l[l[i].effective].value->c_str()); gtk_drop_down_set_selected(GTK_DROP_DOWN(s->search_scope_), XC_SEARCH_SCOPE_BOUNDS); }
  })), this);
  search_text_ = entry(search_box_, "Search text");
  const char *modes[] = {"Regular expression", "Phrase", "Words (automatic)", "Lemma", "Morphology", "Footnote text", "Multiword", "Indexed query", nullptr};
  search_mode_ = choice(search_box_, "Search match mode", modes); gtk_drop_down_set_selected(GTK_DROP_DOWN(search_mode_), XC_SEARCH_PHRASE);
  const char *scopes[] = {"All", "Bounds", "Within last results", nullptr}; search_scope_ = choice(search_box_, "Search scope", scopes);
  range_module_ = choice(search_box_, "Bounds provider module"); range_ = entry(search_box_, "Search bounds, e.g. John 1:1 - John 3:16");
  case_ = check(search_box_, "Case sensitive"); annotations_ = check(search_box_, "Set annotation options");
  bar = box(search_box_, true); annotation_[0] = check(bar, "Strong’s"); annotation_[1] = check(bar, "Morphology"); annotation_[2] = check(bar, "Footnotes");
  search_reset_ = button(search_box_, "Reset search annotations", G_CALLBACK((+[](GtkButton *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->model_ && s->model_->reset_search_annotations()) gtk_check_button_set_active(GTK_CHECK_BUTTON(s->annotations_), FALSE); })), this);
  bar = box(search_box_, true);
  search_start_ = button(bar, "Search", G_CALLBACK((+[](GtkButton *, gpointer data) { static_cast<StudyUI *>(data)->start_search(); })), this);
  search_stop_ = button(bar, "Stop", G_CALLBACK((+[](GtkButton *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->model_) s->model_->cancel_search(); })), this);
  search_clear_ = button(bar, "Clear last", G_CALLBACK((+[](GtkButton *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->model_) s->model_->clear_search_scope(); })), this);
  g_signal_connect(search_text_, "activate", G_CALLBACK((+[](GtkEntry *, gpointer data) { static_cast<StudyUI *>(data)->start_search(); })), this);
  progress_ = gtk_progress_bar_new(); gtk_box_append(GTK_BOX(search_box_), progress_); search_status_ = label(search_box_, "");
  results_ = gtk_list_box_new(); gtk_list_box_set_selection_mode(GTK_LIST_BOX(results_), GTK_SELECTION_MULTIPLE);
  gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(results_), FALSE);
  auto *results_scroll = gtk_scrolled_window_new(); gtk_widget_set_size_request(results_scroll, -1, 150); gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(results_scroll), results_); gtk_box_append(GTK_BOX(search_box_), results_scroll);
  gtk_accessible_update_property(GTK_ACCESSIBLE(results_), GTK_ACCESSIBLE_PROPERTY_LABEL, "Search results: select to preview, activate to read", -1);
  g_signal_connect(results_, "selected-rows-changed", G_CALLBACK((+[](GtkListBox *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->updating_ || !s->model_) return; auto rows = s->selection(); if (!rows.empty()) s->model_->preview_search_hit(rows.front()); })), this);
  g_signal_connect(results_, "row-activated", G_CALLBACK((+[](GtkListBox *, GtkListBoxRow *row, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->model_) s->model_->activate_search_hit(gtk_list_box_row_get_index(row)); })), this);
  open_ = button(search_box_, "Read selected passage", G_CALLBACK((+[](GtkButton *, gpointer data) { auto *s = static_cast<StudyUI *>(data); auto rows = s->selection(); if (s->model_ && !rows.empty()) s->model_->activate_search_hit(rows.front()); })), this);
  gtk_box_append(GTK_BOX(search_box_), view(&preview_view_, false));
  export_html_ = check(search_box_, "HTML export"); export_all_ = check(search_box_, "Export all retained matches"); export_scripture_ = check(search_box_, "Include scripture", true);
  export_context_ = check(search_box_, "Include search details"); export_images_ = check(search_box_, "Embed PNG images"); export_partial_ = check(search_box_, "Export returned matches from incomplete search");
  bar = box(search_box_, true); copy_ = button(bar, "Copy", G_CALLBACK((+[](GtkButton *, gpointer data) { static_cast<StudyUI *>(data)->export_results(false); })), this);
  save_ = button(bar, "Save…", G_CALLBACK((+[](GtkButton *, gpointer data) { static_cast<StudyUI *>(data)->export_results(true); })), this);
  return outer;
}
std::vector<size_t> StudyUI::selection() const {
  std::vector<size_t> result; auto *rows = gtk_list_box_get_selected_rows(GTK_LIST_BOX(results_));
  for (auto *p = rows; p; p = p->next) result.push_back(gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(p->data)));
  g_list_free(rows); return result;
}
void StudyUI::start_search() {
  if (!model_ || closed_ || model_->busy()) return;
  // Tokenization is performed by the shared core; the next tick submits the
  // search after copied names arrive. No native module-list parser.
  if (model_->parse_search_modules(text(search_modules_))) g_object_set_data(G_OBJECT(search_text_), "start-after-parse", GINT_TO_POINTER(1));
}
void StudyUI::export_results(bool save) {
  if (!model_ || closed_ || model_->busy() || dialog_) return;
  const bool context = active(export_context_);
  if (model_->export_search(active(export_html_), active(export_all_), selection(), active(export_scripture_), context,
                            active(export_images_), context && active(export_partial_))) export_save_ = save;
}
void StudyUI::pick(int kind) {
  if (closed_ || dialog_ || !model_ || model_->busy()) return;
  dialog_ = GTK_NATIVE_DIALOG(gtk_file_chooser_native_new(kind == 0 ? "Import saved search lists" : kind == 1 ? "Restore book selection" : "Save search results", parent_,
    kind == 2 ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN, kind == 2 ? "Save" : "Import", "Cancel"));
  g_object_set_data(G_OBJECT(dialog_), "kind", GINT_TO_POINTER(kind));
  if (kind == 2) gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog_), pending_export_html_ ? "search.html" : "search.txt");
  g_signal_connect(dialog_, "response", G_CALLBACK((+[](GtkNativeDialog *dialog, int response, gpointer data) {
    auto *s = static_cast<StudyUI *>(data); int kind = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "kind"));
    if (!s->closed_ && response == GTK_RESPONSE_ACCEPT) {
      auto *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog)); char *path = file ? g_file_get_path(file) : nullptr;
      if (kind == 0 && path) s->model_->import_search_lists(path);
      else if (kind == 1 && path) s->model_->restore_book(path);
      else if (kind == 2 && file) {
        // Async native replacement owns its payload and application hold. The
        // originating window may close before completion.
        struct Write { std::string bytes; GApplication *app; GWeakRef status; };
        auto *job = new Write{s->pending_export_, G_APPLICATION(gtk_window_get_application(s->parent_)), {}};
        g_weak_ref_init(&job->status, G_OBJECT(s->search_status_)); g_application_hold(job->app);
        g_file_replace_contents_async(file, job->bytes.data(), job->bytes.size(), nullptr, FALSE, G_FILE_CREATE_NONE, nullptr,
          +[](GObject *file, GAsyncResult *result, gpointer data) {
            auto *job = static_cast<Write *>(data); GError *error = nullptr;
            const bool ok = g_file_replace_contents_finish(G_FILE(file), result, nullptr, &error);
            auto *status = g_weak_ref_get(&job->status);
            if (status) { gtk_label_set_text(GTK_LABEL(status), ok ? "Search results saved." : (error ? error->message : "Save failed.")); g_object_unref(status); }
            g_clear_error(&error); g_weak_ref_clear(&job->status); g_application_release(job->app); delete job;
          }, job);
      }
      g_free(path); if (file) g_object_unref(file);
    }
    s->dialog_ = nullptr; gtk_native_dialog_hide(dialog); g_object_unref(dialog);
  })), this); gtk_native_dialog_show(dialog_);
}
void StudyUI::show_image(const Image &image) {
  // Match the current Mac PNG preview and bound decoded dimensions before decode.
  png_image header{}; header.version = PNG_IMAGE_VERSION;
  const bool readable = !image.bytes.empty() && png_image_begin_read_from_memory(&header, image.bytes.data(), image.bytes.size());
  const auto width = header.width, height = header.height; png_image_free(&header);
  if (!readable || !width || !height || uint64_t(width) * height > 16 * 1024 * 1024) {
    gtk_label_set_text(GTK_LABEL(book_status_), "The image is not a supported bounded PNG."); return;
  }
  auto *bytes = g_bytes_new(image.bytes.data(), image.bytes.size()); GError *error = nullptr;
  auto *texture = gdk_texture_new_from_bytes(bytes, &error); g_bytes_unref(bytes);
  if (!texture) { gtk_label_set_text(GTK_LABEL(book_status_), error ? error->message : "Image decode failed."); g_clear_error(&error); return; }
  if (image_window_) gtk_window_destroy(image_window_);
  image_window_ = GTK_WINDOW(gtk_window_new()); gtk_window_set_transient_for(image_window_, parent_); gtk_window_set_destroy_with_parent(image_window_, TRUE);
  gtk_window_set_title(image_window_, image.source.c_str()); gtk_window_set_default_size(image_window_, 600, 500);
  auto *picture = gtk_picture_new_for_paintable(GDK_PAINTABLE(texture)); g_object_unref(texture);
  gtk_picture_set_can_shrink(GTK_PICTURE(picture), TRUE); gtk_window_set_child(image_window_, picture);
  // Keep the pointer valid until close()/replacement, including user close.
  gtk_window_set_hide_on_close(image_window_, TRUE); gtk_window_present(image_window_);
}
void StudyUI::tick(ReaderModel &model) {
  if (closed_) return; model_ = &model; updating_ = true;
  bool enabled = model.ready() && !model.busy();
  if (model.ready() && !populated_) {
    for (const auto &m : model.modules()) { if (m.book) books_.push_back(m.id); if (m.search) search_ids_.push_back(m.id); }
    choices(book_module_, books_); gtk_drop_down_set_selected(GTK_DROP_DOWN(book_module_), GTK_INVALID_LIST_POSITION);
    choices(range_module_, search_ids_); if (!search_ids_.empty()) gtk_editable_set_text(GTK_EDITABLE(search_modules_), search_ids_.front().c_str());
    populated_ = true;
  }
  const auto &book = model.book();
  const auto identity = book.module + book.nodes[0].path + book.html + book.css + (book.anchor ? *book.anchor : "") + std::to_string(book.anchor_revision);
  if (identity != book_identity_) {
    if (auto *state = static_cast<BookAnchorState *>(g_object_get_data(G_OBJECT(book_view_), "book-anchor-state"))) {
      state->anchor = book.anchor; ++state->revision;
    }
    book_identity_ = identity; gtk_editable_set_text(GTK_EDITABLE(book_key_), book.nodes[0].path.c_str()); gtk_label_set_text(GTK_LABEL(book_label_), book.nodes[0].path.c_str());
    webkit_web_view_load_html(book_view_, document(book.html, book.css, true).c_str(), "xiphos-preview://study/");
    gtk_label_set_text(GTK_LABEL(book_status_), book.issue.c_str());
    for (size_t i = 0; i < books_.size(); ++i) if (books_[i] == book.module) gtk_drop_down_set_selected(GTK_DROP_DOWN(book_module_), i);
  }
  std::string contents_identity; const auto &contents = model.book_contents();
  for (const auto &n : contents.nodes) contents_identity += std::to_string(n.offset) + n.title;
  contents_identity += contents.module + std::to_string(contents.parent.offset);
  if (contents_identity != contents_identity_) {
    contents_identity_ = contents_identity; while (auto *row = gtk_widget_get_first_child(book_contents_)) gtk_list_box_remove(GTK_LIST_BOX(book_contents_), row);
    for (const auto &n : contents.nodes) { auto *l = gtk_label_new((n.title + (n.children ? " ▸" : "")).c_str()); gtk_label_set_xalign(GTK_LABEL(l), 0); gtk_list_box_append(GTK_LIST_BOX(book_contents_), l); }
  }
  gtk_widget_set_sensitive(book_box_, enabled); gtk_widget_set_sensitive(book_more_, enabled && contents.after.has_value());
  for (int i = 0; i < 4; ++i) gtk_widget_set_sensitive(book_buttons_[i], enabled && book.nodes[i + 1].present);
  int options[] = {book.images, book.fonts, book.spacing, book.numbers};
  for (int i = 0; i < 4; ++i) { gtk_check_button_set_active(GTK_CHECK_BUTTON(book_options_[i]), options[i] == 1); gtk_widget_set_sensitive(book_options_[i], enabled && options[i] >= 0 && !book.module.empty()); }
  const auto &result = model.search_results();
  if (result.revision != results_revision_) {
    results_revision_ = result.revision; gtk_check_button_set_active(GTK_CHECK_BUTTON(export_partial_), FALSE);
    while (auto *row = gtk_widget_get_first_child(results_)) gtk_list_box_remove(GTK_LIST_BOX(results_), row);
    for (const auto &hit : result.hits) { auto *l = gtk_label_new((hit.module + ": " + hit.reference).c_str()); gtk_label_set_xalign(GTK_LABEL(l), 0); gtk_list_box_append(GTK_LIST_BOX(results_), l); }
    std::string status = std::to_string(result.generation.matched_elements) + " SDK matches; " + std::to_string(result.hits.size()) + " shown.";
    for (const auto &g : result.groups) status += "\n" + g.module + ": " + (g.status == XC_OK ? std::to_string(g.info.total_results) + " matches" : "failed (" + std::to_string(g.status) + ")");
    if (result.cancelled) status += "\nSearch stopped; returned results may be incomplete.";
    if (result.limited) status += "\nVisible preview limit reached.";
    gtk_label_set_text(GTK_LABEL(search_status_), status.c_str());
  }
  const auto &preview = model.search_preview(); auto preview_id = preview.module + preview.reference + preview.html;
  if (preview_identity_ != preview_id) { preview_identity_ = preview_id; webkit_web_view_load_html(preview_view_, document(preview.html).c_str(), "xiphos-preview://study/"); }
  std::string lists_id; for (int k = 0; k < 2; ++k) for (const auto &v : model.saved_lists(k)) lists_id += (v.label ? "label:" + *v.label : "absent") + (v.value ? "value:" + *v.value : "absent");
  if (lists_id != lists_identity_) { lists_identity_ = lists_id; for (int k = 0; k < 2; ++k) { std::vector<std::string> names; for (const auto &v : model.saved_lists(k)) names.push_back(v.label ? (v.label->empty() ? "(Empty label)" : *v.label) : "(Unlabelled)"); choices(k ? saved_ranges_ : saved_modules_, names); } }
  if (model.parsed_search_revision() != parsed_revision_) {
    parsed_revision_ = model.parsed_search_revision();
    if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(search_text_), "start-after-parse"))) {
      g_object_set_data(G_OBJECT(search_text_), "start-after-parse", nullptr);
      SearchInput input; input.modules = model.parsed_search_modules(); input.expression = text(search_text_); input.mode = gtk_drop_down_get_selected(GTK_DROP_DOWN(search_mode_));
      input.scope = static_cast<xc_search_scope_kind>(gtk_drop_down_get_selected(GTK_DROP_DOWN(search_scope_))); input.range = text(range_);
      auto i = gtk_drop_down_get_selected(GTK_DROP_DOWN(range_module_)); if (i < search_ids_.size()) input.range_module = search_ids_[i];
      input.case_sensitive = active(case_); if (active(annotations_) && input.mode != XC_SEARCH_PHRASE) input.annotations = xc_search_text_options{active(annotation_[0]), active(annotation_[1]), active(annotation_[2])};
      if (!model.search(input)) gtk_label_set_text(GTK_LABEL(search_status_), model.error().empty() ? "Choose at least one module and enter a search expression." : model.error().c_str());
    } else {
      std::string names; for (const auto &m : model.parsed_search_modules()) { if (!names.empty()) names += ","; names += m; } gtk_editable_set_text(GTK_EDITABLE(search_modules_), names.c_str());
    }
  }
  const auto &output = model.search_export();
  if (output.revision != export_revision_) {
    export_revision_ = output.revision;
    if (output.disposition == XC_SEARCH_EXPORT_OK) {
      pending_export_html_ = !output.html.empty();
      pending_export_ = pending_export_html_ ? output.html : output.text;
      if (export_save_) pick(2);
      else {
        auto *clipboard = gtk_widget_get_clipboard(GTK_WIDGET(parent_));
        if (output.html.empty()) gdk_clipboard_set_text(clipboard, output.text.c_str());
        else {
          auto *html = g_bytes_new(output.html.data(), output.html.size()), *plain = g_bytes_new(output.text.data(), output.text.size());
          GdkContentProvider *providers[] = {gdk_content_provider_new_for_bytes("text/html", html), gdk_content_provider_new_for_bytes("text/plain;charset=utf-8", plain)};
          auto *provider = gdk_content_provider_new_union(providers, 2); gdk_clipboard_set_content(clipboard, provider); g_object_unref(provider); g_bytes_unref(html); g_bytes_unref(plain);
        }
        gtk_label_set_text(GTK_LABEL(search_status_), "Search results copied.");
      }
    } else gtk_label_set_text(GTK_LABEL(search_status_), model.error().c_str());
  }
  if (model.image().revision != image_revision_) { image_revision_ = model.image().revision; show_image(model.image()); }
  enabled = model.ready() && !model.busy();
  const auto progress = model.search_progress(); gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_), progress.percent / 100.0);
  // Keep Stop reachable while the shared operation is running.
  for (auto *w : {search_modules_, search_text_, search_mode_, search_scope_, range_module_, range_, case_, annotations_, search_start_, search_import_, search_reset_, saved_modules_, saved_ranges_, apply_modules_, apply_range_}) gtk_widget_set_sensitive(w, enabled);
  for (auto *w : annotation_) gtk_widget_set_sensitive(w, enabled && active(annotations_) && gtk_drop_down_get_selected(GTK_DROP_DOWN(search_mode_)) != XC_SEARCH_PHRASE);
  gtk_widget_set_sensitive(search_start_, enabled && !text(search_modules_).empty() && !text(search_text_).empty());
  gtk_widget_set_sensitive(search_stop_, model.busy()); gtk_widget_set_sensitive(search_clear_, enabled && result.scope.present);
  gtk_widget_set_sensitive(open_, enabled && !selection().empty());
  for (auto *w : {copy_, save_}) gtk_widget_set_sensitive(w, enabled && !dialog_ && result.generation.id && (active(export_all_) || !selection().empty()));
  gtk_widget_set_sensitive(export_images_, enabled && active(export_html_) && active(export_scripture_));
  gtk_widget_set_sensitive(export_partial_, enabled && active(export_context_) && (result.generation.cancelled || result.generation.failed_groups || result.generation.group_count < result.generation.requested_count));
  if (enabled && !model.error().empty()) g_object_set_data(G_OBJECT(search_text_), "start-after-parse", nullptr);
  updating_ = false;
}
}
