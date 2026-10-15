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
  WebKitWebView *bible_view = nullptr, *commentary_view = nullptr;
  std::unique_ptr<ReaderModel> model;
  std::vector<std::string> bibles, commentaries{""};
  std::string root, rendered;
  guint timer = 0;
  bool populated = false, closing = false, smoke = false, smoke_failed = false;
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
  app->model->visit(selected(app->bible, app->bibles), selected(app->commentary, app->commentaries), gtk_editable_get_text(GTK_EDITABLE(app->reference)));
}
std::string document(const std::string &body) {
  // No module scripts, external resources, frames or network requests. Native
  // policy routes user passage clicks through C++; no browser navigation engine.
  return "<!doctype html><html><head><meta charset='utf-8'><meta http-equiv='Content-Security-Policy' content=\"default-src 'none'; style-src 'unsafe-inline'; img-src 'none'; script-src 'none'; frame-src 'none'; connect-src 'none'\"><style>body{font:18px serif;line-height:1.55;margin:20px;color:#202020;background:#fff}a{color:#174ea6}</style></head><body>" + body + "</body></html>";
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
        app->model->follow(view == app->bible_view ? passage.bible : passage.commentary, uri ? uri : "");
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
  if (app->smoke) webkit_settings_set_hardware_acceleration_policy(webkit_web_view_get_settings(*view), WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);
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
    auto *bibles = gtk_string_list_new(nullptr), *commentaries = gtk_string_list_new(nullptr);
    gtk_string_list_append(commentaries, "No commentary");
    for (const auto &module : app->model->modules()) {
      const auto label = module.id + " — " + module.description;
      if (module.type == "Biblical Texts") { app->bibles.push_back(module.id); gtk_string_list_append(bibles, label.c_str()); }
      if (module.type == "Commentaries") { app->commentaries.push_back(module.id); gtk_string_list_append(commentaries, label.c_str()); }
    }
    gtk_drop_down_set_model(GTK_DROP_DOWN(app->bible), G_LIST_MODEL(bibles));
    gtk_drop_down_set_model(GTK_DROP_DOWN(app->commentary), G_LIST_MODEL(commentaries));
    g_object_unref(bibles); g_object_unref(commentaries); app->populated = true;
  }
  const auto &passage = app->model->passage();
  const auto identity = passage.bible + "\n" + passage.commentary + "\n" + passage.reference + "\n" + passage.bible_html + passage.commentary_html;
  if (!passage.bible.empty() && identity != app->rendered) {
    app->rendered = identity;
    select(app->bible, app->bibles, passage.bible); select(app->commentary, app->commentaries, passage.commentary);
    gtk_editable_set_text(GTK_EDITABLE(app->reference), passage.reference.c_str());
    webkit_web_view_load_html(app->bible_view, document(passage.bible_html).c_str(), "xiphos-preview://study/");
    webkit_web_view_load_html(app->commentary_view, document(passage.commentary_html).c_str(), "xiphos-preview://study/");
  }
  const bool enabled = app->model->ready() && !app->model->busy();
  gtk_widget_set_sensitive(app->go, enabled && !app->bibles.empty());
  gtk_widget_set_sensitive(app->back, enabled && app->model->can_back());
  gtk_widget_set_sensitive(app->forward, enabled && app->model->can_forward());
  gtk_widget_set_sensitive(app->bible, enabled); gtk_widget_set_sensitive(app->commentary, enabled);
  const auto &error = app->model->error();
  gtk_label_set_text(GTK_LABEL(app->status), !error.empty() ? error.c_str() : app->model->busy() ? "Loading…" : app->populated && app->bibles.empty() ? "No Bible modules found. Open a SWORD library with --root DIRECTORY." : "Ready");
  if (app->smoke && enabled) {
    if (!error.empty()) {
      app->smoke_failed = true; std::cerr << error << "\n"; gtk_window_close(GTK_WINDOW(app->window)); return G_SOURCE_REMOVE;
    }
    switch (app->smoke_stage) {
    case 0:
      select(app->bible, app->bibles, "Text"); select(app->commentary, app->commentaries, "Notes");
      g_signal_emit_by_name(app->reference, "activate"); ++app->smoke_stage; break;
    case 1:
      if (passage.reference != "John 3:16" || passage.commentary != "Notes") app->smoke_failed = true;
      gtk_editable_set_text(GTK_EDITABLE(app->reference), "John 3:17");
      g_signal_emit_by_name(app->go, "clicked"); ++app->smoke_stage; break;
    case 2:
      if (!app->model->can_back()) app->smoke_failed = true;
      g_signal_emit_by_name(app->back, "clicked"); ++app->smoke_stage; break;
    case 3:
      if (passage.reference != "John 3:16" || !app->model->can_forward()) app->smoke_failed = true;
      g_signal_emit_by_name(app->forward, "clicked"); ++app->smoke_stage; break;
    case 4:
      if (app->loaded < 2) break;
      if (passage.reference != "John 3:17") app->smoke_failed = true;
      g_signal_emit_by_name(app->go, "clicked"); // accepted work must drain during close
      ++app->smoke_stage; gtk_window_close(GTK_WINDOW(app->window)); return G_SOURCE_REMOVE;
    }
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
  auto *panes = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL); gtk_widget_set_vexpand(panes, TRUE);
  gtk_box_append(GTK_BOX(box), panes);
  auto *library = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_box_append(GTK_BOX(library), gtk_label_new("Bible module"));
  app->bible = gtk_drop_down_new(nullptr, nullptr); gtk_box_append(GTK_BOX(library), app->bible);
  gtk_box_append(GTK_BOX(library), gtk_label_new("Commentary module"));
  app->commentary = gtk_drop_down_new(nullptr, nullptr); gtk_box_append(GTK_BOX(library), app->commentary);
  gtk_widget_set_size_request(library, 220, -1); gtk_paned_set_start_child(GTK_PANED(panes), library);
  auto *study = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_end_child(GTK_PANED(panes), study); gtk_paned_set_position(GTK_PANED(panes), 240);
  gtk_paned_set_start_child(GTK_PANED(study), pane(app, "Bible", &app->bible_view));
  auto *right = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_end_child(GTK_PANED(study), right); gtk_paned_set_position(GTK_PANED(study), 520);
  gtk_paned_set_start_child(GTK_PANED(right), pane(app, "Commentary", &app->commentary_view));
  gtk_paned_set_end_child(GTK_PANED(right), gtk_label_new("Dictionary, books and devotional panes are not implemented in this preview."));
  gtk_paned_set_position(GTK_PANED(right), 440);
  app->status = gtk_label_new("Starting study engine…"); gtk_box_append(GTK_BOX(box), app->status);
  g_signal_connect(app->go, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { go(static_cast<App *>(data)); }), app);
  g_signal_connect(app->reference, "activate", G_CALLBACK(+[](GtkEntry *, gpointer data) { go(static_cast<App *>(data)); }), app);
  g_signal_connect(app->back, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<App *>(data)->model->navigate(-1); }), app);
  g_signal_connect(app->forward, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { static_cast<App *>(data)->model->navigate(1); }), app);
  g_signal_connect(app->window, "close-request", G_CALLBACK(+[](GtkWindow *, gpointer data) -> gboolean { shutdown(static_cast<App *>(data)); return FALSE; }), app);
  try { app->model = std::make_unique<ReaderModel>(app->root.empty() ? nullptr : app->root.c_str()); }
  catch (const std::exception &error) { gtk_label_set_text(GTK_LABEL(app->status), error.what()); gtk_widget_set_sensitive(toolbar, FALSE); gtk_window_present(GTK_WINDOW(app->window)); return; }
  app->timer = g_timeout_add(10, tick, app);
  tick(app); gtk_window_present(GTK_WINDOW(app->window));
}
}
int main(int argc, char **argv) {
  std::setlocale(LC_ALL, "");
  App state;
  if (argc == 3 && (std::string(argv[1]) == "--root" || std::string(argv[1]) == "--smoke-test")) { state.root = argv[2]; state.smoke = std::string(argv[1]) == "--smoke-test"; }
  else if (argc != 1) { std::cerr << "Usage: xiphos-linux-preview [--root SWORD_LIBRARY]\n"; return 2; }
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
    if (state.smoke_failed || state.smoke_stage != 5) { std::cerr << "GTK4 smoke failed\n"; return 1; }
    std::cout << "GTK4 smoke passed: module selection, entry, paired panes, Back/Forward and pending close\n";
  }
  return result;
}
