// SPDX-License-Identifier: GPL-2.0-or-later
// Original main_new_globals initialization from crosswire/xiphos c9976a8.
// Copyright (C) 2000-2026 Xiphos Developer Team
GLOBAL_OPS *legacy_new_globals(const gchar *mod_name)
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

	return ops;
}
