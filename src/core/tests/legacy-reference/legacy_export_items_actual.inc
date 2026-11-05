static gchar *get_module(const gchar *key)
{
	if (main_is_Bible_key(settings.MainWindowModule, (gchar *)key))
		return g_strdup(settings.MainWindowModule);
	return NULL;
}

void gui_set_html_item(GString *str,
		       const gchar *description,
		       const gchar *module,
		       const gchar *key, gboolean with_scripture)
{
	gchar *scripture = NULL;
	gchar *buf = NULL;

	if (with_scripture) {
		gchar *mod_tmp = NULL;
		gint modtype;

		if (strlen(module) < 2)
			mod_tmp = get_module(key);
		else
			mod_tmp = g_strdup(module);

		if (((modtype = main_get_mod_type(mod_tmp)) == TEXT_TYPE) || (modtype == COMMENTARY_TYPE)) {
			gchar *text = main_get_rendered_text(mod_tmp, key);
			scripture = g_strdup_printf("%s", text);
			g_free(text);
		}

		if (mod_tmp)
			g_free(mod_tmp);
	}

	buf = xiphos_export_item(description, module, key, scripture, 1);
	if (!buf)
		g_error("Unable to format export item");
	g_string_append(str, buf);
	xiphos_export_item_free(buf);
	g_free(scripture);
}

void gui_set_plain_text_item(GString *str,
			     const gchar *description,
			     const gchar *module,
			     const gchar *key, gboolean with_scripture)
{
	gchar *scripture = NULL;
	gchar *buf = NULL;

	if (with_scripture) {
		gchar *mod_tmp = NULL;
		gint modtype;

		if (strlen(module) < 2)
			mod_tmp = get_module(key);
		else
			mod_tmp = g_strdup(module);

		if (((modtype = main_get_mod_type(mod_tmp)) == TEXT_TYPE) || (modtype == COMMENTARY_TYPE))
			scripture =
			    main_get_striptext(mod_tmp, (gchar *)key);

		if (mod_tmp)
			g_free(mod_tmp);
	} else
		scripture = g_strdup("");

	buf = xiphos_export_item(description, module, key, scripture, 0);
	if (!buf)
		g_error("Unable to format export item");
	g_string_append(str, buf);
	xiphos_export_item_free(buf);
	g_free(scripture);
}

