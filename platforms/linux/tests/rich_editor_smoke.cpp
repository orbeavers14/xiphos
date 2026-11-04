// SPDX-License-Identifier: GPL-2.0-or-later
#include "rich_editor.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace xiphos::linux_preview;
void check(bool ok, const char *why)
{
	if (!ok)
		throw std::runtime_error(why);
}
int main()
try {
	g_setenv("GSK_RENDERER", "cairo", FALSE);
	if (!gtk_init_check()) {
		std::cerr << "GTK display unavailable\n";
		return 77;
	}
	std::string changed;
	auto *window = gtk_window_new();
	{
		RichEditor editor([&](const std::string &html) {changed=html;return true; });
		gtk_window_set_child(GTK_WINDOW(window), editor.widget());
		gtk_window_present(GTK_WINDOW(window));
		check(editor.load("<p><b>Original</b> café <span style='color:#123456'>α</span></p>"), "Supported rich source");
		auto *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor.widget()));
		GtkTextIter at;
		gtk_text_buffer_get_end_iter(buffer, &at);
		gtk_text_buffer_insert(buffer, &at, " Ω", -1);
		check(!changed.empty() && changed.find("#123456") != std::string::npos, "GTK typing preserves core color and style runs");
		xc_rich_document *raw = nullptr;
		check(xc_rich_document_parse(changed.c_str(), &raw) == XC_OK, "Edited output remains shared-supported markup");
		std::string plain = xc_rich_document_text(raw);
		xc_rich_document_release(raw);
		check(plain.find("Ω") != std::string::npos && plain.find("café") != std::string::npos, "Unicode native editing retained");
		GtkTextIter start, end;
		gtk_text_buffer_get_iter_at_offset(buffer, &start, 9);
		gtk_text_buffer_get_iter_at_offset(buffer, &end, 13);
		gtk_text_buffer_select_range(buffer, &start, &end);
		const auto selected = editor.selection();
		check(selected.second > 4, "GTK character selection converts to UTF8 bytes");
		check(!editor.load("<script>unsupported()</script>"), "Unsupported markup retains Source fallback");
		while (g_main_context_iteration(nullptr, FALSE)) {
		}
	}
	gtk_window_destroy(GTK_WINDOW(window));
	std::cout << "GTK rich runs, Unicode selection, style preservation and Source fallback passed\n";
	return 0;
} catch (const std::exception &e) {
	std::cerr << e.what() << '\n';
	return 1;
}
