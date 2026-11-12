// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_document.hpp"
#include "study_ui.hpp"
namespace xiphos::linux_preview
{
namespace
{
GtkWidget *row(GtkWidget *box)
{
	auto *w = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_append(GTK_BOX(box), w);
	return w;
}
GtkWidget *entry(GtkWidget *box, const char *name)
{
	auto *w = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(w), name);
	gtk_accessible_update_property(GTK_ACCESSIBLE(w), GTK_ACCESSIBLE_PROPERTY_LABEL, name, -1);
	gtk_box_append(GTK_BOX(box), w);
	return w;
}
GtkWidget *text_view(GtkWidget *box, const char *name)
{
	auto *w = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(w), GTK_WRAP_WORD_CHAR);
	auto *scroll = gtk_scrolled_window_new();
	gtk_widget_set_size_request(scroll, -1, 130);
	gtk_widget_set_vexpand(scroll, TRUE);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), w);
	gtk_box_append(GTK_BOX(box), scroll);
	gtk_accessible_update_property(GTK_ACCESSIBLE(w), GTK_ACCESSIBLE_PROPERTY_LABEL, name, -1);
	return w;
}
std::string text(GtkWidget *w)
{
	if (GTK_IS_EDITABLE(w))
		return gtk_editable_get_text(GTK_EDITABLE(w));
	auto *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	auto *p = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
	std::string s(p);
	g_free(p);
	return s;
}
void set_text(GtkWidget *w, const std::string &s) { gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(w)), s.c_str(), s.size()); }
} // namespace
GtkWidget *StudyUI::notes_pane()
{
	auto *scroll = gtk_scrolled_window_new();
	notes_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), notes_box_);
	auto *bar = row(notes_box_);
	const char *files[] = {"Open bookmarks…", "New bookmarks…", "Open notes…", "New notes…"};
	for (int i = 0; i < 4; ++i) {
		if (i == 2)
			bar = row(notes_box_);
		auto *w = gtk_button_new_with_label(files[i]);
		gtk_box_append(GTK_BOX(bar), w);
		g_object_set_data(G_OBJECT(w), "kind", GINT_TO_POINTER(i + 9));
		g_signal_connect(w, "clicked", G_CALLBACK((+[](GtkButton *w, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->note_text_ && s->model_ && text(s->note_text_) != s->model_->note_draft().text) return; s->pick(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "kind"))); })), this);
	}
	bookmark_rows_ = gtk_list_box_new();
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(bookmark_rows_), GTK_SELECTION_SINGLE);
	auto *list_scroll = gtk_scrolled_window_new();
	gtk_widget_set_size_request(list_scroll, -1, 120);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroll), bookmark_rows_);
	gtk_box_append(GTK_BOX(notes_box_), list_scroll);
	g_signal_connect(bookmark_rows_, "row-selected", G_CALLBACK((+[](GtkListBox *, GtkListBoxRow *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (!s->updating_) s->select_bookmark(); })), this);
	const char *fields[] = {"Folder caption", "Color", "Module", "Key/reference", "Module description", "Description"};
	for (int i = 0; i < 6; ++i) {
		bar = row(notes_box_);
		bookmark_fields_[i] = entry(bar, fields[i]);
		bookmark_unset_[i] = gtk_check_button_new_with_label("Unset");
		gtk_box_append(GTK_BOX(bar), bookmark_unset_[i]);
	}
	bar = row(notes_box_);
	bookmark_parent_ = entry(bar, "Parent index (empty = root)");
	bookmark_position_ = gtk_spin_button_new_with_range(0, 100000, 1);
	gtk_box_append(GTK_BOX(bar), bookmark_position_);
	remove_confirm_ = gtk_check_button_new_with_label("Confirm bookmark removal");
	gtk_box_append(GTK_BOX(notes_box_), remove_confirm_);
	const char *actions[] = {"Add bookmark", "Add folder", "Update selected", "Move selected", "Remove selected", "Read selected"};
	bar = row(notes_box_);
	for (int i = 0; i < 6; ++i) {
		if (i == 2 || i == 4)
			bar = row(notes_box_);
		auto *w = gtk_button_new_with_label(actions[i]);
		gtk_box_append(GTK_BOX(bar), w);
		g_object_set_data(G_OBJECT(w), "action", GINT_TO_POINTER(i));
		g_signal_connect(w, "clicked", G_CALLBACK((+[](GtkButton *w, gpointer data) {
					 auto *s = static_cast<StudyUI *>(data);
					 if (!s->model_ || s->model_->busy())
						 return;
					 const auto action = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "action"));
					 auto *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(s->bookmark_rows_));
					 std::optional<size_t> target;
					 if (row)
						 target = gtk_list_box_row_get_index(row);
					 const auto &items = s->model_->bookmarks().items;
					 if (target && *target >= items.size())
						 return;
					 if (action == 5) {
						 if (target)
							 s->model_->open_bookmark(*target);
						 return;
					 }
					 if (action >= 2 && !target)
						 return;
					 if (action == 4 && !gtk_check_button_get_active(GTK_CHECK_BUTTON(s->remove_confirm_)))
						 return;
					 std::optional<size_t> parent;
					 const auto source = text(s->bookmark_parent_);
					 if (!source.empty()) {
						 char *end = nullptr;
						 auto value = g_ascii_strtoull(source.c_str(), &end, 10);
						 if (!end || *end || source.front() == '-')
							 return;
						 parent = value;
					 }
					 std::array<std::optional<std::string>, 6> fields;
					 for (size_t i = 0; i < fields.size(); ++i)
						 if (!gtk_check_button_get_active(GTK_CHECK_BUTTON(s->bookmark_unset_[i])))
							 fields[i] = text(s->bookmark_fields_[i]);
					 s->model_->edit_bookmark(action <= 1 ? XC_BOOKMARK_INSERT : action == 2 ? XC_BOOKMARK_UPDATE
												 : action == 3	 ? XC_BOOKMARK_MOVE
														 : XC_BOOKMARK_REMOVE,
								  action == 1 ? XC_BOOKMARK_FOLDER : target && action >= 2 ? items[*target].kind
															   : XC_BOOKMARK_LEAF,
								  action <= 1 ? std::nullopt : target, parent, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->bookmark_position_)), fields);
				 })),
				 this);
	}
	bar = row(notes_box_);
	auto *import = gtk_button_new_with_label("Review AndBible backup…");
	gtk_box_append(GTK_BOX(bar), import);
	g_signal_connect(import, "clicked", G_CALLBACK(+[](GtkButton *, gpointer p) { static_cast<StudyUI *>(p)->pick(15); }), this);
	auto *export_button = gtk_button_new_with_label("Export exact XML to new file…");
	gtk_box_append(GTK_BOX(bar), export_button);
	g_signal_connect(export_button, "clicked", G_CALLBACK(+[](GtkButton *, gpointer p) { static_cast<StudyUI *>(p)->pick(16); }), this);
	import_folder_ = entry(notes_box_, "Import folder caption");
	gtk_editable_set_text(GTK_EDITABLE(import_folder_), "Imported from AndBible");
	const char *policies[] = {"Reject collisions", "Skip identical", "Keep existing", nullptr};
	import_policy_ = gtk_drop_down_new_from_strings(policies);
	gtk_box_append(GTK_BOX(notes_box_), import_policy_);
	import_review_ = gtk_label_new("");
	gtk_label_set_wrap(GTK_LABEL(import_review_), TRUE);
	gtk_label_set_selectable(GTK_LABEL(import_review_), TRUE);
	gtk_box_append(GTK_BOX(notes_box_), import_review_);
	auto *merge = gtk_button_new_with_label("Import reviewed bookmarks");
	gtk_box_append(GTK_BOX(notes_box_), merge);
	g_signal_connect(merge, "clicked", G_CALLBACK(+[](GtkButton *, gpointer p) {auto*s=static_cast<StudyUI*>(p);if(s->model_)s->model_->merge_bookmarks(text(s->import_folder_),gtk_drop_down_get_selected(GTK_DROP_DOWN(s->import_policy_))); }), this);
	note_generic_ = gtk_check_button_new_with_label("Generic verse note (all modules)");
	gtk_box_append(GTK_BOX(notes_box_), note_generic_);
	auto *begin = gtk_button_new_with_label("Edit current verse note");
	gtk_box_append(GTK_BOX(notes_box_), begin);
	g_signal_connect(begin, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->model_ && text(s->note_text_) == s->model_->note_draft().text) s->model_->begin_note(gtk_check_button_get_active(GTK_CHECK_BUTTON(s->note_generic_))); })), this);
	note_label_ = gtk_label_new("");
	gtk_label_set_wrap(GTK_LABEL(note_label_), TRUE);
	gtk_box_append(GTK_BOX(notes_box_), note_label_);
	note_text_ = text_view(notes_box_, "Verse note text");
	auto *save = gtk_button_new_with_label("Save verse note");
	gtk_box_append(GTK_BOX(notes_box_), save);
	g_signal_connect(save, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (s->model_) s->model_->save_note(text(s->note_text_)); })), this);
	bar = row(notes_box_);
	auto *discard_note = gtk_button_new_with_label("Discard note changes");
	gtk_box_append(GTK_BOX(bar), discard_note);
	g_signal_connect(discard_note, "clicked", G_CALLBACK(+[](GtkButton *, gpointer p) {auto*s=static_cast<StudyUI*>(p);if(s->model_)set_text(s->note_text_,s->model_->note_draft().text); }), this);
	note_remove_ = gtk_check_button_new_with_label("Confirm note removal");
	gtk_box_append(GTK_BOX(notes_box_), note_remove_);
	auto *delete_note = gtk_button_new_with_label("Remove first matching note");
	gtk_box_append(GTK_BOX(notes_box_), delete_note);
	g_signal_connect(delete_note, "clicked", G_CALLBACK(+[](GtkButton *, gpointer p) {auto*s=static_cast<StudyUI*>(p);if(s->model_&&gtk_check_button_get_active(GTK_CHECK_BUTTON(s->note_remove_))){s->model_->save_note("",true);gtk_check_button_set_active(GTK_CHECK_BUTTON(s->note_remove_),FALSE);} }), this);
	repository_status_ = gtk_label_new("");
	gtk_label_set_wrap(GTK_LABEL(repository_status_), TRUE);
	gtk_box_append(GTK_BOX(notes_box_), repository_status_);
	return scroll;
}
void StudyUI::select_bookmark()
{
	if (!model_)
		return;
	auto *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(bookmark_rows_));
	if (!row)
		return;
	const auto i = gtk_list_box_row_get_index(row);
	const auto &items = model_->bookmarks().items;
	if (i < 0 || size_t(i) >= items.size())
		return;
	const auto &item = items[i];
	for (size_t f = 0; f < 6; ++f) {
		gtk_check_button_set_active(GTK_CHECK_BUTTON(bookmark_unset_[f]), !item.fields[f]);
		gtk_editable_set_text(GTK_EDITABLE(bookmark_fields_[f]), item.fields[f].value_or("").c_str());
	}
	gtk_editable_set_text(GTK_EDITABLE(bookmark_parent_), item.parent ? std::to_string(*item.parent).c_str() : "");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(bookmark_position_), item.position);
}
GtkWidget *StudyUI::editor_pane()
{
	editor_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	auto *bar = row(editor_box_);
	const char *names[] = {"Open file…", "New file…", "Open module entry", "New book section", "Save", "Undo", "Redo", "Preview"};
	for (int i = 0; i < 8; ++i) {
		if (i == 2 || i == 4)
			bar = row(editor_box_);
		auto *w = gtk_button_new_with_label(names[i]);
		gtk_box_append(GTK_BOX(bar), w);
		g_object_set_data(G_OBJECT(w), "action", GINT_TO_POINTER(i));
		g_signal_connect(w, "clicked", G_CALLBACK((+[](GtkButton *w, gpointer data) {
					 auto *s = static_cast<StudyUI *>(data);
					 if (!s->model_ || s->model_->busy())
						 return;
					 const auto action = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "action"));
					 if (action < 2) {
						 if (!s->model_->editor_dirty())
							 s->pick(action == 0 ? 13 : 14);
					 } else if (action < 4)
						 s->model_->open_module_editor(text(s->editor_module_), text(s->editor_reference_), action == 3);
					 else if (action == 4) {
						 if (s->model_->change_editor(text(s->editor_text_)))
							 s->model_->save_editor();
					 } else if (action == 5 || action == 6)
						 s->model_->editor_undo(action == 6);
					 else {
						 if (!s->editor_preview_) {
							 gtk_box_append(GTK_BOX(s->editor_preview_box_), s->view(&s->editor_preview_, false));
							 g_object_set_data(G_OBJECT(s->editor_preview_), "parallel", GINT_TO_POINTER(1));
						 }
						 webkit_web_view_load_html(s->editor_preview_, document(s->model_->editor().text).c_str(), "xiphos-preview://study/");
					 }
				 })),
				 this);
	}
	bar = row(editor_box_);
	auto *save_as = gtk_button_new_with_label("Save as new file…");
	gtk_box_append(GTK_BOX(bar), save_as);
	g_signal_connect(save_as, "clicked", G_CALLBACK(+[](GtkButton *, gpointer p) {auto*s=static_cast<StudyUI*>(p);if(s->model_&&s->model_->change_editor(text(s->editor_text_)))s->pick(17); }), this);
	auto *discard = gtk_button_new_with_label("Discard editor changes");
	gtk_box_append(GTK_BOX(bar), discard);
	g_signal_connect(discard, "clicked", G_CALLBACK(+[](GtkButton *, gpointer p) {auto*s=static_cast<StudyUI*>(p);if(s->model_)s->model_->discard_editor(); }), this);
	editor_module_ = entry(editor_box_, "Editable module ID");
	editor_reference_ = entry(editor_box_, "Exact verse or book path");
	editor_label_ = gtk_label_new("");
	gtk_label_set_wrap(GTK_LABEL(editor_label_), TRUE);
	gtk_box_append(GTK_BOX(editor_box_), editor_label_);
	editor_stack_ = gtk_stack_new();
	gtk_box_append(GTK_BOX(editor_box_), editor_stack_);
	gtk_widget_set_vexpand(editor_stack_, TRUE);
	auto *source_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	editor_text_ = text_view(source_box, "Editor source text");
	gtk_stack_add_named(GTK_STACK(editor_stack_), source_box, "source");
	rich_editor_ = std::make_unique<RichEditor>([this](const std::string &html) {
		if (!model_ || closed_ || !model_->change_editor(html))
			return false;
		const bool was_updating = updating_;
		updating_ = true;
		set_text(editor_text_, html);
		updating_ = was_updating;
		return true;
	});
	auto *rich_scroll = gtk_scrolled_window_new();
	gtk_widget_set_size_request(rich_scroll, -1, 130);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(rich_scroll), rich_editor_->widget());
	gtk_stack_add_named(GTK_STACK(editor_stack_), rich_scroll, "rich");
	auto *format_bar = row(editor_box_);
	const char *formats[] = {"Source", "Rich", "Bold", "Italic", "Underline", "Strike", "Text color", "Highlight", "Link"};
	for (int i = 0; i < 9; ++i) {
		if (i == 4 || i == 6)
			format_bar = row(editor_box_);
		auto *w = gtk_button_new_with_label(formats[i]);
		gtk_box_append(GTK_BOX(format_bar), w);
		g_object_set_data(G_OBJECT(w), "format", GINT_TO_POINTER(i));
		g_signal_connect(w, "clicked", G_CALLBACK((+[](GtkButton *w, gpointer data) {
					 auto *s = static_cast<StudyUI *>(data);
					 if (!s->model_ || s->model_->busy())
						 return;
					 int action = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "format"));
					 if (action == 0) {
						 s->rich_mode_ = false;
						 gtk_stack_set_visible_child_name(GTK_STACK(s->editor_stack_), "source");
						 return;
					 }
					 if (action == 1) {
						 if (s->rich_editor_->load(s->model_->editor().text)) {
							 s->rich_mode_ = true;
							 gtk_stack_set_visible_child_name(GTK_STACK(s->editor_stack_), "rich");
						 } else
							 gtk_label_set_text(GTK_LABEL(s->editor_label_), "This markup needs Source mode; the original text is retained.");
						 return;
					 }
					 if (!s->rich_mode_)
						 return;
					 auto selected = s->rich_editor_->selection();
					 const int commands[] = {XC_RICH_BOLD, XC_RICH_ITALIC, XC_RICH_UNDERLINE, XC_RICH_STRIKE, XC_RICH_FOREGROUND, XC_RICH_BACKGROUND, XC_RICH_LINK};
					 s->model_->format_editor(selected.first, selected.second, commands[action - 2], -1, action >= 6 ? text(s->format_value_) : "");
				 })),
				 this);
	}
	const char *blocks[] = {"Normal", "Heading 1", "Heading 2", "Heading 3", "Heading 4", "Heading 5", "Heading 6", "Address", "Preformatted", "Bullets", "Numbers", "Roman numerals", "Letters", nullptr};
	auto *block = gtk_drop_down_new_from_strings(blocks);
	gtk_box_append(GTK_BOX(editor_box_), block);
	g_signal_connect(block, "notify::selected", G_CALLBACK(+[](GObject *w, GParamSpec *, gpointer p) {auto*s=static_cast<StudyUI*>(p);if(!s->model_||!s->rich_mode_||s->updating_)return;auto range=s->rich_editor_->selection();s->model_->format_editor(range.first,range.second,XC_RICH_BLOCK,gtk_drop_down_get_selected(GTK_DROP_DOWN(w))); }), this);
	format_value_ = entry(editor_box_, "Color or internal link (empty clears)");
	g_signal_connect(gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor_text_)), "changed", G_CALLBACK((+[](GtkTextBuffer *, gpointer data) { auto *s = static_cast<StudyUI *>(data); if (!s->updating_ && s->model_ && !s->model_->busy()) s->model_->change_editor(text(s->editor_text_)); })), this);
	editor_preview_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_append(GTK_BOX(editor_box_), editor_preview_box_);
	auto *organize = gtk_expander_new("Organize saved book section");
	gtk_box_append(GTK_BOX(editor_box_), organize);
	auto *commands = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_expander_set_child(GTK_EXPANDER(organize), commands);
	auto *destination = entry(commands, "New section name or destination parent path");
	const char *operations[] = {"Rename section", "Rename subtree", "Move subtree", "Delete section", "Delete subtree", nullptr};
	auto *operation = gtk_drop_down_new_from_strings(operations);
	gtk_accessible_update_property(GTK_ACCESSIBLE(operation), GTK_ACCESSIBLE_PROPERTY_LABEL, "Book organization operation", -1);
	gtk_box_append(GTK_BOX(commands), operation);
	auto *confirm = gtk_check_button_new_with_label("Confirm change to the open book section");
	gtk_box_append(GTK_BOX(commands), confirm);
	auto *apply = gtk_button_new_with_label("Apply to saved section");
	gtk_box_append(GTK_BOX(commands), apply);
	g_object_set_data(G_OBJECT(apply), "destination", destination);
	g_object_set_data(G_OBJECT(apply), "operation", operation);
	g_object_set_data(G_OBJECT(apply), "confirm", confirm);
	g_signal_connect(apply, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
		auto *ui = static_cast<StudyUI *>(data);
		auto *confirm = GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "confirm"));
		if (!ui->model_ || !gtk_check_button_get_active(confirm))
			return;
		auto *operation = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "operation"));
		auto *destination = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "destination"));
		if (ui->model_->organize_book_entry(gtk_drop_down_get_selected(operation), text(destination)))
			gtk_check_button_set_active(confirm, FALSE);
	}), this);
	return editor_box_;
}
void StudyUI::update_repositories()
{
	const bool enabled = model_->ready() && !model_->busy();
	if (notes_box_) {
		gtk_widget_set_sensitive(notes_box_, enabled);
		const auto &incoming = model_->bookmark_import();
		const auto &counts = incoming.review;
		const auto &source = incoming.source;
		std::string review = incoming.ready ? ("Imported " + std::to_string(source.imported) + ", skipped " + std::to_string(source.skipped) + ", unknown versification " + std::to_string(source.unknown_versification) + ", invalid ordinal " + std::to_string(source.invalid_ordinal) + ", invalid identity " + std::to_string(source.invalid_identity) + "\nNew " + std::to_string(counts.new_items) + ", identical " + std::to_string(counts.identical) + ", conflicting " + std::to_string(counts.conflicting) + ", ambiguous " + std::to_string(counts.ambiguous) + ", unidentified " + std::to_string(counts.unidentified)) : "";
		for (const auto &collision : incoming.collisions)
			review += "\n" + collision;
		gtk_label_set_text(GTK_LABEL(import_review_), review.c_str());
		const auto &marks = model_->bookmarks();
		if (marks.version != bookmarks_version_) {
			bookmarks_version_ = marks.version;
			while (auto *w = gtk_widget_get_first_child(bookmark_rows_))
				gtk_list_box_remove(GTK_LIST_BOX(bookmark_rows_), w);
			for (size_t i = 0; i < marks.items.size(); ++i) {
				const auto &item = marks.items[i];
				auto *l = gtk_label_new((std::to_string(i) + ": " + item.fields[XC_BOOKMARK_LABEL].value_or(item.fields[XC_BOOKMARK_CAPTION].value_or("(Unlabelled)"))).c_str());
				gtk_label_set_xalign(GTK_LABEL(l), 0);
				gtk_list_box_append(GTK_LIST_BOX(bookmark_rows_), l);
			}
			gtk_check_button_set_active(GTK_CHECK_BUTTON(remove_confirm_), FALSE);
		}
		if (model_->note_draft().version != note_version_) {
			note_version_ = model_->note_draft().version;
			set_text(note_text_, model_->note_draft().text);
			gtk_label_set_text(GTK_LABEL(note_label_), (model_->note_draft().module + " " + model_->note_draft().reference + "\n" + model_->note_draft().path).c_str());
		}
		gtk_label_set_text(GTK_LABEL(repository_status_), model_->error().empty() ? marks.path.c_str() : model_->error().c_str());
	}
	if (editor_box_) {
		gtk_widget_set_sensitive(editor_box_, enabled);
		const auto &e = model_->editor();
		if (e.version != editor_version_) {
			editor_version_ = e.version;
			if (text(editor_text_) != e.text)
				set_text(editor_text_, e.text);
			if (rich_mode_ && !rich_editor_->load(e.text)) {
				rich_mode_ = false;
				gtk_stack_set_visible_child_name(GTK_STACK(editor_stack_), "source");
			}
		} else if (enabled && text(editor_text_) != e.text)
			model_->change_editor(text(editor_text_));
		gtk_label_set_text(GTK_LABEL(editor_label_), (e.module.empty() ? e.path : e.module + " " + e.reference).c_str());
	}
	if (enabled && (model_->bookmarks().revision || model_->annotations().revision)) {
		const auto parallel_key = std::to_string(model_->parallel().selection_revision) + model_->bookmarks().revision.value_or("") + model_->annotations().revision.value_or("");
		if (!model_->parallel().columns.empty() && parallel_key != decoration_identity_[2] && model_->decorate_parallel()) {
			decoration_identity_[2] = parallel_key;
			return;
		}
		for (int i = 0; i < 2; ++i) {
			const auto &p = model_->passage();
			const auto module = i ? p.commentary : p.bible;
			if (module.empty())
				continue;
			const auto identity = module + p.reference + (i ? model_->commentary_chapter().html : model_->bible_chapter().html) + p.bible_html + p.commentary_html + std::to_string(model_->show_chapter()) + std::to_string(model_->show_commentary_chapter()) + model_->bookmarks().revision.value_or("") + model_->annotations().revision.value_or("");
			if (identity != decoration_identity_[i] && model_->decorate_reading(i != 0)) {
				decoration_identity_[i] = identity;
				break;
			}
		}
	}
}
bool StudyUI::confirm_close()
{
	if (closed_ || discard_on_close_ || !model_)
		return true;
	const bool dirty = model_->editor_dirty() || (editor_text_ && text(editor_text_) != model_->editor().text) || (note_text_ && text(note_text_) != model_->note_draft().text);
	if (!dirty)
		return true;
	if (close_dialog_)
		return false;
	close_dialog_ = gtk_message_dialog_new(parent_, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, "%s", "Discard unsaved editor and verse-note changes?");
	gtk_dialog_add_buttons(GTK_DIALOG(close_dialog_), "Cancel", GTK_RESPONSE_CANCEL, "Discard", GTK_RESPONSE_ACCEPT, nullptr);
	gtk_dialog_set_default_response(GTK_DIALOG(close_dialog_), GTK_RESPONSE_CANCEL);
	g_signal_connect(close_dialog_, "response", G_CALLBACK((+[](GtkDialog *dialog, int response, gpointer data) {
				 auto *s = static_cast<StudyUI *>(data);
				 s->close_dialog_ = nullptr;
				 gtk_window_destroy(GTK_WINDOW(dialog));
				 if (s->on_close_decision)
					 s->on_close_decision(response == GTK_RESPONSE_ACCEPT);
				 if (response == GTK_RESPONSE_ACCEPT && !s->closed_) {
					 s->discard_on_close_ = true;
					 gtk_window_close(s->parent_);
				 }
			 })),
			 this);
	gtk_window_present(GTK_WINDOW(close_dialog_));
	return false;
}
namespace
{
std::pair<size_t, size_t> source_selection(GtkWidget *view)
{
	auto *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GtkTextIter first, last, begin;
	if (!gtk_text_buffer_get_selection_bounds(buffer, &first, &last)) {
		gtk_text_buffer_get_iter_at_mark(buffer, &first, gtk_text_buffer_get_insert(buffer));
		last = first;
	}
	gtk_text_buffer_get_start_iter(buffer, &begin);
	auto *prefix = gtk_text_buffer_get_text(buffer, &begin, &first, FALSE);
	auto *selected = gtk_text_buffer_get_text(buffer, &first, &last, FALSE);
	const size_t start = std::char_traits<char>::length(prefix), end = start + std::char_traits<char>::length(selected);
	g_free(prefix);
	g_free(selected);
	return {start, end};
}
void restore_selection(GtkWidget *view, const RecoveryDraft &draft)
{
	if (draft.start > draft.end || draft.end > draft.text.size())
		return;
	const auto *bytes = draft.text.c_str();
	if (!g_utf8_validate(bytes, draft.start, nullptr) || !g_utf8_validate(bytes, draft.end, nullptr))
		return;
	auto *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GtkTextIter start, end;
	gtk_text_buffer_get_iter_at_offset(buffer, &start, g_utf8_strlen(bytes, draft.start));
	gtk_text_buffer_get_iter_at_offset(buffer, &end, g_utf8_strlen(bytes, draft.end));
	gtk_text_buffer_select_range(buffer, &start, &end);
}
} // namespace
std::vector<RecoveryDraft> StudyUI::recovery_drafts(const std::string &key) const
{
	std::vector<RecoveryDraft> drafts;
	if (!model_ || closed_)
		return drafts;
	if (editor_text_ && model_->editor().version) {
		const auto &editor = model_->editor();
		RecoveryDraft d;
		d.id = key + ":editor";
		d.title = "Editor draft";
		d.target = !editor.module.empty() ? 2 : editor.path.empty() ? 0
									    : 1;
		d.path = editor.path;
		d.module = editor.module;
		d.reference = editor.reference;
		d.revision = editor.revision;
		d.saved = model_->editor_saved_text();
		d.text = text(editor_text_);
		if (!rich_mode_) {
			const auto range = source_selection(editor_text_);
			d.start = range.first;
			d.end = range.second;
		}
		drafts.push_back(std::move(d));
	}
	if (note_text_ && text(note_text_) != model_->note_draft().text) {
		const auto &note = model_->note_draft();
		RecoveryDraft d;
		d.id = key + ":note";
		d.title = "Verse note draft";
		d.target = note.label.empty() ? 0 : 3;
		d.path = note.path;
		d.module = note.module;
		d.reference = note.reference;
		d.label = note.label;
		d.generic = note.generic;
		d.revision = note.revision;
		d.saved = note.text;
		d.text = text(note_text_);
		const auto range = source_selection(note_text_);
		d.start = range.first;
		d.end = range.second;
		drafts.push_back(std::move(d));
	}
	return drafts;
}
bool StudyUI::recover_draft(const RecoveryDraft &draft)
{
	if (!model_ || !model_->recover_draft(draft))
		return false;
	const bool was_updating = updating_;
	updating_ = true;
	update_repositories();
	if (draft.target == 3) {
		set_text(note_text_, draft.text);
		restore_selection(note_text_, draft);
	} else {
		rich_mode_ = false;
		gtk_stack_set_visible_child_name(GTK_STACK(editor_stack_), "source");
		set_text(editor_text_, draft.text);
		restore_selection(editor_text_, draft);
	}
	updating_ = was_updating;
	return true;
}

} // namespace xiphos::linux_preview
