// Frozen from 2fb7387440c4fc0179a0631695d3cdc35dac33ca, gtk/utilities.c
// GPL-2.0-or-later; Copyright (C) 2000-2026 Xiphos Developer Team
void reading_selector(char *modname,
		      char *key,
		      DIALOG_DATA *dialog,
		      GtkMenuItem *menuitem, gpointer user_data)
{
	gchar *url;
	gboolean primary = 0, secondary = 0, all = 0;
	gboolean is_dialog = (dialog ? 1 : 0);

	switch ((int)GPOINTER_TO_INT(user_data)) {
	case 0:
		primary = TRUE;
		secondary = FALSE;
		all = FALSE;
		break;
	case 1:
		primary = FALSE;
		secondary = TRUE;
		all = FALSE;
		break;
	case 2:
		primary = FALSE;
		secondary = FALSE;
		all = TRUE;
		break;
	default:
		g_message("invalid variant %d\n",
			  (int)GPOINTER_TO_INT(user_data));
		gui_generic_warning("Xiphos: invalid internal variant");
		break;
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
