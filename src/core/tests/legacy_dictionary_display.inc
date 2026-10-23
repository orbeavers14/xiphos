// Frozen from 0babbf7; complete retained dictionary/devotional display dispatcher.
/*
 * Xiphos Bible Study Tool
 * sword.cc - glue
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

void main_display_dictionary(const char *mod_name,
			     const char *key)
{
	const gchar *old_key, *feature;

	// for devotional use.
	gchar buf[10];

	if (!settings.havedict || !mod_name)
		return;

	XI_message(("main_display_dictionary\nmod_name: %s\nkey: %s", mod_name, key));

	if (!backend->is_module(mod_name))
		return;
	if (!settings.DictWindowModule)
		settings.DictWindowModule = g_strdup((gchar *)mod_name);

	if (key == NULL)
		key = (char *)"Grace";

	feature = (char *)backend->get_mgr()->getModule(mod_name)->getConfigEntry("Feature");

	// turn on "all strong's" iff we have that kind of dictionary.
	if (feature && (!strcmp(feature, "HebrewDef") || !strcmp(feature, "GreekDef")))
		gtk_widget_show(widgets.all_strongs);
	else
		gtk_widget_hide(widgets.all_strongs);

	if (strcmp(settings.DictWindowModule, mod_name)) {
		// new dict -- is it actually a devotional?
		time_t curtime;
		if (feature && !strcmp(feature, "DailyDevotion")) {
			if ((strlen(key) != 5) ||
			    (key[0] < '0') || (key[0] > '9') ||
			    (key[1] < '0') || (key[1] > '9') ||
			    (key[2] != '.') ||
			    (key[3] < '0') || (key[3] > '9') ||
			    (key[4] < '0') || (key[4] > '9')) {
				struct tm *loctime;
				curtime = time(NULL);
				loctime = localtime(&curtime);
				strftime(buf, 10, "%m.%d", loctime);
				key = buf;
			}
			/* ← rediriger vers l'onglet devotional */
			gui_reassign_strdup(&settings.devotionalmod, (gchar *)mod_name);
			xml_set_value("Xiphos", "modules", "devotional", mod_name);
			if (widgets.entry_devotional)
				gtk_entry_set_text(GTK_ENTRY(widgets.entry_devotional), key);
			gtk_notebook_set_current_page(
				GTK_NOTEBOOK(widgets.notebook_dict_devot), 1);
			main_display_devotional(widgets.html_devotional);
			return;   /* ← ne pas continuer vers html_dict */
		}
		xml_set_value("Xiphos", "modules", "dict", mod_name);
		gui_reassign_strdup(&settings.DictWindowModule, (gchar *)mod_name);
	}
	
	gui_notebook_dict_goto_dict();

	// old_key is uppercase
	std::string click_key;
	try {
		click_key = xiphos::core::dictionary_click_key(key);
	} catch (const std::invalid_argument &) {
		XI_message(("Dictionary click key is not valid UTF-8"));
		return;
	}
	key = click_key.c_str();
	old_key = gtk_entry_get_text(GTK_ENTRY(widgets.entry_dict));
	if (!strcmp(old_key, key))
		main_dictionary_entry_changed(settings.DictWindowModule);
	else {
		gtk_entry_set_text(GTK_ENTRY(widgets.entry_dict), key);
		gtk_widget_activate(widgets.entry_dict);
	}
	main_dict_history_add(mod_name, key);

	//if (settings.browsing)
	gui_update_tab_struct(NULL,
			      NULL,
			      mod_name,
			      NULL,
			      key,
			      NULL,
			      settings.comm_showing,
			      settings.showtexts,
			      settings.showpreview,
			      settings.showcomms,
			      settings.showdicts);
}
