// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2004-2026 Xiphos Developer Team
// Frozen verbatim conditional from src/main/search_dialog.cc at 0d2e73f.
	// For attribute-based searches, e.g. "Word//Lemma/G140",
	// we must constrain the match to whole words.  Otherwise,
	// we will inadvertently return e.g. 140 plus 1401 and 1404.
	if (search_type == -3) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_strongs))) {
			search_params |= SpecialSearchFlag;
			attribute_search_string = g_strdup_printf(
			    "Word//Lemma./%s",
			    search_string);
		} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_morphs))) {
			search_params |= SpecialSearchFlag;
			attribute_search_string = g_strdup_printf(
			    "Word//Morph/%s",
			    search_string);
		} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_footnotes))) {
			attribute_search_string = g_strdup_printf(
			    "Footnote//body/%s",
			    search_string);
		}
		XI_message(("%s", attribute_search_string));
	}

