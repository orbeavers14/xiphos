// SPDX-License-Identifier: GPL-2.0-or-later
#include "study_ui.hpp"
#include <algorithm>
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
	if (closed_)
		return;
	closed_ = true;
	model_ = nullptr;
	rich_editor_.reset();
	if (close_dialog_) {
		g_signal_handlers_disconnect_by_data(close_dialog_, this);
		gtk_window_destroy(GTK_WINDOW(close_dialog_));
		close_dialog_ = nullptr;
	}
	if (parallel_view_)
		g_signal_handlers_disconnect_by_data(webkit_web_view_get_user_content_manager(parallel_view_), this);
	if (dialog_) {
		g_signal_handlers_disconnect_by_data(dialog_, this);
		gtk_native_dialog_hide(dialog_);
		g_object_unref(dialog_);
		dialog_ = nullptr;
	}
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
	if (g_object_get_data(G_OBJECT(view), "parallel"))
		return TRUE;
	if (!self->closed_ && self->model_ && uri) {
          if (view == self->devotional_view_)
            self->model_->follow(self->model_->devotional().module, uri);
          else if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(view), "book"))) self->model_->follow_book(uri);
	  else
		  self->model_->follow_search_link(uri);
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
    if (!s->updating_ && s->model_ && i > 0 && i <= s->books_.size())
	    s->model_->choose_book(s->books_[i - 1]);
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
  list_label_ = entry(search_box_, "Saved choice name");
  bar = box(search_box_, true);
  const char *list_actions[] = {"Save modules…", "Save range…", "Remove modules…", "Remove range…"};
  for (int i = 0; i < 4; ++i) {
	  if (i == 2)
		  bar = box(search_box_, true);
	  auto *w = button(bar, list_actions[i], G_CALLBACK((+[](GtkButton *w, gpointer data) {
				   static_cast<StudyUI *>(data)->pick(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "kind")));
			   })),
			   this);
	  g_object_set_data(G_OBJECT(w), "kind", GINT_TO_POINTER(i + 3));
  }
  mixed_ = check(search_box_, "Include dictionaries and books");
  search_text_ = entry(search_box_, "Search text");
  const char *modes[] = {"Regular expression", "Phrase", "Words (automatic)", "Lemma", "Morphology", "Footnote text", "Multiword", "Indexed query", "Literal surface form", "Surface form without marks", nullptr};
  search_mode_ = choice(search_box_, "Search match mode", modes); gtk_drop_down_set_selected(GTK_DROP_DOWN(search_mode_), XC_SEARCH_PHRASE);
  word_box_ = box(search_box_);
  word_label_ = gtk_label_new("");
  gtk_label_set_wrap(GTK_LABEL(word_label_), TRUE);
  gtk_box_append(GTK_BOX(word_box_), word_label_);
  word_choices_ = choice(word_box_, "Selected word search choices");
  button(word_box_, "Search selected word", G_CALLBACK(+[](GtkButton *, gpointer data) {
    static_cast<StudyUI *>(data)->search_selected_word();
  }), this);
  auto *word_actions = box(word_box_, true);
  const char *actions[] = {"Speak", "Look up on map", "Translate"};
  for (unsigned i = 0; i < G_N_ELEMENTS(actions); ++i) {
    auto *action = button(word_actions, actions[i], G_CALLBACK(+[](GtkButton *button, gpointer data) {
      auto *ui = static_cast<StudyUI *>(data);
      if (!ui->model_)
        return;
      const auto &word = ui->model_->selected_word().surface;
      const auto index = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(button), "word-action"));
      if (word.empty() || word.size() > 4096)
        return;
      if (index == 0) {
        auto *speaker = g_find_program_in_path("spd-say");
        if (!speaker) {
          gtk_label_set_text(GTK_LABEL(ui->search_status_), "Speech Dispatcher (spd-say) is unavailable.");
          return;
        }
        GError *error = nullptr;
        auto *process = g_subprocess_new(G_SUBPROCESS_FLAGS_NONE, &error, speaker, "--", word.c_str(), nullptr);
        if (process)
          g_object_unref(process);
        else
          gtk_label_set_text(GTK_LABEL(ui->search_status_), error ? error->message : "Could not start speech.");
        g_clear_error(&error);
        g_free(speaker);
      } else {
        auto *escaped = g_uri_escape_string(word.c_str(), nullptr, FALSE);
        const std::string url = std::string(index == 1 ? "https://www.openstreetmap.org/search?query=" :
          "https://translate.google.com/?sl=auto&tl=en&op=translate&text=") + escaped;
        g_free(escaped);
        gtk_show_uri(ui->parent_, url.c_str(), GDK_CURRENT_TIME);
      }
    }), this);
    g_object_set_data(G_OBJECT(action), "word-action", GUINT_TO_POINTER(i));
  }
  gtk_widget_set_visible(word_box_, FALSE);
  const char *scopes[] = {"All", "Bounds", "Within last results", "Entry interval", nullptr};
  search_scope_ = choice(search_box_, "Search scope", scopes);
  range_module_ = choice(search_box_, "Bounds provider module"); range_ = entry(search_box_, "Search bounds, e.g. John 1:1 - John 3:16");
  first_key_ = entry(search_box_, "First exact entry key/path");
  last_key_ = entry(search_box_, "Last exact entry key/path");
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
  compare_result_ = button(search_box_, "Compare selected verse in default set", G_CALLBACK(+[](GtkButton *, gpointer data) {
    auto *ui = static_cast<StudyUI *>(data);
    const auto selected = ui->selection();
    if (ui->model_ && selected.size() == 1 && ui->model_->compare_search_hit(selected.front()))
      ui->requested_comparison_ = ui->model_->parallel().revision + 1;
  }), this);
  gtk_box_append(GTK_BOX(search_box_), view(&preview_view_, false));
  export_html_ = check(search_box_, "HTML export");
  export_all_ = check(search_box_, "Export all retained matches");
  export_scripture_ = check(search_box_, "Include entry text", true);
  export_context_ = check(search_box_, "Include search details");
  export_images_ = check(search_box_, "Embed PNG/BMP images");
  export_partial_ = check(search_box_, "Export returned matches from incomplete search");
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
  dialog_ = GTK_NATIVE_DIALOG(gtk_file_chooser_native_new(kind == 0 ? "Import saved search lists" : kind == 1 ? "Restore book selection"
												: kind >= 9   ? "Choose data file"
												: kind >= 7   ? "Parallel reading profile"
												: kind >= 3   ? "Edit saved search choices"
													      : "Save search results",
							  parent_,
							  (kind >= 2 && kind <= 4) || kind == 8 || kind == 10 || kind == 12 || kind == 14 || kind == 16 || kind == 17 ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN, (kind >= 2 && kind <= 4) || kind == 8 || kind == 10 || kind == 12 || kind == 14 || kind == 16 || kind == 17 ? "Save" : "Select", "Cancel"));
  g_object_set_data(G_OBJECT(dialog_), "kind", GINT_TO_POINTER(kind));
  if (kind >= 3 && kind <= 6) {
	  g_object_set_data_full(G_OBJECT(dialog_), "choice-label", g_strdup(text(list_label_).c_str()), g_free);
	  g_object_set_data_full(G_OBJECT(dialog_), "choice-value", g_strdup(text(kind == 4 || kind == 6 ? range_ : search_modules_).c_str()), g_free);
  }
  if (kind == 2) gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog_), pending_export_html_ ? "search.html" : "search.txt");
  g_signal_connect(dialog_, "response", G_CALLBACK((+[](GtkNativeDialog *dialog, int response, gpointer data) {
    auto *s = static_cast<StudyUI *>(data); int kind = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "kind"));
    if (!s->closed_ && response == GTK_RESPONSE_ACCEPT) {
      auto *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog)); char *path = file ? g_file_get_path(file) : nullptr;
      if (kind == 0 && path) s->model_->import_search_lists(path);
      else if (kind == 1 && path) s->model_->restore_book(path);
      else if (kind == 7 && path)
	      s->model_->load_parallel_profile(path);
      else if (kind == 8 && path)
	      s->model_->new_parallel_profile(path);
      else if ((kind == 9 || kind == 10) && path)
	      s->model_->open_bookmarks(path, kind == 10);
      else if ((kind == 11 || kind == 12) && path)
	      s->model_->open_annotations(path, kind == 12);
      else if (kind == 15 && path)
	      s->model_->preview_andbible(path);
      else if (kind == 16 && path)
	      s->model_->export_bookmarks(path);
      else if (kind == 17 && path)
	      s->model_->save_editor_as(path);
      else if ((kind == 13 || kind == 14) && path)
	      s->model_->open_editor(path, kind == 14);
      else if (kind >= 3 && kind <= 6 && path)
	      s->model_->edit_search_list(path, kind == 4 || kind == 6,
					  static_cast<const char *>(g_object_get_data(G_OBJECT(dialog), "choice-label")),
					  static_cast<const char *>(g_object_get_data(G_OBJECT(dialog), "choice-value")), kind >= 5);
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
bool StudyUI::valid_reading_color(const std::string &color)
{
  return color.empty() || (color.size() == 7 && color.front() == '#' &&
    std::all_of(color.begin() + 1, color.end(), [](char c) { return g_ascii_isxdigit(c); }));
}
std::string StudyUI::reading_style_css(const std::string &module, const std::string &selector) const
{
  auto style = reading_styles_.find(module);
  if (style == reading_styles_.end())
    style = reading_styles_.find("");
  if (style == reading_styles_.end())
    return {};
  const auto &value = style->second;
  const char *fonts[] = {"serif", "sans-serif", "monospace"};
  std::string css = selector + "{font-family:" + fonts[value.font] + ";font-size:" + std::to_string(value.size) + "px;";
  if (!value.foreground.empty())
    css += "color:" + value.foreground + ";";
  if (!value.background.empty())
    css += "background-color:" + value.background + ";";
  return css + "}";
}
void StudyUI::load_reading_style()
{
  const auto index = gtk_drop_down_get_selected(GTK_DROP_DOWN(style_module_));
  if (index >= style_modules_.size())
    return;
  auto saved = reading_styles_.find(style_modules_[index]);
  if (saved == reading_styles_.end())
    saved = reading_styles_.find("");
  const auto style = saved == reading_styles_.end() ? ReadingStyle{} : saved->second;
  gtk_drop_down_set_selected(GTK_DROP_DOWN(style_font_), style.font);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(style_size_), style.size);
  gtk_editable_set_text(GTK_EDITABLE(style_foreground_), style.foreground.c_str());
  gtk_editable_set_text(GTK_EDITABLE(style_background_), style.background.c_str());
  gtk_label_set_text(GTK_LABEL(style_status_), "");
}
GtkWidget *StudyUI::reading_style_menu()
{
  auto *menu = gtk_menu_button_new();
  gtk_menu_button_set_label(GTK_MENU_BUTTON(menu), "Reading style");
  auto *popover = gtk_popover_new();
  auto *content = box(nullptr);
  gtk_popover_set_child(GTK_POPOVER(popover), content);
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu), popover);
  style_module_ = choice(content, "Apply reading style to");
  const char *fonts[] = {"Serif", "Sans serif", "Monospace", nullptr};
  style_font_ = choice(content, "Reading font", fonts);
  style_size_ = gtk_spin_button_new_with_range(12, 36, 1);
  gtk_accessible_update_property(GTK_ACCESSIBLE(style_size_), GTK_ACCESSIBLE_PROPERTY_LABEL, "Reading text size", -1);
  gtk_box_append(GTK_BOX(content), style_size_);
  style_foreground_ = entry(content, "Text color (#RRGGBB, optional)");
  style_background_ = entry(content, "Background color (#RRGGBB, optional)");
  style_status_ = label(content, "");
  auto *actions = box(content, true);
  for (const auto *name : {"Apply", "Reset"}) {
    auto *action = button(actions, name, G_CALLBACK((+[](GtkButton *button, gpointer data) {
      auto *ui = static_cast<StudyUI *>(data);
      const auto index = gtk_drop_down_get_selected(GTK_DROP_DOWN(ui->style_module_));
      if (index >= ui->style_modules_.size())
        return;
      const auto &module = ui->style_modules_[index];
      if (g_object_get_data(G_OBJECT(button), "reset")) {
        ui->reading_styles_.erase(module);
        ui->load_reading_style();
      } else {
        const auto foreground = text(ui->style_foreground_), background = text(ui->style_background_);
        if (!valid_reading_color(foreground) || !valid_reading_color(background)) {
          gtk_label_set_text(GTK_LABEL(ui->style_status_), "Use #RRGGBB or leave the color blank.");
          return;
        }
        ui->reading_styles_[module] = {gtk_drop_down_get_selected(GTK_DROP_DOWN(ui->style_font_)),
          unsigned(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ui->style_size_))), foreground, background};
        gtk_label_set_text(GTK_LABEL(ui->style_status_), "Reading style applied.");
      }
      ++ui->style_revision_;
      ui->parallel_needs_render_ = true;
    })), this);
    if (std::string(name) == "Reset")
      g_object_set_data(G_OBJECT(action), "reset", GINT_TO_POINTER(1));
  }
  g_signal_connect(style_module_, "notify::selected", G_CALLBACK(+[](GObject *, GParamSpec *, gpointer data) {
    static_cast<StudyUI *>(data)->load_reading_style();
  }), this);
  return menu;
}
GtkWidget *StudyUI::devotional_pane()
{
  auto *pane = box(nullptr);
  devotional_module_ = choice(pane, "Devotional module");
  devotional_date_ = entry(pane, "Devotional date (MM.DD)");
  auto *actions = box(pane, true);
  button(actions, "Today", G_CALLBACK(+[](GtkButton *, gpointer data) {
    static_cast<StudyUI *>(data)->read_devotional(true);
  }), this);
  button(actions, "Read date", G_CALLBACK(+[](GtkButton *, gpointer data) {
    static_cast<StudyUI *>(data)->read_devotional(false);
  }), this);
  devotional_status_ = gtk_label_new("");
  gtk_box_append(GTK_BOX(pane), devotional_status_);
  gtk_box_append(GTK_BOX(pane), view(&devotional_view_, false));
  return pane;
}
void StudyUI::set_default_devotional(std::string module)
{
  default_devotional_ = std::move(module);
  if (!devotional_module_)
    return;
  const auto preferred = std::find(devotional_modules_.begin(), devotional_modules_.end(), default_devotional_);
  if (preferred != devotional_modules_.end())
    gtk_drop_down_set_selected(GTK_DROP_DOWN(devotional_module_), preferred - devotional_modules_.begin());
}
void StudyUI::read_devotional(bool today)
{
  if (!model_)
    return;
  const auto index = gtk_drop_down_get_selected(GTK_DROP_DOWN(devotional_module_));
  if (index >= devotional_modules_.size()) {
    gtk_label_set_text(GTK_LABEL(devotional_status_), "No devotional modules are installed.");
    return;
  }
  auto *now = g_date_time_new_now_local();
  if (!now)
    return;
  model_->read_devotional(devotional_modules_[index], today ? "" : text(devotional_date_),
    g_date_time_get_month(now), g_date_time_get_day_of_month(now));
  g_date_time_unref(now);
}
void StudyUI::update_selected_word()
{
  const auto &word = model_->selected_word();
  if (!word_box_ || !word.revision || word.revision == word_revision_)
    return;
  word_revision_ = word.revision;
  gtk_label_set_text(GTK_LABEL(word_label_), (word.surface + " — " + word.module + " · " + word.reference).c_str());
  std::vector<std::string> values{"Literal surface form", "Surface form without marks"};
  for (const auto &lemma : word.lemmas)
    values.push_back("Lemma: " + lemma);
  choices(word_choices_, values);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(word_choices_), 0);
  gtk_widget_set_visible(word_box_, TRUE);
  if (show_search)
    show_search();
}
void StudyUI::search_selected_word()
{
  if (!model_ || model_->busy())
    return;
  const auto &word = model_->selected_word();
  const auto selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(word_choices_));
  SearchInput input;
  input.modules = {word.module};
  if (selected < 2) {
    input.expression = word.surface;
    input.mode = selected == 0 ? XC_SEARCH_SURFACE_LITERAL : XC_SEARCH_SURFACE_UNMARKED;
  } else if (selected - 2 < word.lemmas.size()) {
    input.expression = word.lemmas[selected - 2];
    input.mode = XC_SEARCH_LEMMA;
  } else {
    return;
  }
  if (model_->search(input)) {
    gtk_editable_set_text(GTK_EDITABLE(search_text_), input.expression.c_str());
    gtk_editable_set_text(GTK_EDITABLE(search_modules_), word.module.c_str());
    gtk_drop_down_set_selected(GTK_DROP_DOWN(search_mode_), input.mode);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(search_scope_), 0);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(mixed_), FALSE);
  }
}
void StudyUI::tick(ReaderModel &model) {
  if (closed_) return; model_ = &model; updating_ = true;
  bool enabled = model.ready() && !model.busy();
  if (model.ready() && (!populated_ || catalog_version_ != model.catalog_version())) {
	  catalog_version_ = model.catalog_version();
	  book_identity_.clear();
	  books_.clear();
	  search_ids_.clear();
	  for (const auto &m : model.modules()) {
		  if (m.book)
			  books_.push_back(m.id);
		  if (m.search)
			  search_ids_.push_back(m.id);
	  }
	  if (devotional_module_) {
		  devotional_modules_.clear();
		  for (const auto &module : model.modules())
			  if (module.devotional)
				  devotional_modules_.push_back(module.id);
		  choices(devotional_module_, devotional_modules_);
		  set_default_devotional(default_devotional_);
	  }
	  if (style_module_) {
		  style_modules_ = {""};
		  std::vector<std::string> names{"Default for all modules"};
		  for (const auto &module : model.modules()) {
			  style_modules_.push_back(module.id);
			  names.push_back(module.id);
		  }
		  choices(style_module_, names);
		  load_reading_style();
	  }
	  auto book_choices = books_;
	  book_choices.insert(book_choices.begin(), "Select a general book");
	  choices(book_module_, book_choices);
	  gtk_drop_down_set_selected(GTK_DROP_DOWN(book_module_), 0);
	  choices(range_module_, search_ids_);
	  if (!populated_ && !search_ids_.empty())
		  gtk_editable_set_text(GTK_EDITABLE(search_modules_), search_ids_.front().c_str());
	  populated_ = true;
  }
  const auto &book = model.book();
  const auto style_css = reading_style_css(book.module);
  const auto identity = style_css + book.module + book.nodes[0].path + book.html + book.css + (book.anchor ? *book.anchor : "") + std::to_string(book.anchor_revision);
  if (identity != book_identity_) {
    if (auto *state = static_cast<BookAnchorState *>(g_object_get_data(G_OBJECT(book_view_), "book-anchor-state"))) {
      state->anchor = book.anchor; ++state->revision;
    }
    book_identity_ = identity; gtk_editable_set_text(GTK_EDITABLE(book_key_), book.nodes[0].path.c_str()); gtk_label_set_text(GTK_LABEL(book_label_), book.nodes[0].path.c_str());
    webkit_web_view_load_html(book_view_, document(book.html, book.css + style_css, true).c_str(), "xiphos-preview://study/");
    gtk_label_set_text(GTK_LABEL(book_status_), book.issue.c_str());
    for (size_t i = 0; i < books_.size(); ++i)
	    if (books_[i] == book.module)
		    gtk_drop_down_set_selected(GTK_DROP_DOWN(book_module_), i + 1);
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
  if (preview_identity_ != preview_id) {
	  preview_identity_ = preview_id;
	  webkit_web_view_load_html(preview_view_, document(preview.html, "", model.search_preview_kind() != 0).c_str(), "xiphos-preview://study/");
  }
  std::string lists_id; for (int k = 0; k < 2; ++k) for (const auto &v : model.saved_lists(k)) lists_id += (v.label ? "label:" + *v.label : "absent") + (v.value ? "value:" + *v.value : "absent");
  if (lists_id != lists_identity_) { lists_identity_ = lists_id; for (int k = 0; k < 2; ++k) { std::vector<std::string> names; for (const auto &v : model.saved_lists(k)) names.push_back(v.label ? (v.label->empty() ? "(Empty label)" : *v.label) : "(Unlabelled)"); choices(k ? saved_ranges_ : saved_modules_, names); } }
  if (model.parsed_search_revision() != parsed_revision_) {
    parsed_revision_ = model.parsed_search_revision();
    if (parallel_modules_ && GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parallel_modules_), "pending-action"))) {
	    const int action = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parallel_modules_), "pending-action"));
	    g_object_set_data(G_OBJECT(parallel_modules_), "pending-action", nullptr);
	    if (action == 1)
		    model.visit_parallel(model.parsed_search_modules(), text(parallel_reference_));
	    else if (action == 2)
		    model.edit_parallel_set(XC_PARALLEL_PROFILE_CREATE, text(parallel_name_), "", model.parsed_search_modules(), active(parallel_default_));
	    else {
		    const auto i = gtk_drop_down_get_selected(GTK_DROP_DOWN(parallel_sets_));
		    const auto &sets = model.parallel_profile().sets;
		    if (i < sets.size())
			    model.edit_parallel_set(XC_PARALLEL_PROFILE_SAVE, sets[i].key, "", model.parsed_search_modules(), active(parallel_default_));
	    }
    } else if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(search_text_), "start-after-parse"))) {
	    g_object_set_data(G_OBJECT(search_text_), "start-after-parse", nullptr);
	    SearchInput input;
	    input.modules = model.parsed_search_modules();
	    input.expression = text(search_text_);
	    input.mode = gtk_drop_down_get_selected(GTK_DROP_DOWN(search_mode_));
	    const auto scope = gtk_drop_down_get_selected(GTK_DROP_DOWN(search_scope_));
	    input.scope = scope == 3 ? XC_SEARCH_SCOPE_ALL : static_cast<xc_search_scope_kind>(scope);
	    input.range = text(range_);
	    input.mixed = active(mixed_);
	    if (scope == 3)
		    input.interval = std::make_pair(text(first_key_), text(last_key_));
	    auto i = gtk_drop_down_get_selected(GTK_DROP_DOWN(range_module_));
	    if (i < search_ids_.size())
		    input.range_module = search_ids_[i];
	    input.case_sensitive = active(case_);
	    if (active(annotations_) && input.mode != XC_SEARCH_PHRASE)
		    input.annotations = xc_search_text_options{active(annotation_[0]), active(annotation_[1]), active(annotation_[2])};
	    if (!model.search(input))
		    gtk_label_set_text(GTK_LABEL(search_status_), model.error().empty() ? "Choose at least one module and enter a search expression." : model.error().c_str());
    } else {
	    std::string names;
	    for (const auto &m : model.parsed_search_modules()) {
		    if (!names.empty())
			    names += ",";
		    names += m;
	    }
	    gtk_editable_set_text(GTK_EDITABLE(search_modules_), names.c_str());
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
  for (auto *w : {mixed_, first_key_, last_key_, list_label_, search_modules_, search_text_, search_mode_, search_scope_, range_module_, range_, case_, annotations_, search_start_, search_import_, search_reset_, saved_modules_, saved_ranges_, apply_modules_, apply_range_})
	  gtk_widget_set_sensitive(w, enabled);
  for (auto *w : annotation_) gtk_widget_set_sensitive(w, enabled && active(annotations_) && gtk_drop_down_get_selected(GTK_DROP_DOWN(search_mode_)) != XC_SEARCH_PHRASE);
  gtk_widget_set_sensitive(search_start_, enabled && !text(search_modules_).empty() && !text(search_text_).empty());
  gtk_widget_set_sensitive(search_stop_, model.busy());
  gtk_widget_set_sensitive(search_clear_, enabled && (result.scope.present || result.generation.id));
  gtk_widget_set_sensitive(open_, enabled && !selection().empty());
  const auto selected_rows = selection();
  gtk_widget_set_sensitive(compare_result_, enabled && !model.parallel_profile().path.empty() &&
    selected_rows.size() == 1 && selected_rows.front() < result.hits.size() && result.hits[selected_rows.front()].kind == 0);
  if (requested_comparison_ && enabled) {
    if (model.parallel().revision == requested_comparison_ && show_parallel)
      show_parallel();
    requested_comparison_ = 0;
  }
  for (auto *w : {copy_, save_}) gtk_widget_set_sensitive(w, enabled && !dialog_ && result.generation.id && (active(export_all_) || !selection().empty()));
  gtk_widget_set_sensitive(export_images_, enabled && active(export_html_) && active(export_scripture_));
  gtk_widget_set_sensitive(export_partial_, enabled && active(export_context_) && (result.generation.cancelled || result.generation.failed_groups || result.generation.group_count < result.generation.requested_count));
  if (enabled && !model.error().empty()) g_object_set_data(G_OBJECT(search_text_), "start-after-parse", nullptr);
  if (devotional_view_ && model.devotional_revision() != devotional_revision_) {
    devotional_revision_ = model.devotional_revision();
    const auto &entry = model.devotional();
    gtk_editable_set_text(GTK_EDITABLE(devotional_date_), entry.reference.c_str());
    gtk_label_set_text(GTK_LABEL(devotional_status_), (entry.module + " · " + entry.expression).c_str());
    webkit_web_view_load_html(devotional_view_, document(entry.html).c_str(), "xiphos-preview://study/");
  }
  update_selected_word();
  if (parallel_box_)
	  update_parallel();
  if (notes_box_ || editor_box_)
	  update_repositories();
  if (!model.busy() && !restore_sources_.empty()) {
	  const auto source = restore_sources_.front();
	  restore_sources_.erase(restore_sources_.begin());
	  if (source.first == 0)
		  model.open_bookmarks(source.second);
	  else if (source.first == 1)
		  model.open_annotations(source.second);
	  else
		  model.load_parallel_profile(source.second);
  }
  updating_ = false;
}
}
