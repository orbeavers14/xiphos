void reading_selector(char *modname,
		      char *key,
		      DIALOG_DATA *dialog,
		      GtkMenuItem *menuitem, gpointer user_data)
{
	gchar *url;
	gboolean primary = 0, secondary = 0, all = 0;
	gboolean is_dialog = (dialog ? 1 : 0);

	const XiphosVariantSelection selection =
	    xiphos_core_variant_selection((int)GPOINTER_TO_INT(user_data));
	primary = selection.primary;
	secondary = selection.secondary;
	all = selection.all;
	if (!selection.valid) {
		g_message("invalid variant %d\n",
			  (int)GPOINTER_TO_INT(user_data));
		gui_generic_warning("Xiphos: invalid internal variant");
	}

	url = g_strdup_printf("sword://%s/%s", modname, key);
	main_save_module_options(modname, "Primary Reading", primary);
	main_save_module_options(modname, "Secondary Reading", secondary);
	main_save_module_options(modname, "All Readings", all);
	if (is_dialog)
		main_dialogs_url_handler(dialog, url, TRUE);
	else
		main_url_handler(url, TRUE);

	g_free(url);
}
