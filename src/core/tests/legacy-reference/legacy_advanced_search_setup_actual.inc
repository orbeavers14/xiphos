static void set_up_dialog_search(GList *modlist)
{
	backendSearch->clear_scope();
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_custom_range))) {
		// if any non-bible, non-commentary modules are in use,
		// we must not respect this "custom range" selector.
		gboolean range_ok = TRUE;
		gchar *name_for_range = NULL;

		while (modlist) {
			if (name_for_range == NULL)
				name_for_range = (char *)modlist->data;
			int mod_type = backend->module_type((char *)modlist->data);
			if ((mod_type != TEXT_TYPE) &&
			    (mod_type != COMMENTARY_TYPE)) {
				range_ok = FALSE;
				break;
			}
			modlist = g_list_next(modlist);
		}

		if (range_ok) {
			backendSearch->clear_search_list();
			const gchar *label =
			    gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(search1.combo_range))));
			gchar *range =
			    (gchar *)xml_get_list_from_label("ranges", "range", label);
			if (range) {
				backendSearch->set_range(name_for_range, range);
				backendSearch->set_scope2range();
			}
		}
	}

	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search1.rb_last))) {
		backendSearch->set_scope2last_search();
	}

	else {
		backendSearch->clear_search_list();
	}
}

