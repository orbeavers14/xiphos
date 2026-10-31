// Frozen from 95bdfbb; complete parallel-set delete callback.
/*
 * Xiphos Bible Study Tool
 * preferences_dialog.c - get user preferences
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
static void on_parallel_set_delete_clicked(GtkWidget *btn, gpointer user_data)
{
	gchar *name = gtk_combo_box_text_get_active_text(
	    GTK_COMBO_BOX_TEXT(parallel_select.sets_combo));

	if (!name || g_strcmp0(name, _("New set...")) == 0) {
		g_free(name);
		return;
	}

	/* remove from set_names */
	gchar **names = g_strsplit(settings.parallel_set_names, ",", -1);
	GString *new_names = g_string_new("");
	for (gint i = 0; names[i]; ++i) {
		if (g_strcmp0(names[i], name) != 0) {
			if (new_names->len > 0)
				g_string_append_c(new_names, ',');
			g_string_append(new_names, names[i]);
		}
	}
	g_strfreev(names);

	g_free(settings.parallel_set_names);
	settings.parallel_set_names = g_string_free(new_names, FALSE);
	xml_set_or_create_value("modules", "parallel_set_names",
			    settings.parallel_set_names);

	/* remove the set key */
	gchar *key = g_strdup_printf("parallel_set_%s", name);
	xml_remove_node("modules", key, NULL);
	g_free(key);

	/* reset current if needed */
	if (g_strcmp0(settings.parallel_set_current, name) == 0) {
		g_free(settings.parallel_set_current);
		settings.parallel_set_current = NULL;
		xml_set_or_create_value("modules", "parallel_set_current", "");
	}

	xml_save_settings_doc(settings.fnconfigure);

	/* rebuild combo */
	g_signal_handlers_block_by_func(parallel_select.sets_combo,
					on_parallel_sets_combo_changed, NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_combo_box_text_remove_all(
	    GTK_COMBO_BOX_TEXT(parallel_select.sets_combo));
#else
	{
		GtkTreeModel *m = gtk_combo_box_get_model(
		    GTK_COMBO_BOX(parallel_select.sets_combo));
		gtk_list_store_clear(GTK_LIST_STORE(m));
	}
#endif
	if (settings.parallel_set_names && *settings.parallel_set_names) {
		gchar **all = g_strsplit(settings.parallel_set_names, ",", -1);
		for (gint i = 0; all[i]; ++i)
			gtk_combo_box_text_append_text(
			    GTK_COMBO_BOX_TEXT(parallel_select.sets_combo), all[i]);
		g_strfreev(all);
	}
	gtk_combo_box_text_append_text(
	    GTK_COMBO_BOX_TEXT(parallel_select.sets_combo), _("New set..."));
	gtk_combo_box_set_active(GTK_COMBO_BOX(parallel_select.sets_combo), 0);
	g_signal_handlers_unblock_by_func(parallel_select.sets_combo,
					  on_parallel_sets_combo_changed, NULL);

	g_free(name);
}
