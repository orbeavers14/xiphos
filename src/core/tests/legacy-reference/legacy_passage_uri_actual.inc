static gint show_in_previewer(const gchar *url)
{

	gchar **work_buf = NULL;
	gchar *mybuf = NULL;

	work_buf = g_strsplit(url, "/", 4);

	// might be an abbrev.  get the real.
	const char *real_mod = main_abbrev_to_name(work_buf[MODULE]);

	mybuf = main_get_rendered_text((real_mod ? real_mod : work_buf[MODULE]),
				       work_buf[KEY]);

	if (mybuf) {
		main_information_viewer(
		    (gchar *)(real_mod ? real_mod : work_buf[MODULE]),
		    mybuf,
		    (gchar *)work_buf[KEY],
		    NULL,
		    NULL,
		    NULL,
		    NULL);
		g_free(mybuf);
	}
	g_strfreev(work_buf);
	return 1;
}


gint sword_uri(const gchar *url, gboolean clicked)
{
	const gchar *key = NULL;
	gchar *tmpkey = NULL;
	gint mod_type;
	gint verse_count;
	gchar **work_buf = NULL;
	gchar *mod;

	// don't recurse between paratab and here.
	static gboolean handling_uri = FALSE;

	if (handling_uri)
		return 0;
	handling_uri = TRUE;

	if (!clicked) {
		gchar *name = g_strstr_len(url, 10, "://");
		if (!name)
			gui_set_statusbar(url);
		else {
			name += 3; // at name beginning.
			gchar *slash = g_strstr_len(name, 20, "/");
			if (slash)
				*slash = '\0'; // limit to name end.

			// might be an abbrev.  get the real.
			const char *real_mod = main_abbrev_to_name(name);

			int mod_type = backend->module_type(real_mod ? real_mod : name);

			if (slash)
				*slash = '/';

			if (mod_type == DICTIONARY_TYPE)
				show_in_previewer(url);
			else {
				const gchar *ref_text = name + ((*name == '/') ? 1 : 0);
				gchar *tag_info = bookmark_get_tag_info_for_key(ref_text);
				if (tag_info) {
					gui_set_statusbar(tag_info);
					g_free(tag_info);
				} else
					gui_set_statusbar(ref_text);
			}
		}
		handling_uri = FALSE;
		return 1;
	}

	work_buf = g_strsplit(url, "/", 4);
	if (!work_buf[MODULE] && !work_buf[KEY]) {
		alert_url_not_found(url);
		g_strfreev(work_buf);
		handling_uri = FALSE;
		return 0;
	}
	if (!work_buf[KEY]) {
		tmpkey = work_buf[MODULE];
	} else
		tmpkey = work_buf[KEY];
	settings.special_anchor = xiphos::core::passage_uri_anchor(tmpkey);

	XI_message(("work_buf: %s, %s",
		    work_buf[MODULE],
		    (work_buf[KEY] ? work_buf[KEY] : "-null-")));

	mod = (work_buf[MODULE] && *work_buf[MODULE]
		   ? work_buf[MODULE]
		   : settings.MainWindowModule);

	const char *real_mod = main_abbrev_to_name(mod);
	if (real_mod)
		mod = (gchar *)real_mod;
	
	if (xiphos::core::passage_uri_reference_list(mod, tmpkey,
	    [](const char *name) { return main_get_mod_type(name) == TEXT_TYPE; })) {
		main_display_verse_list_in_sidebar(settings.currentverse, mod, tmpkey);
		handling_uri = FALSE;
		return 1;
	}

	verse_count = 1; //backend->is_Bible_key(mykey, settings.currentverse);
	if (backend->is_module(mod)) {
		mod_type = backend->module_type(mod);
		switch (mod_type) {
		case TEXT_TYPE:
			key = main_update_nav_controls(mod, tmpkey);
			main_display_bible(mod, key);
			if (settings.comm_showing)
				main_display_commentary(NULL, key);
			main_keep_bibletext_dialog_in_sync((gchar *)key);
			editor_sync_with_main();

			if (key)
				g_free((gchar *)key);
			break;
		case COMMENTARY_TYPE:
		case PERCOM_TYPE:
			if (gtk_notebook_get_current_page(GTK_NOTEBOOK(widgets.notebook_comm_book)) != 0) {
				gchar *save = settings.special_anchor;
				gtk_notebook_set_current_page(
				    GTK_NOTEBOOK(widgets.notebook_comm_book), 0);
				settings.special_anchor = save;
			}
			settings.comm_showing = TRUE;
			key = main_update_nav_controls(mod, tmpkey);
			main_display_commentary(mod, key);
			main_display_bible(NULL, key);
			main_keep_bibletext_dialog_in_sync((gchar *)key);
			if (key)
				g_free((gchar *)key);
			break;
		case DICTIONARY_TYPE:
			main_display_dictionary(mod, tmpkey);
			break;
		case BOOK_TYPE:
		case PRAYERLIST_TYPE:
			if (gtk_notebook_get_current_page(GTK_NOTEBOOK(widgets.notebook_comm_book)) != 1) {
				gchar *save = settings.special_anchor;
				gtk_notebook_set_current_page(
				    GTK_NOTEBOOK(widgets.notebook_comm_book), 1);
				settings.special_anchor = save;
			}
			settings.comm_showing = FALSE;
			main_display_book(mod, tmpkey);
			break;
		}
	} else { /* module name not found or not given */
		if (verse_count) {
			key = main_update_nav_controls(settings.MainWindowModule, tmpkey);
			/* display in current Bible and Commentary */
			main_display_commentary(NULL, key);
			main_display_bible(NULL, key);
			main_keep_bibletext_dialog_in_sync((gchar *)key);

			editor_sync_with_main();

			if (key)
				g_free((gchar *)key);
		} else {
			alert_url_not_found(url);
		}
	}
	g_strfreev(work_buf);
	settings.addhistoryitem = TRUE;
	handling_uri = FALSE;
	return 1;
}

