void main_save_current_adv_search_as_bookmarks(void)
{
	RESULTS *list_item;
	gchar *module_name = NULL;

	list_for_bookmarking = g_list_first(list_for_bookmarking);
	while (list_for_bookmarking) {
		GList *verses = (GList *)list_for_bookmarking->data;

		GString *name = g_string_new(NULL);
		GString *verse_string = g_string_new("");
		gboolean first_entry = TRUE;

		while (verses) {
			list_item = (RESULTS *)verses->data;
			if (main_is_Bible_key(list_item->module, list_item->key)) {
				if (first_entry) {
					module_name = g_strdup(list_item->module);
					first_entry = FALSE;
				} else {
					verse_string = g_string_append(verse_string,
								       "; ");
				}
				verse_string = g_string_append(verse_string,
							       list_item->key);
			}
			verses = g_list_next(verses);
		}
		g_string_printf(name, _("Search result %s: %s"), module_name, settings.searchText);
		gui_bookmark_dialog(name->str, module_name, verse_string->str);
		g_string_free(name, TRUE);
		g_string_free(verse_string, TRUE);
		g_free(module_name);
		list_for_bookmarking = g_list_next(list_for_bookmarking);
	}
}

/******************************************************************************
 * Name
 *   main_range_text_changed
 *
 * Synopsis
 *   #include "gui/search_dialog.h"
 *
 *   void main_range_text_changed(GtkEditable * editable)
 *
 * Description
 *   text in the range text entry has changed
 *   text is sent to the backend to get the search range list
 *   then list_ranges is updated to the range list
 *   and the range text in the clist_range is updated
 *
 * Return value
 *   void
 */

