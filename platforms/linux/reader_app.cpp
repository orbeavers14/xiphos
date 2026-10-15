// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <clocale>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
using xiphos::linux_preview::ReaderModel;
namespace {
struct App {
  GtkApplication *application = nullptr;
  GtkWidget *window = nullptr, *reference = nullptr, *bible = nullptr, *commentary = nullptr;
  GtkWidget *back = nullptr, *forward = nullptr, *go = nullptr, *status = nullptr;
  GtkWidget *chapter_mode = nullptr, *commentary_mode = nullptr, *numbers = nullptr, *highlight = nullptr;
  GtkWidget *selectors[3] = {}, *previous_chapter = nullptr, *next_chapter = nullptr, *clear_history = nullptr;
  bool updating = false;
  std::string selector_identity;
  GtkWidget *dictionary_module = nullptr, *dictionary_key = nullptr, *dictionary_lookup = nullptr;
  GtkWidget *dictionary_previous = nullptr, *dictionary_next = nullptr, *dictionary_back = nullptr, *dictionary_forward = nullptr;
  GtkWidget *dictionary_nearby = nullptr, *dictionary_clear = nullptr, *dictionary_close = nullptr;
  WebKitWebView *dictionary_view = nullptr;
  std::vector<std::string> dictionaries;
  std::string dictionary_identity, dictionary_choices_identity, dictionary_submitted;
  WebKitWebView *bible_view = nullptr, *commentary_view = nullptr;
  std::unique_ptr<ReaderModel> model;
  std::vector<std::string> bibles, commentaries{""};
  std::string root, rendered, submitted_draft;
  guint timer = 0;
  bool populated = false, closing = false, smoke = false, smoke_failed = false, software = false;
  unsigned smoke_stage = 0, loaded = 0;
};
std::string selected(GtkWidget *dropdown, const std::vector<std::string> &ids) {
  const auto index = gtk_drop_down_get_selected(GTK_DROP_DOWN(dropdown));
  return index < ids.size() ? ids[index] : "";
}
void select(GtkWidget *dropdown, const std::vector<std::string> &ids, const std::string &id) {
  for (size_t i = 0; i < ids.size(); ++i)
    if (ids[i] == id) { gtk_drop_down_set_selected(GTK_DROP_DOWN(dropdown), i); return; }
}
void shutdown(App *app);
void go(App *app) {
  app->submitted_draft = gtk_editable_get_text(GTK_EDITABLE(app->reference));
  app->model->visit(selected(app->bible, app->bibles), selected(app->commentary, app->commentaries), gtk_editable_get_text(GTK_EDITABLE(app->reference)));
}
std::string document(const std::string &body) {
  // No module scripts, external resources, frames or network requests. Native
  // policy routes user passage clicks through C++; no browser navigation engine.
  return "<!doctype html><html><head><meta charset='utf-8'><meta http-equiv='Content-Security-Policy' content=\"default-src 'none'; style-src 'unsafe-inline'; img-src 'none'; script-src 'none'; frame-src 'none'; connect-src 'none'\"><style>body{font:18px serif;line-height:1.55;margin:20px;color:#202020;background:#fff}a{color:#174ea6}.xc-verse{padding:.15em 0}.xc-current{background:#fff0b3}.xc-commentary td{padding:4px;vertical-align:top}@media(prefers-color-scheme:dark){body{background:#202020;color:#eee}a{color:#9ecbff}.xc-current{background:#4d421e}}</style></head><body>" + body + "</body></html>";
}
gboolean policy(WebKitWebView *view, WebKitPolicyDecision *decision, WebKitPolicyDecisionType type, gpointer data) {
  auto *app = static_cast<App *>(data);
  if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION || type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION) {
    auto *action = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));
    if (webkit_navigation_action_get_navigation_type(action) == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED) {
      webkit_policy_decision_ignore(decision);
      if (!app->closing) {
        auto *request = webkit_navigation_action_get_request(action);
        const char *uri = webkit_uri_request_get_uri(request);
        const auto &passage = app->model->passage();
        app->submitted_draft = gtk_editable_get_text(GTK_EDITABLE(app->reference));
        app->model->follow(view == app->commentary_view ? passage.commentary : passage.bible, uri ? uri : "");
      }
      return TRUE;
    }
    // load_html uses this fixed local origin. All other destinations reject.
    const char *uri = webkit_uri_request_get_uri(webkit_navigation_action_get_request(action));
    if (!uri || (std::string(uri) != "xiphos-preview://study/" && std::string(uri) != "about:blank")) {
      webkit_policy_decision_ignore(decision); return TRUE;
    }
  }
  return FALSE;
}
GtkWidget *pane(App *app, const char *title, WebKitWebView **view) {
  auto *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  auto *heading = gtk_label_new(title); gtk_box_append(GTK_BOX(box), heading);
  *view = WEBKIT_WEB_VIEW(webkit_web_view_new());
  webkit_settings_set_enable_javascript(webkit_web_view_get_settings(*view), FALSE);
  if (app->software) webkit_settings_set_hardware_acceleration_policy(webkit_web_view_get_settings(*view), WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);
  g_signal_connect(*view, "decide-policy", G_CALLBACK(policy), app);
  g_signal_connect(*view, "load-changed", G_CALLBACK(+[](WebKitWebView *, WebKitLoadEvent event, gpointer data) {
    if (event == WEBKIT_LOAD_FINISHED) ++static_cast<App *>(data)->loaded;
  }), app);
  gtk_widget_set_vexpand(GTK_WIDGET(*view), TRUE);
  gtk_box_append(GTK_BOX(box), GTK_WIDGET(*view));
  webkit_web_view_load_html(*view, document("<p>Select a module and enter a reference.</p>").c_str(), "xiphos-preview://study/");
  return box;
}
gboolean tick(gpointer data) {
  auto *app = static_cast<App *>(data);
  if (app->closing) return G_SOURCE_REMOVE;
  app->model->tick();
  if (app->model->ready() && !app->populated) {
    auto *bibles = gtk_string_list_new(nullptr), *commentaries = gtk_string_list_new(nullptr), *dictionaries = gtk_string_list_new(nullptr);
    gtk_string_list_append(commentaries, "No commentary");
    for (const auto &module : app->model->modules()) {
      const auto label = module.id + " — " + module.description;
      if (module.type == "Biblical Texts") { app->bibles.push_back(module.id); gtk_string_list_append(bibles, label.c_str()); }
      if (module.dictionary) { app->dictionaries.push_back(module.id); gtk_string_list_append(dictionaries, label.c_str()); }
      if (module.type == "Commentaries") { app->commentaries.push_back(module.id); gtk_string_list_append(commentaries, label.c_str()); }
    }
    gtk_drop_down_set_model(GTK_DROP_DOWN(app->bible), G_LIST_MODEL(bibles));
    gtk_drop_down_set_model(GTK_DROP_DOWN(app->commentary), G_LIST_MODEL(commentaries));
    gtk_drop_down_set_model(GTK_DROP_DOWN(app->dictionary_module), G_LIST_MODEL(dictionaries)); g_object_unref(dictionaries);
    g_object_unref(bibles); g_object_unref(commentaries); app->populated = true;
  }
  const auto &passage = app->model->passage();
  const auto bible_html = app->model->show_chapter() ? app->model->bible_chapter().html : passage.bible_html;
  const auto commentary_html = app->model->show_commentary_chapter() ? app->model->commentary_chapter().html : passage.commentary_html;
  const auto identity = passage.bible + "\n" + passage.commentary + "\n" + passage.reference + "\n" + bible_html + commentary_html;
  if (!app->model->busy() && !passage.bible.empty() && identity != app->rendered) {
    app->rendered = identity;
    select(app->bible, app->bibles, passage.bible); select(app->commentary, app->commentaries, passage.commentary);
    if (app->submitted_draft == gtk_editable_get_text(GTK_EDITABLE(app->reference)))
      gtk_editable_set_text(GTK_EDITABLE(app->reference), passage.reference.c_str());
    webkit_web_view_load_html(app->bible_view, document(bible_html).c_str(), "xiphos-preview://study/");
    webkit_web_view_load_html(app->commentary_view, document(commentary_html).c_str(), "xiphos-preview://study/");
  }
  const bool enabled = app->model->ready() && !app->model->busy();
  gtk_widget_set_sensitive(app->go, enabled && !app->bibles.empty());
  gtk_widget_set_sensitive(app->back, enabled && app->model->can_back());
  gtk_widget_set_sensitive(app->forward, enabled && app->model->can_forward());
  for (const auto *name : {"go", "back", "forward", "focus-reference"}) {
    auto *action = g_action_map_lookup_action(G_ACTION_MAP(app->window), name);
    const bool active = std::string(name) == "go" ? enabled && !app->bibles.empty()
      : std::string(name) == "back" ? enabled && app->model->can_back()
      : std::string(name) == "forward" ? enabled && app->model->can_forward() : app->model->ready();
    g_simple_action_set_enabled(G_SIMPLE_ACTION(action), active);
  }
  gtk_widget_set_sensitive(app->bible, enabled); gtk_widget_set_sensitive(app->commentary, enabled);
  app->updating = true;
  gtk_check_button_set_active(GTK_CHECK_BUTTON(app->chapter_mode), app->model->show_chapter());
  gtk_check_button_set_active(GTK_CHECK_BUTTON(app->commentary_mode), app->model->show_commentary_chapter());
  gtk_check_button_set_active(GTK_CHECK_BUTTON(app->numbers), app->model->display_flags() & XC_CHAPTER_VERSE_NUMBERS);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(app->highlight), app->model->display_flags() & XC_CHAPTER_HIGHLIGHT_SELECTED);
  gtk_widget_set_sensitive(app->chapter_mode, enabled);
  gtk_widget_set_sensitive(app->numbers, enabled); gtk_widget_set_sensitive(app->highlight, enabled);
  gtk_widget_set_sensitive(app->commentary_mode, enabled && !passage.commentary.empty());
  gtk_widget_set_sensitive(app->previous_chapter, enabled && !passage.bible.empty());
  gtk_widget_set_sensitive(app->next_chapter, enabled && !passage.bible.empty());
  gtk_widget_set_sensitive(app->clear_history, enabled && !passage.bible.empty());
  std::string selector_identity;
  for (int k = 0; k < 3; ++k) {
    selector_identity += app->model->choice_label(k) + "\n";
    for (const auto &choice : app->model->choices(k)) selector_identity += choice.label + choice.target + "\n";
  }
  if (selector_identity != app->selector_identity) {
    app->selector_identity = selector_identity;
    for (int k = 0; k < 3; ++k) {
      auto *list = gtk_string_list_new(nullptr); guint selected_index = 0;
      const auto &choices = app->model->choices(k);
      for (size_t i = 0; i < choices.size(); ++i) {
        gtk_string_list_append(list, choices[i].label.c_str());
        if (choices[i].label == app->model->choice_label(k)) selected_index = i;
      }
      gtk_drop_down_set_model(GTK_DROP_DOWN(app->selectors[k]), G_LIST_MODEL(list)); g_object_unref(list);
      gtk_drop_down_set_selected(GTK_DROP_DOWN(app->selectors[k]), selected_index);
    }
  }
  for (auto *selector : app->selectors) gtk_widget_set_sensitive(selector, enabled && !passage.bible.empty());
  const auto &dictionary = app->model->dictionary();
  const auto dictionary_identity = dictionary.module + "\n" + dictionary.key + "\n" + dictionary.html;
  if (!app->model->busy() && dictionary_identity != app->dictionary_identity) {
    app->dictionary_identity = dictionary_identity;
    select(app->dictionary_module, app->dictionaries, dictionary.module);
    if (app->dictionary_submitted == gtk_editable_get_text(GTK_EDITABLE(app->dictionary_key)))
      gtk_editable_set_text(GTK_EDITABLE(app->dictionary_key), dictionary.key.c_str());
    webkit_web_view_load_html(app->dictionary_view, document(dictionary.html).c_str(), "xiphos-preview://study/");
  }
  std::string dictionary_choices_identity;
  for (const auto &key : app->model->dictionary_keys()) dictionary_choices_identity += key + "\n";
  if (dictionary_choices_identity != app->dictionary_choices_identity) {
    app->dictionary_choices_identity = dictionary_choices_identity;
    auto *list = gtk_string_list_new(nullptr);
    for (const auto &key : app->model->dictionary_keys()) gtk_string_list_append(list, key.c_str());
    gtk_drop_down_set_model(GTK_DROP_DOWN(app->dictionary_nearby), G_LIST_MODEL(list)); g_object_unref(list);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(app->dictionary_nearby), GTK_INVALID_LIST_POSITION);
  }
  const bool dictionary_matches = !dictionary.module.empty() && selected(app->dictionary_module, app->dictionaries) == dictionary.module;
  gtk_widget_set_sensitive(app->dictionary_module, enabled);
  gtk_widget_set_sensitive(app->dictionary_lookup, enabled && !app->dictionaries.empty());
  gtk_widget_set_sensitive(app->dictionary_previous, enabled && dictionary_matches);
  gtk_widget_set_sensitive(app->dictionary_next, enabled && dictionary_matches);
  gtk_widget_set_sensitive(app->dictionary_nearby, enabled && dictionary_matches);
  gtk_widget_set_sensitive(app->dictionary_back, enabled && app->model->dictionary_can_back());
  gtk_widget_set_sensitive(app->dictionary_forward, enabled && app->model->dictionary_can_forward());
  gtk_widget_set_sensitive(app->dictionary_clear, enabled && app->model->dictionary_history_count() > 0);
  gtk_widget_set_sensitive(app->dictionary_close, enabled && !dictionary.module.empty());
  app->updating = false;
  const auto &error = app->model->error();
  gtk_label_set_text(GTK_LABEL(app->status), !error.empty() ? error.c_str() : app->model->busy() ? "Loading…" : app->populated && app->bibles.empty() ? "No Bible modules found. Open a SWORD library with --root DIRECTORY." : "Ready");
  if (app->smoke && enabled) {
    if (!error.empty()) {
      app->smoke_failed = true; std::cerr << error << "\n"; gtk_window_close(GTK_WINDOW(app->window)); return G_SOURCE_REMOVE;
    }
    const auto prior_stage = app->smoke_stage;
    switch (app->smoke_stage) {
    case 0:
      for (const auto *name : {"win.back", "win.forward", "win.focus-reference"}) {
        char **keys = gtk_application_get_accels_for_action(app->application, name);
        if (!keys || !keys[0]) app->smoke_failed = true;
        g_strfreev(keys);
      }
      if (g_action_group_get_action_enabled(G_ACTION_GROUP(app->window), "back") ||
          g_action_group_get_action_enabled(G_ACTION_GROUP(app->window), "forward")) app->smoke_failed = true;
      gtk_widget_activate_action(app->window, "win.focus-reference", nullptr);
      {
        auto *focus = gtk_root_get_focus(GTK_ROOT(app->window));
        if (!focus || (focus != app->reference && !gtk_widget_is_ancestor(focus, app->reference))) app->smoke_failed = true;
      }
      select(app->bible, app->bibles, "Text"); select(app->commentary, app->commentaries, "Notes");
      g_signal_emit_by_name(app->reference, "activate");
      gtk_editable_set_text(GTK_EDITABLE(app->reference), "John 4:2"); ++app->smoke_stage; break;
    case 1:
      if (std::string(gtk_editable_get_text(GTK_EDITABLE(app->reference))) != "John 4:2") app->smoke_failed = true;
      if (passage.reference != "John 3:16" || passage.commentary != "Notes") app->smoke_failed = true;
      gtk_editable_set_text(GTK_EDITABLE(app->reference), "John 3:17");
      gtk_widget_activate_action(app->window, "win.go", nullptr); ++app->smoke_stage; break;
    case 2:
      if (!app->model->can_back()) app->smoke_failed = true;
      gtk_widget_activate_action(app->window, "win.back", nullptr); ++app->smoke_stage; break;
    case 3:
      if (passage.reference != "John 3:16" || !app->model->can_forward()) app->smoke_failed = true;
      gtk_widget_activate_action(app->window, "win.forward", nullptr); ++app->smoke_stage; break;
    case 4:
      if (app->loaded < 2) break;
      if (passage.reference != "John 3:17") app->smoke_failed = true;
      gtk_check_button_set_active(GTK_CHECK_BUTTON(app->chapter_mode), TRUE); ++app->smoke_stage; break;
    case 5:
      if (app->model->bible_chapter().html.empty() || app->model->choices(XC_VERSE).empty()) app->smoke_failed = true;
      g_signal_emit_by_name(app->next_chapter, "clicked"); ++app->smoke_stage; break;
    case 6:
      if (passage.reference != "John 4:1") app->smoke_failed = true;
      gtk_widget_activate_action(app->window, "win.back", nullptr); ++app->smoke_stage; break;
    case 7:
      if (passage.reference != "John 3:17") app->smoke_failed = true;
      gtk_check_button_set_active(GTK_CHECK_BUTTON(app->commentary_mode), FALSE); ++app->smoke_stage; break;
    case 8:
      if (app->model->show_commentary_chapter()) app->smoke_failed = true;
      g_signal_emit_by_name(app->clear_history, "clicked"); ++app->smoke_stage; break;
    case 9:
      if (app->model->can_back() || app->model->can_forward()) app->smoke_failed = true;
      select(app->dictionary_module, app->dictionaries, "Dictionary");
      gtk_editable_set_text(GTK_EDITABLE(app->dictionary_key), "ALPHA");
      g_signal_emit_by_name(app->dictionary_key, "activate"); ++app->smoke_stage; break;
    case 10:
      if (app->model->dictionary().html.find("dictionary pane fixture") == std::string::npos || app->model->dictionary_keys().size() != 9) app->smoke_failed = true;
      g_signal_emit_by_name(app->dictionary_next, "clicked"); ++app->smoke_stage; break;
    case 11:
      if (app->model->dictionary_history_count() != 0) app->smoke_failed = true;
      g_signal_emit_by_name(app->dictionary_close, "clicked"); ++app->smoke_stage; break;
    case 12:
      if (!app->model->dictionary().module.empty()) app->smoke_failed = true;
      g_signal_emit_by_name(app->go, "clicked"); // accepted work must drain during close
      ++app->smoke_stage; gtk_window_close(GTK_WINDOW(app->window)); return G_SOURCE_REMOVE;
    }
    if (app->smoke_failed) std::cerr << "GTK4 smoke assertion at stage " << prior_stage << " reference=" << passage.reference << " commentary=" << passage.commentary << "\n";
  }
  return G_SOURCE_CONTINUE;
}
void shutdown(App *app) {
  if (app->closing) return;
  app->closing = true;
  if (app->timer) { g_source_remove(app->timer); app->timer = 0; }
  auto *worker = app->model ? app->model->detach() : nullptr;
  if (!worker) return;
  g_application_hold(G_APPLICATION(app->application));
  auto *task = g_task_new(app->application, nullptr, [](GObject *source, GAsyncResult *result, gpointer) {
    g_task_propagate_boolean(G_TASK(result), nullptr); g_application_release(G_APPLICATION(source));
  }, nullptr);
  g_task_set_task_data(task, worker, nullptr);
  g_task_run_in_thread(task, [](GTask *task, gpointer, gpointer worker, GCancellable *) {
    xc_worker_close(static_cast<xc_worker *>(worker)); g_task_return_boolean(task, TRUE);
  });
  g_object_unref(task);
}
void activate(GtkApplication *application, gpointer data) {
  auto *app = static_cast<App *>(data);
  if (app->window) { gtk_window_present(GTK_WINDOW(app->window)); return; }
  app->application = application;
  app->window = gtk_application_window_new(application);
  gtk_window_set_title(GTK_WINDOW(app->window), "Xiphos Linux Preview");
  gtk_window_set_default_size(GTK_WINDOW(app->window), 1150, 720);
  auto *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_window_set_child(GTK_WINDOW(app->window), box);
  auto *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_append(GTK_BOX(box), toolbar);
  app->back = gtk_button_new_with_label("Back"); app->forward = gtk_button_new_with_label("Forward");
  gtk_box_append(GTK_BOX(toolbar), app->back); gtk_box_append(GTK_BOX(toolbar), app->forward);
  app->reference = gtk_entry_new(); gtk_editable_set_text(GTK_EDITABLE(app->reference), "John 3:16");
  gtk_widget_set_hexpand(app->reference, TRUE); gtk_box_append(GTK_BOX(toolbar), app->reference);
  app->go = gtk_button_new_with_label("Go"); gtk_box_append(GTK_BOX(toolbar), app->go);
  auto *display_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_append(GTK_BOX(box), display_bar);
  app->previous_chapter = gtk_button_new_with_label("Previous chapter");
  app->next_chapter = gtk_button_new_with_label("Next chapter");
  gtk_box_append(GTK_BOX(display_bar), app->previous_chapter); gtk_box_append(GTK_BOX(display_bar), app->next_chapter);
  const char *selector_names[] = {"Book", "Chapter", "Verse"};
  for (int k = 0; k < 3; ++k) {
    app->selectors[k] = gtk_drop_down_new(nullptr, nullptr);
    g_object_set_data(G_OBJECT(app->selectors[k]), "choice-kind", GINT_TO_POINTER(k));
    gtk_accessible_update_property(GTK_ACCESSIBLE(app->selectors[k]), GTK_ACCESSIBLE_PROPERTY_LABEL, selector_names[k], -1);
    gtk_box_append(GTK_BOX(display_bar), app->selectors[k]);
    g_signal_connect(app->selectors[k], "notify::selected", G_CALLBACK(+[](GObject *object, GParamSpec *, gpointer data) {
      auto *app = static_cast<App *>(data);
      if (app->updating || !app->model || app->model->busy()) return;
      const int kind = GPOINTER_TO_INT(g_object_get_data(object, "choice-kind"));
      const auto index = gtk_drop_down_get_selected(GTK_DROP_DOWN(object)); const auto &choices = app->model->choices(kind);
      if (index >= choices.size()) return;
      const auto passage = app->model->passage(); const auto target = choices[index].target;
      app->submitted_draft = gtk_editable_get_text(GTK_EDITABLE(app->reference));
      app->model->visit(passage.bible, passage.commentary, target);
    }), app);
  }
  app->chapter_mode = gtk_check_button_new_with_label("Bible chapter");
  app->commentary_mode = gtk_check_button_new_with_label("Commentary chapter");
  app->numbers = gtk_check_button_new_with_label("Verse numbers");
  app->highlight = gtk_check_button_new_with_label("Highlight selected");
  for (auto *control : {app->chapter_mode, app->commentary_mode, app->numbers, app->highlight}) {
    gtk_box_append(GTK_BOX(display_bar), control);
    g_signal_connect(control, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
      auto *app = static_cast<App *>(data); if (app->updating || !app->model || app->model->busy()) return;
      if (GTK_WIDGET(button) == app->commentary_mode) app->model->set_commentary_chapter(gtk_check_button_get_active(button));
      else {
        unsigned flags = (gtk_check_button_get_active(GTK_CHECK_BUTTON(app->numbers)) ? XC_CHAPTER_VERSE_NUMBERS : 0u)
          | (gtk_check_button_get_active(GTK_CHECK_BUTTON(app->highlight)) ? XC_CHAPTER_HIGHLIGHT_SELECTED : 0u);
        app->model->set_display(gtk_check_button_get_active(GTK_CHECK_BUTTON(app->chapter_mode)), flags);
      }
    }), app);
  }
  app->clear_history = gtk_button_new_with_label("Clear history"); gtk_box_append(GTK_BOX(display_bar), app->clear_history);
  g_signal_connect(app->clear_history, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<App *>(data)->model->clear_history(); }), app);
  for (auto *button : {app->previous_chapter, app->next_chapter}) {
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
      auto *app = static_cast<App *>(data); app->submitted_draft = gtk_editable_get_text(GTK_EDITABLE(app->reference));
      app->model->navigate_chapter(GTK_WIDGET(button) == app->previous_chapter ? -1 : 1);
    }), app);
  }
  auto *panes = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL); gtk_widget_set_vexpand(panes, TRUE);
  gtk_box_append(GTK_BOX(box), panes);
  auto *library = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  auto *bible_label = gtk_label_new_with_mnemonic("_Bible module");
  gtk_box_append(GTK_BOX(library), bible_label);
  app->bible = gtk_drop_down_new(nullptr, nullptr); gtk_box_append(GTK_BOX(library), app->bible);
  auto *commentary_label = gtk_label_new_with_mnemonic("_Commentary module");
  gtk_box_append(GTK_BOX(library), commentary_label);
  app->commentary = gtk_drop_down_new(nullptr, nullptr); gtk_box_append(GTK_BOX(library), app->commentary);
  gtk_label_set_mnemonic_widget(GTK_LABEL(bible_label), app->bible);
  gtk_label_set_mnemonic_widget(GTK_LABEL(commentary_label), app->commentary);
  gtk_accessible_update_property(GTK_ACCESSIBLE(app->bible), GTK_ACCESSIBLE_PROPERTY_LABEL, "Bible module", -1);
  gtk_accessible_update_property(GTK_ACCESSIBLE(app->commentary), GTK_ACCESSIBLE_PROPERTY_LABEL, "Commentary module", -1);
  gtk_accessible_update_property(GTK_ACCESSIBLE(app->reference), GTK_ACCESSIBLE_PROPERTY_LABEL, "Passage reference", -1);
  gtk_widget_set_size_request(library, 220, -1); gtk_paned_set_start_child(GTK_PANED(panes), library);
  auto *study = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_end_child(GTK_PANED(panes), study); gtk_paned_set_position(GTK_PANED(panes), 240);
  gtk_paned_set_start_child(GTK_PANED(study), pane(app, "Bible", &app->bible_view));
  auto *right = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_end_child(GTK_PANED(study), right); gtk_paned_set_position(GTK_PANED(study), 520);
  gtk_paned_set_start_child(GTK_PANED(right), pane(app, "Commentary", &app->commentary_view));
  auto *dictionary_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_paned_set_end_child(GTK_PANED(right), dictionary_box);
  app->dictionary_module = gtk_drop_down_new(nullptr, nullptr); gtk_box_append(GTK_BOX(dictionary_box), app->dictionary_module);
  gtk_accessible_update_property(GTK_ACCESSIBLE(app->dictionary_module), GTK_ACCESSIBLE_PROPERTY_LABEL, "Dictionary module", -1);
  auto *lookup_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4); gtk_box_append(GTK_BOX(dictionary_box), lookup_bar);
  app->dictionary_key = gtk_entry_new(); gtk_widget_set_hexpand(app->dictionary_key, TRUE); gtk_box_append(GTK_BOX(lookup_bar), app->dictionary_key);
  gtk_accessible_update_property(GTK_ACCESSIBLE(app->dictionary_key), GTK_ACCESSIBLE_PROPERTY_LABEL, "Dictionary word or key", -1);
  app->dictionary_lookup = gtk_button_new_with_label("Look up"); gtk_box_append(GTK_BOX(lookup_bar), app->dictionary_lookup);
  app->dictionary_nearby = gtk_drop_down_new(nullptr, nullptr); gtk_box_append(GTK_BOX(lookup_bar), app->dictionary_nearby);
  gtk_accessible_update_property(GTK_ACCESSIBLE(app->dictionary_nearby), GTK_ACCESSIBLE_PROPERTY_LABEL, "Nearby dictionary keys", -1);
  auto *dictionary_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4); gtk_box_append(GTK_BOX(dictionary_box), dictionary_bar);
  app->dictionary_previous = gtk_button_new_with_label("Previous key"); app->dictionary_next = gtk_button_new_with_label("Next key");
  app->dictionary_back = gtk_button_new_with_label("Back"); app->dictionary_forward = gtk_button_new_with_label("Forward");
  app->dictionary_clear = gtk_button_new_with_label("Clear history"); app->dictionary_close = gtk_button_new_with_label("Close");
  gtk_accessible_update_property(GTK_ACCESSIBLE(app->dictionary_back), GTK_ACCESSIBLE_PROPERTY_LABEL, "Dictionary history Back", -1);
  gtk_accessible_update_property(GTK_ACCESSIBLE(app->dictionary_forward), GTK_ACCESSIBLE_PROPERTY_LABEL, "Dictionary history Forward", -1);
  for (auto *button : {app->dictionary_previous, app->dictionary_next, app->dictionary_back, app->dictionary_forward, app->dictionary_clear, app->dictionary_close}) {
    gtk_box_append(GTK_BOX(dictionary_bar), button);
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
      auto *app = static_cast<App *>(data);
      app->dictionary_submitted = gtk_editable_get_text(GTK_EDITABLE(app->dictionary_key));
      if (GTK_WIDGET(button) == app->dictionary_previous) app->model->step_dictionary(-1);
      else if (GTK_WIDGET(button) == app->dictionary_next) app->model->step_dictionary(1);
      else if (GTK_WIDGET(button) == app->dictionary_back) app->model->navigate_dictionary_history(-1);
      else if (GTK_WIDGET(button) == app->dictionary_forward) app->model->navigate_dictionary_history(1);
      else if (GTK_WIDGET(button) == app->dictionary_clear) app->model->clear_dictionary_history();
      else app->model->close_dictionary();
    }), app);
  }
  g_signal_connect(app->dictionary_lookup, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
    auto *app = static_cast<App *>(data); app->dictionary_submitted = gtk_editable_get_text(GTK_EDITABLE(app->dictionary_key));
    app->model->lookup_dictionary(selected(app->dictionary_module, app->dictionaries), app->dictionary_submitted);
  }), app);
  g_signal_connect(app->dictionary_key, "activate", G_CALLBACK(+[](GtkEntry *, gpointer data) { g_signal_emit_by_name(static_cast<App *>(data)->dictionary_lookup, "clicked"); }), app);
  g_signal_connect(app->dictionary_nearby, "notify::selected", G_CALLBACK(+[](GObject *object, GParamSpec *, gpointer data) {
    auto *app = static_cast<App *>(data); if (app->updating || !app->model || app->model->busy()) return;
    const auto index = gtk_drop_down_get_selected(GTK_DROP_DOWN(object)); const auto &keys = app->model->dictionary_keys();
    if (index >= keys.size()) return;
    gtk_editable_set_text(GTK_EDITABLE(app->dictionary_key), keys[index].c_str()); g_signal_emit_by_name(app->dictionary_lookup, "clicked");
  }), app);
  gtk_box_append(GTK_BOX(dictionary_box), pane(app, "Dictionary", &app->dictionary_view));
  gtk_paned_set_position(GTK_PANED(right), 440);
  gtk_accessible_update_property(GTK_ACCESSIBLE(app->bible_view), GTK_ACCESSIBLE_PROPERTY_LABEL, "Bible passage", -1);
  gtk_accessible_update_property(GTK_ACCESSIBLE(app->commentary_view), GTK_ACCESSIBLE_PROPERTY_LABEL, "Commentary passage", -1);
  app->status = gtk_label_new("Starting study engine…"); gtk_box_append(GTK_BOX(box), app->status);
  const GActionEntry actions[] = {
    {"go", +[](GSimpleAction *, GVariant *, gpointer data) { go(static_cast<App *>(data)); }, nullptr, nullptr, nullptr, {}},
    {"back", +[](GSimpleAction *, GVariant *, gpointer data) { auto *app = static_cast<App *>(data); app->submitted_draft = gtk_editable_get_text(GTK_EDITABLE(app->reference)); app->model->navigate(-1); }, nullptr, nullptr, nullptr, {}},
    {"forward", +[](GSimpleAction *, GVariant *, gpointer data) { auto *app = static_cast<App *>(data); app->submitted_draft = gtk_editable_get_text(GTK_EDITABLE(app->reference)); app->model->navigate(1); }, nullptr, nullptr, nullptr, {}},
    {"focus-reference", +[](GSimpleAction *, GVariant *, gpointer data) { gtk_widget_grab_focus(static_cast<App *>(data)->reference); }, nullptr, nullptr, nullptr, {}}
  };
  g_action_map_add_action_entries(G_ACTION_MAP(app->window), actions, G_N_ELEMENTS(actions), app);
  for (const auto &entry : actions) g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(app->window), entry.name)), FALSE);
  gtk_actionable_set_action_name(GTK_ACTIONABLE(app->go), "win.go");
  gtk_actionable_set_action_name(GTK_ACTIONABLE(app->back), "win.back");
  gtk_actionable_set_action_name(GTK_ACTIONABLE(app->forward), "win.forward");
  const char *back_keys[] = {"<Alt>Left", nullptr}, *forward_keys[] = {"<Alt>Right", nullptr};
  const char *focus_keys[] = {"<Primary>l", nullptr};
  gtk_application_set_accels_for_action(application, "win.back", back_keys);
  gtk_application_set_accels_for_action(application, "win.forward", forward_keys);
  gtk_application_set_accels_for_action(application, "win.focus-reference", focus_keys);
  gtk_widget_set_tooltip_text(app->back, "Back (Alt+Left)");
  gtk_widget_set_tooltip_text(app->forward, "Forward (Alt+Right)");
  gtk_widget_set_tooltip_text(app->reference, "Passage reference (Ctrl+L to focus; Enter to go)");
  g_signal_connect(app->reference, "activate", G_CALLBACK(+[](GtkEntry *, gpointer data) {
    auto *app = static_cast<App *>(data); gtk_widget_activate_action(app->window, "win.go", nullptr);
  }), app);
  g_signal_connect(app->window, "close-request", G_CALLBACK(+[](GtkWindow *, gpointer data) -> gboolean { shutdown(static_cast<App *>(data)); return FALSE; }), app);
  try { app->model = std::make_unique<ReaderModel>(app->root.empty() ? nullptr : app->root.c_str()); }
  catch (const std::exception &error) { gtk_label_set_text(GTK_LABEL(app->status), error.what()); gtk_widget_set_sensitive(box, FALSE); gtk_window_present(GTK_WINDOW(app->window)); return; }
  app->timer = g_timeout_add(10, tick, app);
  tick(app); gtk_window_present(GTK_WINDOW(app->window));
}
}
int main(int argc, char **argv) {
  std::setlocale(LC_ALL, "");
  App state;
  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    if (argument == "--software-rendering") state.software = true;
    else if ((argument == "--root" || argument == "--smoke-test") && i + 1 < argc) {
      state.root = argv[++i];
      if (argument == "--smoke-test") state.smoke = state.software = true;
    } else {
      std::cerr << "Usage: xiphos-linux-preview [--software-rendering] [--root SWORD_LIBRARY]\n";
      return argument == "--help" ? 0 : 2;
    }
  }
  if (state.software) g_setenv("GSK_RENDERER", "cairo", FALSE); // Preserve an explicit user renderer override.
  if (!state.root.empty()) state.root = std::filesystem::absolute(state.root).string();
  auto *app = gtk_application_new("org.xiphos.LinuxPreview", G_APPLICATION_NON_UNIQUE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), &state);
  guint deadline = 0;
  if (state.smoke) deadline = g_timeout_add_seconds(90, +[](gpointer data) -> gboolean {
    auto *state = static_cast<App *>(data); state->smoke_failed = true;
    std::cerr << "GTK4 smoke timeout at stage " << state->smoke_stage << " loaded=" << state->loaded << "\n";
    if (state->window) gtk_window_close(GTK_WINDOW(state->window));
    return G_SOURCE_REMOVE;
  }, &state);
  char *args[] = {argv[0], nullptr};
  auto result = g_application_run(G_APPLICATION(app), 1, args);
  if (deadline && !state.smoke_failed) g_source_remove(deadline);
  shutdown(&state); g_object_unref(app);
  if (state.smoke) {
    if (state.smoke_failed || state.smoke_stage != 13) { std::cerr << "GTK4 smoke failed\n"; return 1; }
    std::cout << "GTK4 smoke passed: focus action, module selection, draft retention, paired panes, navigation actions, chapters, selectors, clear history, dictionary lookup/keys/close and pending close\n";
  }
  return result;
}
