// SPDX-License-Identifier: GPL-2.0-or-later
#include "study_ui.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
namespace xiphos::linux_preview {
struct StudyUISmoke {
  GtkApplication *app = nullptr; GtkWindow *window = nullptr;
  std::unique_ptr<ReaderModel> model; std::unique_ptr<StudyUI> ui;
  std::string root, scratch;
  bool dom_pending = false, dom_done = false, dom_ok = false;
  unsigned stage = 0, ticks = 0;
  bool failed = false;
  void check(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
  GtkWidget *find_button(GtkWidget *parent, const char *label)
  {
	  if (GTK_IS_BUTTON(parent) && g_strcmp0(gtk_button_get_label(GTK_BUTTON(parent)), label) == 0)
		  return parent;
	  for (auto *child = gtk_widget_get_first_child(parent); child; child = gtk_widget_get_next_sibling(child))
		  if (auto *found = find_button(child, label))
			  return found;
	  return nullptr;
  }
  void click(GtkWidget *parent, const char *label)
  {
	  auto *button = find_button(parent, label);
	  check(button, "Workflow button exists");
	  g_signal_emit_by_name(button, "clicked");
  }
  gboolean tick() {
    try {
      check(++ticks < 6000, "Desktop smoke deadline"); model->tick(); ui->tick(*model);
      if (!model->ready() || model->busy()) return G_SOURCE_CONTINUE;
      check(model->error().empty(), model->error().c_str());
      switch (stage) {
      case 0: {
        size_t i = 0; while (i < ui->books_.size() && ui->books_[i] != "StyledBook") ++i;
	check(i < ui->books_.size(), "Styled fixture available");
	gtk_drop_down_set_selected(GTK_DROP_DOWN(ui->book_module_), i + 1);
	++stage;
	break;
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
	gtk_editable_set_text(GTK_EDITABLE(ui->parallel_modules_), "Text,Commentary,Text");
	gtk_editable_set_text(GTK_EDITABLE(ui->parallel_reference_), "John1:1");
	click(ui->parallel_box_, "Read");
	++stage;
	break;
      case 7:
	      if (!model->parallel().revision || webkit_web_view_is_loading(ui->parallel_view_) || GPOINTER_TO_INT(g_object_get_data(G_OBJECT(ui->parallel_view_), "parallel-ready")) != int(model->parallel().revision))
		      break;
	      check(model->parallel().columns.size() == 3 && model->parallel().columns[0].module == "Text", "Native parallel control keeps ordered duplicates");
	      if (!dom_pending) {
		      dom_pending = true;
		      const char *script = R"JS(
(() => {
  const table = document.querySelector('body > table');
  const cell = Array.from(table?.tBodies[0]?.rows || [])
    .map(row => row.cells[1]).find(cell => cell.textContent.trim().length > 0);
  if (!cell) return false;
  cell.innerHTML = `<span onclick="this.dataset.executed='yes'">Original inline handler</span>
    <table><tr><td data-row="0" data-column="1">
    <a href="sword://Text/John1:2">Original nested cell</a></td></tr></table>`;
  const span = cell.querySelector('span');
  span.click();
  if (span.dataset.executed) return false;
  cell.querySelector('a').click();
  return table.tBodies[0].rows[0].cells.length === 4;
})()
)JS";
		      webkit_web_view_evaluate_javascript(ui->parallel_view_, script, -1, "xiphos-host", nullptr, nullptr, +[](GObject *view, GAsyncResult *result, gpointer data) {auto*s=static_cast<StudyUISmoke*>(data);GError*error=nullptr;auto*value=webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(view),result,&error);if(error)std::cerr<<"Parallel script: "<<error->message<<"\n";s->dom_ok=value&&!error&&jsc_value_to_boolean(value);s->dom_done=true;g_clear_object(&value);g_clear_error(&error); }, this);
	      }
	      if (!dom_done)
		      break;
	      check(dom_ok, "Parallel markup stays inside its cell and inline scripts stay disabled");
	      ++stage;
	      break;
      case 8:
	      if (model->passage().bible != "Text" || model->passage().reference != "John 1:2")
		      break;
	      check(model->open_editor(scratch + "/source.html", true), "New native editor source");
	      ++stage;
	      break;
      case 9:
	      gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(ui->editor_text_)), "Ω draft", -1);
	      click(ui->editor_box_, "Rich");
	      check(ui->rich_mode_, "Supported source opens in Rich mode");
	      {
		      auto *b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ui->rich_editor_->widget()));
		      GtkTextIter a, z;
		      gtk_text_buffer_get_iter_at_offset(b, &a, 0);
		      gtk_text_buffer_get_iter_at_offset(b, &z, 1);
		      gtk_text_buffer_select_range(b, &a, &z);
	      }
	      click(ui->editor_box_, "Bold");
	      ++stage;
	      break;
      case 10:
	      check(model->editor().text.find("<b>Ω</b>") != std::string::npos, "Native format uses UTF-8 core selection");
	      click(ui->editor_box_, "Save");
	      ++stage;
	      break;
      case 11:
	      check(!model->editor_dirty() && model->editor().revision.has_value(), "Native source saved with revision");
	      check(model->open_annotations(scratch + "/notes.xml", true), "Explicit note file");
	      click(ui->notes_box_, "Edit current verse note");
	      ++stage;
	      break;
      case 12:
	      check(model->note_draft().reference == "John 1:2", "Native note captures current reference");
	      gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(ui->note_text_)), "Original note Ω <plain>", -1);
	      click(ui->notes_box_, "Save verse note");
	      ++stage;
	      break;
      case 13:
	      check(model->note_draft().text == "Original note Ω <plain>", "Saved note baseline copies plain text");
	      {
		      const auto host = ui->capture_state("");
		      check(StudyUI::validate_state(host) && StudyUI::reference_draft(host) == std::optional<std::string>(""), "Host state preserves explicit empty reference draft");
		      gtk_editable_set_text(GTK_EDITABLE(ui->search_text_), "changed");
		      check(ui->restore_state(host), "Restore copied host controls");
		      check(std::string(gtk_editable_get_text(GTK_EDITABLE(ui->search_text_))) == "Original", "Search draft restored");
	      }
	      ++stage;
	      break;
      case 14:
	      check(model->decorated().html.find("Original note Ω") != std::string::npos, "Native reading decoration refreshes saved note");
	      // Destination tests deliberately do not touch the user's clipboard.
	      check(model->export_search(true, false, {4}, true, true, false, false), "Queue HTML export before window close");
	      ui->close();
	      model->detach();
	      gtk_window_destroy(window);
	      window = nullptr;
	      g_application_quit(G_APPLICATION(app));
	      return G_SOURCE_REMOVE;
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
  StudyUISmoke state;
  {
	  auto *folder = g_dir_make_tmp("xiphos-gtk-workflows-XXXXXX", nullptr);
	  if (!folder)
		  return 1;
	  state.scratch = folder;
	  g_free(folder);
  }
  state.root = argv[1];
  state.app = gtk_application_new("org.xiphos.StudySmoke", G_APPLICATION_NON_UNIQUE);
  xc_worker *worker = nullptr;
  if (xc_worker_open(state.root.c_str(), &worker) != XC_OK) return 1;
  g_signal_connect(state.app, "activate", G_CALLBACK(+[](GtkApplication *app, gpointer data) {
    auto *s = static_cast<StudyUISmoke *>(data); s->window = GTK_WINDOW(gtk_application_window_new(app));
    s->ui = std::make_unique<StudyUI>(s->window, true); auto *tabs = gtk_notebook_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), s->ui->book_pane(), gtk_label_new("Book"));
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), s->ui->search_pane(), gtk_label_new("Search"));
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), s->ui->parallel_pane(), gtk_label_new("Parallel"));
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), s->ui->notes_pane(), gtk_label_new("Notes"));
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), s->ui->editor_pane(), gtk_label_new("Editor"));
    gtk_window_set_child(s->window, tabs); gtk_window_set_default_size(s->window, 750, 700); gtk_window_present(s->window);
    g_timeout_add(10, +[](gpointer p) -> gboolean { return static_cast<StudyUISmoke *>(p)->tick(); }, s);
  }), &state);
  state.model = std::make_unique<ReaderModel>(*worker);
  char *args[] = {argv[0], nullptr}; g_application_run(G_APPLICATION(state.app), 1, args);
  state.ui->close(); state.model.reset(); xc_worker_close(worker); state.ui.reset(); g_object_unref(state.app);
  std::filesystem::remove_all(state.scratch);
  if (!state.failed)
	  std::cout << "GTK book/search, trusted parallel cells, Unicode rich editing, notes, host state and pending export close passed\n";
  return state.failed ? 1 : 0;
}
