
void main_dictionary_button_clicked(gint direction)
{
	gchar *key = NULL;

	if (!settings.havedict || !settings.DictWindowModule)
		return;

	backend->set_module_key(settings.DictWindowModule,
				settings.dictkey);
	const std::string adjacent = xiphos::core::dictionary_step_key(
	    *backend->display_mod, direction == 0 ? -1 : 1);
	key = g_strdup(adjacent.c_str());
	gtk_entry_set_text(GTK_ENTRY(widgets.entry_dict), key);
	gtk_widget_activate(widgets.entry_dict);
	g_free(key);
}
