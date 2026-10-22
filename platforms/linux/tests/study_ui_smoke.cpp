// SPDX-License-Identifier: GPL-2.0-or-later
#include "study_ui.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
namespace xiphos::linux_preview {
struct StudyUISmoke {
  GtkApplication *app = nullptr; GtkWindow *window = nullptr;
  std::unique_ptr<ReaderModel> model; std::unique_ptr<StudyUI> ui;
  std::string root; unsigned stage = 0, ticks = 0; bool failed = false;
  void check(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
  gboolean tick() {
    try {
      check(++ticks < 6000, "Desktop smoke deadline"); model->tick(); ui->tick(*model);
      if (!model->ready() || model->busy()) return G_SOURCE_CONTINUE;
      check(model->error().empty(), model->error().c_str());
      switch (stage) {
      case 0: {
        size_t i = 0; while (i < ui->books_.size() && ui->books_[i] != "StyledBook") ++i;
        check(i < ui->books_.size(), "Styled fixture available"); gtk_drop_down_set_selected(GTK_DROP_DOWN(ui->book_module_), i); ++stage; break;
      }
      case 1:
        check(model->book().module == "StyledBook", "Book module signal publishes");
        gtk_editable_set_text(GTK_EDITABLE(ui->book_key_), "/Styled"); g_signal_emit_by_name(ui->book_read_, "clicked"); ++stage; break;
      case 2:
        check(model->book().nodes[0].path == "/Styled", "Read signal uses core path");
        gtk_check_button_set_active(GTK_CHECK_BUTTON(ui->book_options_[2]), TRUE); ++stage; break;
      case 3:
        check(model->book().spacing == 1, "Book option signal refreshes core CSS");
        gtk_editable_set_text(GTK_EDITABLE(ui->search_modules_), "Text,Commentary,Text"); gtk_editable_set_text(GTK_EDITABLE(ui->search_text_), "Original");
        g_signal_emit_by_name(ui->search_start_, "clicked"); ++stage; break;
      case 4:
        if (!model->search_results().revision) break;
        check(model->search_results().hits.size() == 12, "Ordered search controls publish all groups");
        gtk_list_box_select_row(GTK_LIST_BOX(ui->results_), gtk_list_box_get_row_at_index(GTK_LIST_BOX(ui->results_), 4)); ++stage; break;
      case 5:
        check(model->search_preview().module == "Commentary", "Selected row preview keeps source");
        g_signal_emit_by_name(ui->results_, "row-activated", gtk_list_box_get_row_at_index(GTK_LIST_BOX(ui->results_), 4)); ++stage; break;
      case 6:
        check(model->passage().bible == "Commentary", "Activated row uses actual module");
        check(gtk_widget_get_sensitive(ui->search_start_), "Search controls recover after reading");
        // Destination tests deliberately do not touch the user's clipboard.
        check(model->export_search(true, false, {4}, true, true, false, false), "Queue HTML export before window close");
        ui->close(); model->detach(); gtk_window_destroy(window); window = nullptr;
        g_application_quit(G_APPLICATION(app)); return G_SOURCE_REMOVE;
      }
      return G_SOURCE_CONTINUE;
    } catch (const std::exception &e) {
      std::cerr << "GTK study smoke stage " << stage << ": " << e.what() << '\n'; failed = true;
      ui->close(); if (window) { gtk_window_destroy(window); window = nullptr; } g_application_quit(G_APPLICATION(app)); return G_SOURCE_REMOVE;
    }
  }
};
}
int main(int argc, char **argv) {
  using namespace xiphos::linux_preview;
  if (argc != 2) { std::cerr << "Combined original fixture root required\n"; return 2; }
  g_setenv("GSK_RENDERER", "cairo", FALSE);
  if (!gtk_init_check()) { std::cerr << "No accessible GTK display; desktop smoke unavailable\n"; return 77; }
  StudyUISmoke state; state.root = argv[1]; state.app = gtk_application_new("org.xiphos.StudySmoke", G_APPLICATION_NON_UNIQUE);
  xc_worker *worker = nullptr;
  if (xc_worker_open(state.root.c_str(), &worker) != XC_OK) return 1;
  g_signal_connect(state.app, "activate", G_CALLBACK(+[](GtkApplication *app, gpointer data) {
    auto *s = static_cast<StudyUISmoke *>(data); s->window = GTK_WINDOW(gtk_application_window_new(app));
    s->ui = std::make_unique<StudyUI>(s->window, true); auto *tabs = gtk_notebook_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), s->ui->book_pane(), gtk_label_new("Book"));
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), s->ui->search_pane(), gtk_label_new("Search"));
    gtk_window_set_child(s->window, tabs); gtk_window_set_default_size(s->window, 750, 700); gtk_window_present(s->window);
    g_timeout_add(10, +[](gpointer p) -> gboolean { return static_cast<StudyUISmoke *>(p)->tick(); }, s);
  }), &state);
  state.model = std::make_unique<ReaderModel>(*worker);
  char *args[] = {argv[0], nullptr}; g_application_run(G_APPLICATION(state.app), 1, args);
  state.ui->close(); state.model.reset(); xc_worker_close(worker); state.ui.reset(); g_object_unref(state.app);
  if (!state.failed) std::cout << "GTK book/search controls, source activation and pending export close passed\n";
  return state.failed ? 1 : 0;
}
