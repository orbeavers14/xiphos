// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_document.hpp"
#include "study_ui.hpp"
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
	gtk_box_append(GTK_BOX(parallel_box_), view(&parallel_view_, false));
	g_object_set_data(G_OBJECT(parallel_view_), "parallel", GINT_TO_POINTER(1));
	auto *manager = webkit_web_view_get_user_content_manager(parallel_view_);
	webkit_user_content_manager_register_script_message_handler(manager, "xiphosParallel", "xiphos-host");
	g_signal_connect(manager, "script-message-received::xiphosParallel", G_CALLBACK((+[](WebKitUserContentManager *, JSCValue *value, gpointer data) {
				 auto *s = static_cast<StudyUI *>(data);
				 if (s->closed_ || !s->model_ || s->model_->busy() || !jsc_value_is_object(value))
					 return;
				 auto *r = jsc_value_object_get_property(value, "row"), *c = jsc_value_object_get_property(value, "column"), *v = jsc_value_object_get_property(value, "revision"), *u = jsc_value_object_get_property(value, "uri");
				 const auto &p = s->model_->parallel();
				 const int row = jsc_value_to_int32(r), col = jsc_value_to_int32(c);
				 if (jsc_value_is_number(r) && jsc_value_is_number(c) && jsc_value_is_number(v) && jsc_value_is_string(u) && jsc_value_to_int32(v) == int(p.revision) && row >= 0 && col >= 0 && size_t(row) < p.rows.size() && size_t(col) < p.columns.size()) {
					 auto *uri = jsc_value_to_string(u);
					 s->model_->follow(p.columns[col].module, uri ? uri : "", p.rows[row].cells[col].reference);
					 g_free(uri);
				 }
				 g_object_unref(r);
				 g_object_unref(c);
				 g_object_unref(v);
				 g_object_unref(u);
			 })),
			 this);
	g_signal_connect(parallel_view_, "load-changed", G_CALLBACK((+[](WebKitWebView *view, WebKitLoadEvent event, gpointer) {
				 if (event != WEBKIT_LOAD_FINISHED)
					 return;
				 auto *script = static_cast<const char *>(g_object_get_data(G_OBJECT(view), "parallel-populate"));
				 if (script)
					 webkit_web_view_evaluate_javascript(view, script, -1, "xiphos-host", nullptr, nullptr, nullptr, nullptr);
			 })),
			 nullptr);
	return parallel_box_;
}
void StudyUI::update_parallel()
{
	const auto &p = model_->parallel();
	gtk_widget_set_sensitive(parallel_box_, model_->ready() && !model_->busy());
	if (p.revision != parallel_revision_) {
		parallel_revision_ = p.revision;
		std::string html = "<table><thead><tr><th>Passage</th>";
		for (const auto &c : p.columns)
			html += "<th>" + escape(c.requested) + (c.status == XC_PARALLEL_READY ? "" : " (unavailable)") + "</th>";
		html += "</tr></thead><tbody>";
		for (size_t r = 0; r < p.rows.size(); ++r) {
			html += "<tr><th>" + escape(p.rows[r].reference) + "</th>";
			for (size_t c = 0; c < p.columns.size(); ++c)
				html += "<td data-xiphos-cell='1' data-row='" + std::to_string(r) + "' data-column='" + std::to_string(c) + "' dir='" + (p.columns[c].rtl ? "rtl" : "ltr") + "'>" + "</td>";
			html += "</tr>";
		}
		html += "</tbody></table>";
		if (p.annotations_incomplete || p.invalid_colors)
			html += "<p>Some annotations could not be displayed. Invalid colors: " + std::to_string(p.invalid_colors) + ".</p>";
		auto page = document(html, "table{border-collapse:collapse}td,th{min-width:170px;border:1px solid #888;padding:6px;vertical-align:top}");
		const auto at = page.find("<body>");
		if (at != std::string::npos)
			page.replace(at, 6, "<body data-revision='" + std::to_string(p.revision) + "'>");
		// Populate each cell through innerHTML only after constructing the host
		// table. Module markup cannot close or replace the host's outer cells.
		// The isolated-world listener captures host indices; module attributes do
		// not supply navigation context and module scripts remain disabled.
		std::string script = "(()=>{const revision=" + std::to_string(p.revision) + ";if(Number(document.body.dataset.revision)!==revision)return;const cells=Array.from(document.querySelector('body > table').tBodies[0].rows).map(r=>Array.from(r.cells).slice(1));";
		for (size_t r = 0; r < p.rows.size(); ++r)
			for (size_t c = 0; c < p.columns.size(); ++c) {
				script += "{const cell=cells[" + std::to_string(r) + "][" + std::to_string(c) + "];cell.innerHTML=" + javascript_string(p.rows[r].cells[c].html) + ";cell.addEventListener('click',e=>{const a=e.target.closest('a[href]');if(!a||!cell.contains(a))return;e.preventDefault();e.stopPropagation();window.webkit.messageHandlers.xiphosParallel.postMessage({row:" + std::to_string(r) + ",column:" + std::to_string(c) + ",revision,uri:a.getAttribute('href')});},true);}";
			}
		script += "})();";
		g_object_set_data_full(G_OBJECT(parallel_view_), "parallel-populate", g_strdup(script.c_str()), g_free);
		webkit_web_view_load_html(parallel_view_, page.c_str(), "xiphos-preview://study/");
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
