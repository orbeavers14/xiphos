// Frozen from 5e45425; complete detached parallel orchestration.
/*
 * Xiphos Bible Study Tool
 * parallel_view.cc - support for displaying multiple modules
 *
 * Copyright (C) 2004-2026 Xiphos Developer Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#define HTML_START \
	"<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />\
<style type=\"text/css\">\
A { text-decoration:none } \
*[dir=rtl] { text-align: right; }\
h3 { font-style: %s }\
%s %s %s \
</style></head>"
// last 3 "%s" are for CSS init from getRenderHeader() sticky modnames, and margin justify.

// how to build a table whose top row of modname labels doesn't scroll. based on:
// http://geeksforgeeks.org/how-to-create-a-table-with-fixed-header-and-scrollable-body/
#define	STICKY_MODNAMES \
"<style> \
.table-container { overflow-y: auto; width: 100%; } \
table { width: 100%; border-collapse: collapse; } \
thead th { position: sticky; top: 0; z-index: 1; } \
th { text-align: center; } \
</style>"

void main_update_parallel_page_detached(void)
{
	SWBuf text("");
	gchar buf[5000];
	gint modidx, parallel_count, fraction;

	if (!widgets.html_parallel_dialog ||
	    !gtk_widget_get_realized(GTK_WIDGET(widgets.html_parallel_dialog)))
		return;

	/* how big a pile of parallels have we got? */
	if (settings.parallel_list == NULL)
		return;
	for (parallel_count = 0; settings.parallel_list[parallel_count]; ++parallel_count)
		/* just count non-null string ptrs */;

	/* get the per-column percentage width. */
	/* 2 => 50, 4 => 25, 5 => 20, 10 => 10, ... */
	fraction = (parallel_count ? (100 / parallel_count) : 100);

	// identify the module whose v11n and CSS display defaults will drive us.
	// should be 1st parallel module.
	// it would be really weird if that's missing, but be ready anyhow.
	char *control_name = (settings.parallel_list
			     ? settings.parallel_list[0]
			     : settings.MainWindowModule);
	// might have an abbrev. get the real.
	const char *real_mod = main_abbrev_to_name(control_name);
	if (real_mod)
		control_name = (gchar *)real_mod;

	SWModule *control = backend->get_SWModule(control_name);
	if (!control)
	{
		gui_generic_warning(_("Failed to find 1st parallel module for display control."));
		return;
	}

	snprintf(buf, 4999, HTML_START
		 "<body bgcolor=\"%s\" text=\"%s\" link=\"%s\">"
		 "  <div class=\"table-container\">"
		 "    <table>"
		 "      <thead>"
		 "        <tr>",
		 (settings.parallel_italic_headings ? "italic" : "bold"),
		 control->getRenderHeader(),
		 STICKY_MODNAMES,
		 (settings.justify_margins ? "<style> td { text-align: justify; padding: 5px; } </style>" : ""),
		 settings.bible_bg_color, settings.bible_text_color,
		 settings.link_color);
	text += buf;

	for (modidx = 0; settings.parallel_list[modidx]; ++modidx) {
		const char *abbreviation = main_name_to_abbrev(settings.parallel_list[modidx]);
		snprintf(buf, 499,
			 "<th width=\"%d%%\" bgcolor=\"#c0c0c0\"><font color=\"%s\" size=\"%+d\"><b>%s</b></font></th>",
			 fraction,
			 settings.bible_verse_num_color,
			 settings.verse_num_font_size + settings.base_font_size,
			 (abbreviation ? abbreviation : settings.parallel_list[modidx]));
		text += buf;
	}

	text += "</tr> </thead> <tbody>";
	interpolate_parallel_display(control, control_name, text, settings.cvparallel, parallel_count, fraction);
	text += "</tbody> </table> </div> </body> </html>";

	snprintf(buf, 499, "%d", settings.intCurVerse);

	HtmlOutput((char *)(settings.imageresize
				? AnalyzeForImageSize((char *)text.c_str(), parallel_count*2/3,
						      GDK_WINDOW(gtk_widget_get_window(widgets.html_parallel_dialog)))
				: (char *)text.c_str()),
		   widgets.html_parallel_dialog, NULL, buf);
}
