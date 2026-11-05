void main_display_book(const char *mod_name,
		       const char *key)
{
	if (!settings.havebook || !mod_name)
		return;

	if (key == NULL)
		key = "0";

	XI_message(("main_display_book\nmod_name: %s\nkey: %s", mod_name, key));

	if (!backend->is_module(mod_name))
		return;
	if (!settings.book_mod)
		settings.book_mod = g_strdup((char *)mod_name);

	if (strcmp(settings.book_mod, mod_name)) {
		xml_set_value("Xiphos", "modules", "book", mod_name);
		gui_reassign_strdup(&settings.book_mod, (gchar *)mod_name);
	}

	if (!isdigit(key[0])) {
		xml_set_value("Xiphos", "keys", "book", key);
		settings.book_key = xml_get_value("keys", "book");

		backend->set_module(mod_name);
		backend->set_treekey(0);
		settings.book_offset = backend->treekey_set_key((char *)key);
	} else {
		settings.book_offset = atol(key);
		if (settings.book_offset < 4)
			settings.book_offset = 4;
		xml_set_value("Xiphos", "keys", "book", key);
		settings.book_key = xml_get_value("keys", "book");
		xml_set_value("Xiphos", "keys", "offset", key);

		backend->set_module(mod_name);
		backend->set_treekey(settings.book_offset);
	}

	main_check_unlock(mod_name, TRUE);

	backend->display_mod->display();
	main_setup_navbar_book(settings.book_mod, settings.book_offset);
	//if (settings.browsing)
	gui_update_tab_struct(NULL,
			      NULL,
			      NULL,
			      mod_name,
			      NULL,
			      key,
			      FALSE,
			      settings.showtexts,
			      settings.showpreview,
			      settings.showcomms,
			      settings.showdicts);
}

