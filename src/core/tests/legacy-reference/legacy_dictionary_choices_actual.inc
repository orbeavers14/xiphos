
GtkWidget *main_dictionary_drop_down_new(char *mod_name, char *old_key)
{
	gchar *key = NULL;
	GtkWidget *menu;

	menu = gtk_menu_new();

	if (!settings.havedict || !mod_name)
		return NULL;
	if (strcmp(settings.DictWindowModule, mod_name)) {
		xml_set_value("Xiphos", "modules", "dict",
			      mod_name);
		settings.DictWindowModule = xml_get_value(
		    "modules", "dict");
	}
	key = g_strdup((gchar *)gtk_entry_get_text(GTK_ENTRY(widgets.entry_dict)));

	XI_message(("\nold_key: %s\nkey: %s", old_key, key));
	backend->set_module_key(mod_name, key);
	g_free(key);
	key = backend->get_module_key();

	xml_set_value("Xiphos", "keys", "dictionary", key);
	settings.dictkey = xml_get_value("keys", "dictionary");

	main_check_unlock(mod_name, TRUE);

	backend->set_module_key(mod_name, key);
	backend->display_mod->display();

	const auto adjacent = xiphos::core::dictionary_nearby_keys(*backend->display_mod);
	for (const auto &new_key : adjacent) {
		GtkWidget *item = gtk_menu_item_new_with_label(new_key.c_str());
		gtk_widget_show(item);
		g_signal_connect(G_OBJECT(item), "activate",
				 G_CALLBACK(dict_key_list_select),
				 g_strdup(new_key.c_str()));
		gtk_container_add(GTK_CONTAINER(menu), item);
	}

	g_free(key);
	return menu;
}

/******************************************************************************
 * Name
 *   main_dictionary_button_clicked
 *
 * Synopsis
 *   #include "main/sword.h"
 *
 *   void main_dictionary_button_clicked(gint direction)
 *
 * Description
 *   The back or foward dictinary key button was clicked.
 *   the module key is set to the current dictkey.
 *   then the module is incremented or decremented.
 *   the new key is returned from the module and the dictionary entry is set
 *   to the new key. The entry is then activated.
 *
 * Return value
 *   void
 */
