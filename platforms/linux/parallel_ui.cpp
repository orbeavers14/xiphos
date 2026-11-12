// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_document.hpp"
#include "study_ui.hpp"
#include <algorithm>
namespace xiphos::linux_preview
{
namespace
{
std::string escape(const std::string &s)
{
	auto *p = g_markup_escape_text(s.c_str(), s.size());
	std::string v(p);
	g_free(p);
	return v;
}
std::string javascript_string(const std::string &s)
{
	std::string out = "\"";
	const char hex[] = "0123456789abcdef";
	for (unsigned char c : s) {
		if (c == '"' || c == '\\') {
			out += '\\';
			out += char(c);
		} else if (c < 32 || c == '<' || c == '>' || c == '&') {
			out += "\\u00";
			out += hex[c >> 4];
			out += hex[c & 15];
		} else
			out += char(c);
	}
	return out + '"';
}
GtkWidget *row(GtkWidget *parent)
{
	auto *w = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_append(GTK_BOX(parent), w);
	return w;
}
GtkWidget *field(GtkWidget *parent, const char *name)
{
	auto *w = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(w), name);
	gtk_accessible_update_property(GTK_ACCESSIBLE(w), GTK_ACCESSIBLE_PROPERTY_LABEL, name, -1);
	gtk_widget_set_hexpand(w, TRUE);
	gtk_box_append(GTK_BOX(parent), w);
	return w;
}
std::string text(GtkWidget *w) { return gtk_editable_get_text(GTK_EDITABLE(w)); }
} // namespace
GtkWidget *StudyUI::parallel_pane()
{
	parallel_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	parallel_modules_ = field(parallel_box_, "Ordered parallel modules (comma-separated)");
	auto *bar = row(parallel_box_);
	parallel_reference_ = field(bar, "Parallel passage");
	const char *names[] = {"Read", "Previous chapter", "Next chapter", "Open sets…", "New sets…", "Read selected set", "Create set", "Replace selected", "Rename selected", "Remove selected"};
	for (int i = 0; i < 10; ++i) {
		if (i == 3 || i == 6 || i == 8)
			bar = row(parallel_box_);
		auto *w = gtk_button_new_with_label(names[i]);
		gtk_box_append(GTK_BOX(bar), w);
		g_object_set_data(G_OBJECT(w), "command", GINT_TO_POINTER(i));
		g_signal_connect(w, "clicked", G_CALLBACK((+[](GtkButton *w, gpointer data) {
					 auto *s = static_cast<StudyUI *>(data);
					 if (!s->model_ || s->closed_ || s->model_->busy())
						 return;
					 const auto action = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "command"));
					 if (action == 0 || action == 6 || action == 7) {
						 if (action == 7 && !gtk_check_button_get_active(GTK_CHECK_BUTTON(s->parallel_confirm_)))
							 return;
						 if (s->model_->parse_search_modules(text(s->parallel_modules_)))
							 g_object_set_data(G_OBJECT(s->parallel_modules_), "pending-action", GINT_TO_POINTER(action == 0 ? 1 : action == 6 ? 2
																					   : 3));
					 } else if (action == 1 || action == 2)
						 s->model_->navigate_parallel(action == 1 ? -1 : 1);
					 else if (action == 3 || action == 4)
						 s->pick(action == 3 ? 7 : 8);
					 else {
						 const auto i = gtk_drop_down_get_selected(GTK_DROP_DOWN(s->parallel_sets_));
						 const auto &sets = s->model_->parallel_profile().sets;
						 if (i >= sets.size())
							 return;
						 if (action == 5)
							 s->model_->open_parallel_set(sets[i].key, text(s->parallel_reference_));
						 else if (action == 8)
							 s->model_->edit_parallel_set(XC_PARALLEL_PROFILE_RENAME, sets[i].key, text(s->parallel_name_), {});
						 else if (gtk_check_button_get_active(GTK_CHECK_BUTTON(s->parallel_confirm_)))
							 s->model_->edit_parallel_set(XC_PARALLEL_PROFILE_REMOVE, sets[i].key, "", {});
					 }
				 })),
				 this);
	}
	parallel_sets_ = gtk_drop_down_new(nullptr, nullptr);
	gtk_box_append(GTK_BOX(parallel_box_), parallel_sets_);
	gtk_accessible_update_property(GTK_ACCESSIBLE(parallel_sets_), GTK_ACCESSIBLE_PROPERTY_LABEL, "Saved parallel reading set", -1);
	parallel_name_ = field(parallel_box_, "New or renamed set name");
	parallel_default_ = gtk_check_button_new_with_label("Make saved set the default");
	gtk_box_append(GTK_BOX(parallel_box_), parallel_default_);
	parallel_confirm_ = gtk_check_button_new_with_label("Confirm replacement or removal of selected set");
	gtk_box_append(GTK_BOX(parallel_box_), parallel_confirm_);
	auto *layout_bar = row(parallel_box_);
	const char *layouts[] = {"Chapter columns", "Stacked verse", nullptr};
	parallel_layout_ = gtk_drop_down_new_from_strings(layouts);
	gtk_accessible_update_property(GTK_ACCESSIBLE(parallel_layout_), GTK_ACCESSIBLE_PROPERTY_LABEL, "Comparison layout", -1);
	gtk_box_append(GTK_BOX(layout_bar), parallel_layout_);
	g_signal_connect(parallel_layout_, "notify::selected", G_CALLBACK(+[](GObject *, GParamSpec *, gpointer data) {
		static_cast<StudyUI *>(data)->parallel_needs_render_ = true;
	}), this);
	parallel_previous_verse_ = gtk_button_new_with_label("Previous verse");
	parallel_next_verse_ = gtk_button_new_with_label("Next verse");
	for (auto *button : {parallel_previous_verse_, parallel_next_verse_}) {
		gtk_box_append(GTK_BOX(layout_bar), button);
		g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
			auto *ui = static_cast<StudyUI *>(data);
			ui->step_parallel_verse(GTK_WIDGET(button) == ui->parallel_previous_verse_ ? -1 : 1);
		}), this);
	}
	auto *word_button = gtk_button_new_with_label("Study selected word");
	gtk_box_append(GTK_BOX(layout_bar), word_button);
	g_signal_connect(word_button, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
		static_cast<StudyUI *>(data)->study_parallel_selection();
	}), this);
	auto *translations = gtk_menu_button_new();
	gtk_menu_button_set_label(GTK_MENU_BUTTON(translations), "Translations");
	auto *visibility = gtk_popover_new();
	parallel_visibility_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_popover_set_child(GTK_POPOVER(visibility), parallel_visibility_);
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(translations), visibility);
	gtk_box_append(GTK_BOX(layout_bar), translations);
	gtk_box_append(GTK_BOX(parallel_box_), view(&parallel_view_, false));
	// The isolated host world's click listeners need script execution. Keep
	// page-authored scripts disabled here and through the document's CSP.
	auto *settings = webkit_web_view_get_settings(parallel_view_);
	webkit_settings_set_enable_javascript_markup(settings, FALSE);
	webkit_settings_set_enable_javascript(settings, TRUE);
	g_object_set_data(G_OBJECT(parallel_view_), "parallel", GINT_TO_POINTER(1));
	auto *manager = webkit_web_view_get_user_content_manager(parallel_view_);
	webkit_user_content_manager_register_script_message_handler(manager, "xiphosParallel", "xiphos-host");
	g_signal_connect(manager, "script-message-received::xiphosParallel", G_CALLBACK((+[](WebKitUserContentManager *, JSCValue *value, gpointer data) {
				 auto *s = static_cast<StudyUI *>(data);
				 if (s->closed_ || !s->model_ || s->model_->busy() || !jsc_value_is_object(value))
					 return;
				 auto *r = jsc_value_object_get_property(value, "row"), *c = jsc_value_object_get_property(value, "column"), *v = jsc_value_object_get_property(value, "revision"), *u = jsc_value_object_get_property(value, "uri");
				 auto *word = jsc_value_object_get_property(value, "word");
				 auto *part_value = jsc_value_object_get_property(value, "part");
				 const auto &p = s->model_->parallel();
				 const int row = jsc_value_to_int32(r), col = jsc_value_to_int32(c);
				 if (jsc_value_is_number(r) && jsc_value_is_number(c) && jsc_value_is_number(v) && (jsc_value_is_string(u) || jsc_value_is_string(word)) && jsc_value_to_int32(v) == int(p.revision) && row >= 0 && col >= 0 && size_t(row) < p.rows.size() && size_t(col) < p.columns.size()) {
					 const auto &module = p.columns[col].module;
					 const auto &cell = p.rows[row].cells[col];
					 const int part = jsc_value_to_int32(part_value);
					 const auto &reference = part > 0 && size_t(part) <= cell.additional.size()
						 ? cell.additional[part - 1].first : cell.reference;
					 const bool studying = jsc_value_is_string(word);
					 auto *argument = jsc_value_to_string(studying ? word : u);
					 if (studying)
						 s->model_->word_study(module, reference, argument ? argument : "");
					 else
						 s->model_->follow(module, argument ? argument : "", reference);
					 g_free(argument);
				 }
				 g_object_unref(r);
				 g_object_unref(c);
				 g_object_unref(v);
				 g_object_unref(u);
				 g_object_unref(word);
				 g_object_unref(part_value);
			 })),
			 this);
	g_signal_connect(parallel_view_, "load-changed", G_CALLBACK((+[](WebKitWebView *view, WebKitLoadEvent event, gpointer) {
				 if (event != WEBKIT_LOAD_FINISHED)
					 return;
				 auto *script = static_cast<const char *>(g_object_get_data(G_OBJECT(view), "parallel-populate"));
				 if (script)
                     webkit_web_view_evaluate_javascript(view, script, -1, "xiphos-host", nullptr, nullptr,
                       +[](GObject *source, GAsyncResult *result, gpointer) {
                         GError *error=nullptr;auto *value=webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(source),result,&error);
                         if(error)g_warning("Parallel presentation: %s",error->message);
                         if(value&&jsc_value_is_number(value))g_object_set_data(source,"parallel-ready",GINT_TO_POINTER(jsc_value_to_int32(value)));
                         g_clear_object(&value);g_clear_error(&error);
                       }, nullptr);
			 })),
			 nullptr);
	return parallel_box_;
}
void StudyUI::study_parallel_selection()
{
	if (!model_ || model_->busy() || !parallel_view_ || webkit_web_view_is_loading(parallel_view_))
		return;
	webkit_web_view_evaluate_javascript(parallel_view_, "globalThis.xiphosStudySelection?.()", -1,
		"xiphos-host", nullptr, nullptr, nullptr, nullptr);
}
void StudyUI::step_parallel_verse(int direction)
{
	if (!model_ || model_->busy())
		return;
	const auto &p = model_->parallel();
	auto selected = std::find_if(p.rows.begin(), p.rows.end(), [&](const auto &item) { return item.ordinal == p.selected_verse; });
	if (selected == p.rows.end() || (direction < 0 ? selected == p.rows.begin() : std::next(selected) == p.rows.end()))
		return;
	model_->visit_parallel(parallel_order_, (selected + direction)->reference);
}

