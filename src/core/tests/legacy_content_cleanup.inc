/*
 * Xiphos Bible Study Tool
 * display.cc -
 *
 * Copyright (C) 2000-2026 Xiphos Developer Team
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

// Complete frozen CleanupContent, CrossWire c9976a8.
GString *
CleanupContent(GString *text,
	       GLOBAL_OPS *ops,
	       const char *name,
	       bool reset = true)
{
	if (ops->image_content == 0)
		ClearImages((gchar *)text->str);
	else if ((ops->image_content == -1) && // "unknown"
		 (strcasestr(text->str, "<img ") != NULL)) {
		ops->image_content = 1; // now known.
		main_save_module_options(name, "Image Content", 1);
	}
	if (ops->respect_font_faces == 0)
		ClearFontFaces((gchar *)text->str);
	else if ((ops->respect_font_faces == -1) && // "unknown"
		 (strcasestr(text->str, CLEAR_FONT_NAME) != NULL)) {
		ops->respect_font_faces = 1; // now known.
		main_save_module_options(name, "Respect Font Faces", 1);
	}

	gint pos;
	gchar value[50], *reported, *s = text->str;

	// test for any 'n="X"' content.  if so, use it directly.
	if ((reported = backend->get_entry_attribute("Footnote", "1", "n", false))) {
		g_free(reported); // dispose of test junk.
	}
	// otherwise we simply count notes & xrefs through the verse.
	else if (ops->xrefnotenumbers) {
		while ((s = strstr(s, "*n"))) {
			g_snprintf(value, 5, "%d", ++footnote);
			pos = s - (text->str) + 2;
			text = g_string_insert(text, pos, value);
			s = text->str + pos + 1;
		}

		s = text->str;
		while ((s = strstr(s, "*x"))) {
			g_snprintf(value, 5, "%d", ++xref);
			pos = s - (text->str) + 2;
			text = g_string_insert(text, pos, value);
			s = text->str + pos + 1;
		}
	}

	return text;
}

