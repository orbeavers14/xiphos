// Frozen from 73f88b4; complete contiguous per-verse color priority block.
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

		// several verse colorization choices.
		// default is no colorization.
		color_choices   = COLOR_NONE;
		color_chosen_fg = NULL;
		color_chosen_bg = NULL;

		// but then decide on the special cases for this verse.
		// (current-verse highlighting is suppressed when a bookmark
		// tag color is active, so it can't override the contrast
		// color already applied to the tag background; annotation
		// highlighting still takes precedence and coexists with it,
		// per review feedback.)
		if (settings.annotate_highlight && e) {
			color_choices   = COLOR_BOTH;
			color_chosen_fg = settings.highlight_bg;
			color_chosen_bg = settings.highlight_fg;
		} else if ((curChapter == thisChapter) && (curVerse == k) && !tag_color) {
			if (settings.versehighlight) {
				color_choices   = COLOR_BOTH;
				color_chosen_fg = settings.highlight_fg;
				color_chosen_bg = settings.highlight_bg;
			} else {
				// regular colorized current verse.
				color_choices   = COLOR_TEXT;
				color_chosen_fg = settings.currentverse_color;
			}
		}