void StudyUI::update_parallel()
{
	const auto &p = model_->parallel();
	gtk_widget_set_sensitive(parallel_box_, model_->ready() && !model_->busy());
	const auto selected = std::find_if(p.rows.begin(), p.rows.end(), [&](const auto &item) { return item.ordinal == p.selected_verse; });
	gtk_widget_set_sensitive(parallel_previous_verse_, selected != p.rows.end() && selected != p.rows.begin());
	gtk_widget_set_sensitive(parallel_next_verse_, selected != p.rows.end() && std::next(selected) != p.rows.end());
	if (p.revision != parallel_revision_ || parallel_needs_render_) {
		const bool new_passage = p.revision != parallel_revision_;
		parallel_revision_ = p.revision;
		parallel_needs_render_ = false;
		std::vector<std::string> order;
		for (const auto &column : p.columns)
			order.push_back(column.requested);
		if (order != parallel_order_) {
			parallel_order_ = order;
			while (auto *child = gtk_widget_get_first_child(parallel_visibility_))
				gtk_box_remove(GTK_BOX(parallel_visibility_), child);
			const auto hidden = parallel_hidden_.find(order);
			for (size_t c = 0; c < order.size(); ++c) {
				auto *toggle = gtk_check_button_new_with_label(order[c].c_str());
				gtk_check_button_set_active(GTK_CHECK_BUTTON(toggle), hidden == parallel_hidden_.end() || !hidden->second[c]);
				gtk_box_append(GTK_BOX(parallel_visibility_), toggle);
				g_signal_connect(toggle, "toggled", G_CALLBACK(+[](GtkCheckButton *, gpointer data) {
					auto *ui = static_cast<StudyUI *>(data);
					std::vector<bool> hidden;
					for (auto *child = gtk_widget_get_first_child(ui->parallel_visibility_); child; child = gtk_widget_get_next_sibling(child))
						hidden.push_back(!gtk_check_button_get_active(GTK_CHECK_BUTTON(child)));
					ui->parallel_hidden_[ui->parallel_order_] = std::move(hidden);
					ui->parallel_needs_render_ = true;
				}), this);
			}
		}
		const auto hidden = parallel_hidden_.find(order);
		const auto visible = [&](size_t c) { return hidden == parallel_hidden_.end() || !hidden->second[c]; };
		const bool stacked = gtk_drop_down_get_selected(GTK_DROP_DOWN(parallel_layout_)) == 1;
		const auto title = [&](size_t c) {
			return escape(p.columns[c].requested) + (p.columns[c].status == XC_PARALLEL_READY ? "" : " (unavailable)");
		};
		struct Cell { size_t row, column, part; };
		std::vector<Cell> cells;
		std::string html;
		if (!stacked) {
			html = "<table><thead><tr><th scope='col'>Passage</th>";
			for (size_t c = 0; c < order.size(); ++c)
				if (visible(c))
					html += "<th scope='col'>" + title(c) + "</th>";
			html += "</tr></thead><tbody>";
		}
		for (size_t r = 0; r < p.rows.size(); ++r) {
			const auto &verse = p.rows[r];
			if (stacked && verse.ordinal != p.selected_verse)
				continue;
			if (stacked)
				html += "<h2>" + escape(verse.reference) + "</h2>";
			else
				html += "<tr><th scope='row'>" + escape(verse.reference) + "</th>";
			for (size_t c = 0; c < order.size(); ++c) {
				if (!visible(c))
					continue;
				const std::string direction = p.columns[c].rtl ? "rtl" : "ltr";
				if (stacked)
					html += "<section><h3>" + title(c) + "</h3><div dir='" + direction + "'>";
				else
					html += "<td dir='" + direction + "'>";
				const auto &cell = verse.cells[c];
				for (size_t part = 0; part <= cell.additional.size(); ++part) {
					html += "<div data-xiphos-cell class='xc-comparison-" + std::to_string(c) + "'></div>";
					cells.push_back({r, c, part});
				}
				html += stacked ? "</div></section>" : "</td>";
			}
			if (!stacked)
				html += "</tr>";
		}
		if (!stacked)
			html += "</tbody></table>";
		if (cells.empty() && !p.rows.empty())
			html += "<p>No translations shown. Use Translations to show a column.</p>";
		if (p.annotations_incomplete || p.invalid_colors)
			html += "<p>Some annotations could not be displayed. Invalid colors: " + std::to_string(p.invalid_colors) + ".</p>";
		std::string css = "table{border-collapse:collapse}td,th{min-width:170px;border:1px solid #888;padding:6px;vertical-align:top}";
		for (size_t c = 0; c < order.size(); ++c)
			css += reading_style_css(p.columns[c].module, ".xc-comparison-" + std::to_string(c));
		auto page = document(html, css);
		const auto at = page.find("<body>");
		if (at != std::string::npos)
			page.replace(at, 6, "<body data-revision='" + std::to_string(p.revision) + "'>");
		// Capture the host cell nodes before inserting any module markup.
		std::string script = "(()=>{const revision=" + std::to_string(p.revision) + ";if(Number(document.body.dataset.revision)!==revision)return;const cells=Array.from(document.querySelectorAll('[data-xiphos-cell]'));";
		for (size_t i = 0; i < cells.size(); ++i) {
			const auto [r, c, part] = cells[i];
			const auto &cell = p.rows[r].cells[c];
			const auto &body = part == 0 ? cell.html : cell.additional[part - 1].second;
			script += "{const cell=cells[" + std::to_string(i) + "];cell.innerHTML=" + javascript_string(body) +
				";cell.addEventListener('click',e=>{const a=e.target.closest('a[href]');if(!a||!cell.contains(a))return;e.preventDefault();e.stopPropagation();window.webkit.messageHandlers.xiphosParallel.postMessage({row:" +
				std::to_string(r) + ",column:" + std::to_string(c) + ",part:" + std::to_string(part) + ",revision,uri:a.getAttribute('href')});},true);}";
		}
		script += "const sources=[";
		for (const auto &cell : cells)
			script += "[" + std::to_string(cell.row) + "," + std::to_string(cell.column) + "," + std::to_string(cell.part) + "],";
		script += "];globalThis.xiphosStudySelection=()=>{const selection=getSelection();"
			"if(!selection||selection.isCollapsed)return;const word=selection.toString().trim();"
			"if(!word||word.length>256||/\\s/u.test(word))return;"
			"const index=cells.findIndex(cell=>cell.contains(selection.anchorNode)&&cell.contains(selection.focusNode));"
			"if(index<0)return;const [row,column,part]=sources[index];"
			"window.webkit.messageHandlers.xiphosParallel.postMessage({row,column,part,revision,word});};return revision;})();";
		g_object_set_data_full(G_OBJECT(parallel_view_), "parallel-populate", g_strdup(script.c_str()), g_free);
		webkit_web_view_load_html(parallel_view_, page.c_str(), "xiphos-preview://study/");
		if (new_passage)
			gtk_editable_set_text(GTK_EDITABLE(parallel_reference_), p.reference.c_str());
	}
	const auto &profile = model_->parallel_profile();
	auto identity = profile.path + profile.revision.value_or("");
	if (identity != parallel_profile_identity_) {
		parallel_profile_identity_ = identity;
		auto *list = gtk_string_list_new(nullptr);
		for (const auto &set : profile.sets)
			gtk_string_list_append(list, set.name.c_str());
		gtk_drop_down_set_model(GTK_DROP_DOWN(parallel_sets_), G_LIST_MODEL(list));
		g_object_unref(list);
		gtk_check_button_set_active(GTK_CHECK_BUTTON(parallel_confirm_), FALSE);
	}
}
} // namespace xiphos::linux_preview
