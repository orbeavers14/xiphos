// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2003-2026 Xiphos Developer Team
// Frozen verbatim from src/gtk/export_bookmarks.c at f205722.
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
			scripture = g_strdup_printf("<li>%s</li>", text);
			g_free(text);
		}

		if (mod_tmp)
			g_free(mod_tmp);
	} else
		scripture = g_strdup("");

	{
		gchar *desc_esc = g_markup_escape_text(description, -1);
		gchar *mod_esc = g_markup_escape_text(module, -1);
		gchar *key_esc = g_markup_escape_text(key, -1);

		buf = g_strdup_printf("<li>%s<ul><li>%s %s</li>%s</ul><br/></li>",
				      desc_esc,
				      mod_esc, key_esc, (scripture ? scripture : ""));

		g_free(desc_esc);
		g_free(mod_esc);
		g_free(key_esc);
	}

	g_string_append(str, buf);
	g_free(buf);
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

	buf = g_strdup_printf("\t%s\n\t\t%s %s\n\t\t%s\n\n",
			      description,
			      module, key, (scripture ? scripture : ""));

	g_string_append(str, buf);
	g_free(buf);
	g_free(scripture);
}

