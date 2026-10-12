/*
 * Xiphos Bible Study Tool
 * mod_global_ops.c - setup for SWORD global options and
 *                    a few of our own in the gui
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

// Complete frozen main_new_globals, CrossWire c9976a8.
GLOBAL_OPS *frozen_content_globals(const gchar *mod_name)
{
	GLOBAL_OPS *ops;
	gchar *buf = g_strdup_printf("%s/modops.conf", settings.gSwordDir);
	SWConfig module_options(buf);

	module_options.load();
	g_free(buf);

	ops = g_new0(GLOBAL_OPS, 1);
	ops->strongs =
	    gui_of2tf(module_options[mod_name]["Strong's Numbers"].c_str());
	ops->morphs =
	    gui_of2tf(module_options[mod_name]["Morphological Tags"].c_str());
	ops->lemmas =
	    gui_of2tf(module_options[mod_name]["Lemmas"].c_str());
	ops->transliteration =
	    gui_of2tf(module_options[mod_name]["Transliteration"].c_str());
	ops->italic_headings =
	    gui_of2tf(module_options[mod_name]["Italic Headings"].c_str());
	ops->xlit =
	    gui_of2tf(module_options[mod_name]["Transliterated Forms"].c_str());
	ops->enumerated =
	    gui_of2tf(module_options[mod_name]["Enumerations"].c_str());
	ops->morphseg =
	    gui_of2tf(module_options[mod_name]["Morpheme Segmentation"].c_str());

	ops->variants_all =
	    gui_of2tf(module_options[mod_name]["All Readings"].c_str());
	ops->variants_primary =
	    gui_of2tf(module_options[mod_name]["Primary Reading"].c_str());
	ops->variants_secondary =
	    gui_of2tf(module_options[mod_name]["Secondary Reading"].c_str());
	if ((ops->variants_all == FALSE) &&
	    (ops->variants_primary == FALSE) &&
	    (ops->variants_secondary == FALSE)) {
		ops->variants_primary = TRUE;
	}

	// special case options, default on:
	ops->words_in_red =
	    (module_options[mod_name]["Words of Christ in Red"] != "Off");
	ops->headings =
	    (module_options[mod_name]["Headings"] != "Off");
	ops->footnotes =
	    (module_options[mod_name]["Footnotes"] != "Off");
	ops->scripturerefs =
	    (module_options[mod_name]["Cross-references"] != "Off");
	ops->xrefnotenumbers =
	    (module_options[mod_name]["XrefNoteNumbers"] != "Off");
	ops->display_chapter_N =
	    (module_options[mod_name]["Display Chapter N"] != "Off");

	// more special case default on: heb/grk support.
	ops->greekaccents =
	    (module_options[mod_name]["Greek Accents"] != "Off");
	ops->hebrewpoints =
	    (module_options[mod_name]["Hebrew Vowel Points"] != "Off");
	ops->hebrewcant =
	    (module_options[mod_name]["Hebrew Cantillation"] != "Off");

	// more special case default on: japanese glosses.
	ops->glosses =
	    (module_options[mod_name]["Glosses"] != "Off");

	// we prefer and assume paragraph layout.
	// we take user preference, and alter our default for
	// those modules showing proper configuration for it.
	ops->verse_per_line =
	    ((*(module_options[mod_name]["style"].c_str()) == 'v')
		 ? 1
		 : ((*(module_options[mod_name]["style"].c_str()) == 'p')
			? 0
			: (main_check_for_option(mod_name, "Feature", "NoParagraphs")
			       ? 1
			       : 0))); // default to paragraph style.

	// options not from configuration: xiphos-specific capability.
	ops->image_content =
	    (*(module_options[mod_name]["Image Content"].c_str()) == '\0')
		? -1 // "unknown"; otherwise, it's like the others.
		: gui_of2tf(module_options[mod_name]["Image Content"].c_str());

	ops->respect_font_faces =
	    (*(module_options[mod_name]["Respect Font Faces"].c_str()) == '\0')
		? -1 // "unknown"; otherwise, it's like the others.
		: gui_of2tf(module_options[mod_name]["Respect Font Faces"].c_str());

	// special case, xiphos-specific feature: commentary whole chapter.
	ops->commentary_by_chapter =
	    ((backend->module_type(mod_name) == COMMENTARY_TYPE) &&
	     (module_options[mod_name]["Commentary by Chapter"] != "Off"));

	ops->doublespace =
	    gui_of2tf(module_options[mod_name]["Doublespace"].c_str());

	return ops;
}
