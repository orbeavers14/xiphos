static void check_search_global_options(void)
{
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_exact_phrase))) {
		const xiphos::core::SearchTextOptions options {
			bool(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.cb_include_strongs))),
			bool(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.cb_include_morphs))),
			bool(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.cb_include_footnotes)))
		};
		for (const auto &setting : xiphos::core::search_text_settings(options))
			set_search_global_option(setting.name, setting.enabled);
	}
}

/******************************************************************************
 * Name
 *   get_current_search_mod
 *
 * Synopsis
 *   #include "gui/search_dialog.h"
 *
 *   GList *get_current_search_mod(void)
 *
 * Description
 *
 *
 * Return value
 *   GList *
 */

