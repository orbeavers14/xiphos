// SPDX-License-Identifier: GPL-2.0-or-later
#include "rich_editor.hpp"
#include <algorithm>
#include <memory>
namespace xiphos::linux_preview
{
namespace
{
std::string copy(const char *p) { return p ? p : ""; }
} // namespace
RichEditor::RichEditor(std::function<bool(const std::string &)> changed) : changed_(std::move(changed))
{
	view_ = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view_), GTK_WRAP_WORD_CHAR);
	gtk_accessible_update_property(GTK_ACCESSIBLE(view_), GTK_ACCESSIBLE_PROPERTY_LABEL, "Rich document text", -1);
	g_signal_connect(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view_)), "changed", G_CALLBACK(+[](GtkTextBuffer *, gpointer p) {auto *s=static_cast<RichEditor *>(p);if(!s->updating_)s->publish(); }), this);
}
RichEditor::~RichEditor()
{
	if (view_)
		g_signal_handlers_disconnect_by_data(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view_)), this);
}
std::pair<size_t, size_t> RichEditor::selection() const
{
	auto *b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view_));
	GtkTextIter start, end, first, last;
	gtk_text_buffer_get_bounds(b, &first, &last);
	auto *text = gtk_text_buffer_get_text(b, &first, &last, TRUE);
	if (!gtk_text_buffer_get_selection_bounds(b, &start, &end)) {
		gtk_text_buffer_get_iter_at_mark(b, &start, gtk_text_buffer_get_insert(b));
		end = start;
	}
	const auto a = g_utf8_offset_to_pointer(text, gtk_text_iter_get_offset(&start)) - text, z = g_utf8_offset_to_pointer(text, gtk_text_iter_get_offset(&end)) - text;
	g_free(text);
	return {size_t(a), size_t(z - a)};
}
bool RichEditor::load(const std::string &html)
{
	xc_rich_document *raw = nullptr;
	if (xc_rich_document_parse(html.c_str(), &raw) != XC_OK)
		return false;
	std::unique_ptr<xc_rich_document, decltype(&xc_rich_document_release)> doc(raw, xc_rich_document_release);
	auto *b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view_));
	GtkTextIter first, last;
	if (!gtk_text_buffer_get_selection_bounds(b, &first, &last)) {
		gtk_text_buffer_get_iter_at_mark(b, &first, gtk_text_buffer_get_insert(b));
		last = first;
	}
	const int insert = gtk_text_iter_get_offset(&first), bound = gtk_text_iter_get_offset(&last);
	updating_ = true;
	gtk_text_buffer_set_text(b, "", 0);
	auto *table = gtk_text_buffer_get_tag_table(b);
	for (auto *tag : tags_)
		gtk_text_tag_table_remove(table, tag);
	tags_.clear();
	for (size_t i = 0; i < xc_rich_document_run_count(doc.get()); ++i) {
		xc_rich_run run{};
		xc_rich_document_run(doc.get(), i, &run);
		auto *style = new Style{run.flags, run.block, copy(run.foreground), copy(run.background), copy(run.link)};
		auto *tag = gtk_text_buffer_create_tag(b, nullptr, "weight", (run.flags & XC_RICH_STYLE_BOLD) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL, "style", (run.flags & XC_RICH_STYLE_ITALIC) ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL, "underline", (run.flags & XC_RICH_STYLE_UNDERLINE) || !style->link.empty() ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE, "strikethrough", (run.flags & XC_RICH_STYLE_STRIKE) != 0, nullptr);
		if (!style->foreground.empty())
			g_object_set(tag, "foreground", style->foreground.c_str(), nullptr);
		if (!style->background.empty())
			g_object_set(tag, "background", style->background.c_str(), nullptr);
		if (run.block >= XC_RICH_H1 && run.block <= XC_RICH_H6)
			g_object_set(tag, "scale", 1.0 + (7 - run.block) * 0.12, nullptr);
		if (run.block == XC_RICH_PREFORMATTED)
			g_object_set(tag, "family", "monospace", nullptr);
		g_object_set_data_full(G_OBJECT(tag), "xiphos-style", style, +[](gpointer p) { delete static_cast<Style *>(p); });
		tags_.push_back(tag);
		GtkTextIter end;
		gtk_text_buffer_get_end_iter(b, &end);
		gtk_text_buffer_insert_with_tags(b, &end, run.text ? run.text : "", -1, tag, nullptr);
	}
	const int count = gtk_text_buffer_get_char_count(b);
	gtk_text_buffer_get_iter_at_offset(b, &first, std::min(insert, count));
	gtk_text_buffer_get_iter_at_offset(b, &last, std::min(bound, count));
	gtk_text_buffer_select_range(b, &first, &last);
	html_ = html;
	updating_ = false;
	return true;
}
void RichEditor::publish()
{
	struct Run {
		std::string text;
		Style style;
	};
	std::vector<Run> owned;
	auto *b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view_));
	GtkTextIter at, end;
	gtk_text_buffer_get_bounds(b, &at, &end);
	while (gtk_text_iter_compare(&at, &end) < 0) {
		Style style;
		auto *tags = gtk_text_iter_get_tags(&at);
		int priority = -1;
		for (auto *p = tags; p; p = p->next) {
			auto *s = static_cast<Style *>(g_object_get_data(G_OBJECT(p->data), "xiphos-style"));
			if (s && gtk_text_tag_get_priority(GTK_TEXT_TAG(p->data)) > priority) {
				style = *s;
				priority = gtk_text_tag_get_priority(GTK_TEXT_TAG(p->data));
			}
		}
		g_slist_free(tags);
		auto next = at;
		if (!gtk_text_iter_forward_to_tag_toggle(&next, nullptr) || gtk_text_iter_compare(&next, &end) > 0)
			next = end;
		auto *text = gtk_text_buffer_get_text(b, &at, &next, TRUE);
		owned.push_back({text, std::move(style)});
		g_free(text);
		at = next;
	}
	std::vector<xc_rich_run> runs;
	for (const auto &r : owned)
		runs.push_back({r.text.c_str(), r.style.flags, r.style.block, r.style.foreground.c_str(), r.style.background.c_str(), r.style.link.c_str()});
	xc_rich_document *raw = nullptr;
	bool accepted = false;
	if (xc_rich_document_create(runs.data(), runs.size(), &raw) == XC_OK) {
		auto html = copy(xc_rich_document_html(raw));
		accepted = changed_(html);
		if (accepted)
			html_ = std::move(html);
		xc_rich_document_release(raw);
	}
	if (!accepted)
		load(html_);
}
} // namespace xiphos::linux_preview
