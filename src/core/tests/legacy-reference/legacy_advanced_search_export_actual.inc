gboolean main_export_current_adv_search(GString *str, gboolean html, gboolean with_scripture)
{
	GList *tmp = NULL;
	RESULTS *list_item;
	gboolean ret = FALSE;
	gchar *desc;

	const gchar *search_string = gtk_entry_get_text(GTK_ENTRY(search1.search_entry));
	gchar search_type[100];

	g_strlcpy(search_type,
		  (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_regexp))
		   ? _("Regular expression")
		   : (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_exact_phrase))
		      ? _("Exact phrase")
		      : (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_words))
			 ? _("Multi-word")
			 : (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_optimized))
			    ? _("Optimized (\"Lucene\")")
			    : _("Attribute"))))), 99);


	tmp = g_list_first(list_for_bookmarking);
	if (html)
		str = g_string_append(str, "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" /></head><body>");
	while (tmp) {
		GList *verses = (GList *)tmp->data;

		GString *verse_string = g_string_new("");
		gboolean first_entry = TRUE;

		while (verses) {
			list_item = (RESULTS *)verses->data;
			if (main_is_Bible_key(list_item->module, list_item->key)) {
				ret = TRUE;
				if (first_entry) {
					if (html)
						g_string_printf(verse_string,
								"<b>%s</b><br/>%s<br/><br/><ul><b>%s</b>",
								search_string, search_type,
								list_item->module);
					else
						g_string_printf(verse_string, "%s\n%s\n\n%s\n",
								search_string, search_type,
								list_item->module);
					first_entry = FALSE;
					str = g_string_append(str, verse_string->str);
				} else {
					desc = g_strdup_printf("%s %s",
							       list_item->key,
							       list_item->module);
					if (html)
						gui_set_html_item(str,
								  desc,
								  list_item->module,
								  list_item->key,
								  with_scripture);
					else
						gui_set_plain_text_item(str,
									desc,
									list_item->module,
									list_item->key,
									with_scripture);
				}
			}
			verses = g_list_next(verses);
		}
		g_string_free(verse_string, TRUE);
		if (html)
			str = g_string_append(str, "</ul>");
		tmp = g_list_next(tmp);
	}
	return ret;
}

/******************************************************************************
 * Name
 *   main_save_current_adv_search_as_bookmarks
 *
 * Synopsis
 *   #include "main/search_dialog.h"
 *
 *   void main_save_current_adv_search_as_bookmarks (void)
 *
 * Description
 *   saves search results as bookmarks - single bookmark per module
 *
 *
 * Return value
 *   void
 */

