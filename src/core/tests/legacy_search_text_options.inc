// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2004-2026 Xiphos Developer Team
// Frozen from src/main/search_dialog.cc at 0d0f37a.
static void check_search_global_options(void)
{
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_exact_phrase)))
	{
		set_search_global_option("Strong's Numbers",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.cb_include_strongs)));
		set_search_global_option("Morphological Tags",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.cb_include_morphs)));
		set_search_global_option("Footnotes",
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.cb_include_footnotes)));
	}
}

