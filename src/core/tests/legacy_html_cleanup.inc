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

// Complete frozen functions, CrossWire c9976a8.
void
ClearImages(gchar *text)
{
	gchar *s;

	for (s = strstr(text, "<img "); s; s = strstr(s, "<img ")) {
		gchar *t;
		if ((t = strchr(s + 4, '>'))) {
			while (s <= t)
				*(s++) = ' ';
		} else {
			XI_message(("ClearImages: no img end: %s\n", s));
			return;
		}
	}
}

//
// utility function to blank font names `<font face="..." />' in text.
//

#define	CLEAR_FONT_NAME		"<font face=\""
#define	CLEAR_FONT_NAME_LENGTH	12

void
ClearFontFaces(gchar *text)
{
	gchar *s;

	// huge assumption: no nested <font> specs: <font face="">
	// is never followed by another <font anything> before </font>.
	for (s = strstr(text, CLEAR_FONT_NAME); s; s = strstr(s, CLEAR_FONT_NAME)) {
		gchar *t;
		s += CLEAR_FONT_NAME_LENGTH;
		if ((t = strchr(s, '"'))) {
			while (s < t)
				*(s++) = '.';
		} else {
			XI_message(("ClearFontFaces: no closing double quote: %s\n", s));
			return;
		}
	}
}

