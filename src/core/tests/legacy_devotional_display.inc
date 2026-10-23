// Frozen from 595e127; complete retained devotional renderer.
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

void main_display_devotional(GtkWidget *target_widget)
{
	gchar buf[10];
	gchar *prettybuf;
	time_t curtime;
	struct tm *loctime;
	gchar *text;

	/*
	 * This makes sense only if you've installed & defined one.
	 */
	if (settings.devotionalmod == NULL) {
		GList *glist = get_list(DEVOTION_LIST);

		if (g_list_length(glist) != 0) {
			xml_set_value("Xiphos", "modules", "devotional",
				      (char *)glist->data);
			gui_reassign_strdup(&settings.devotionalmod, (gchar *)glist->data);
		} else {
			gui_generic_warning(_("Daily devotional was requested, but there are none installed."));
		}
	}

	/*
	 * Get the current time, converted to local time.
	 */
	curtime = time(NULL);
	loctime = localtime(&curtime);
	strftime(buf, 10, "%m.%d", loctime);   /* date par défaut = aujourd'hui */

	if (widgets.entry_devotional &&
		strlen(gtk_entry_get_text(GTK_ENTRY(widgets.entry_devotional))) == 5) {
		/* utiliser la date saisie/sélectionnée */
		strncpy(buf, gtk_entry_get_text(GTK_ENTRY(widgets.entry_devotional)), 10);
		int month = atoi(buf);
		int day   = atoi(buf+3);
		prettybuf = g_strdup_printf("<b>%s %d</b>",
					    gettext(month_names[month - 1]), day);
	} else {
		prettybuf = g_strdup_printf("<b>%s %d</b>",
					    gettext(month_names[loctime->tm_mon]),
					    loctime->tm_mday);
	}

	text = backend->get_render_text(settings.devotionalmod, buf);
	if (text) {
		if (target_widget != NULL) {
			SWDisplay *saved = backend->dictDisplay;
			backend->dictDisplay = new GTKEntryDisp(target_widget, backend);
			backend->set_module_key(settings.devotionalmod, buf);
			backend->dictDisplay->display(*backend->display_mod);
			delete backend->dictDisplay;
			backend->dictDisplay = saved;
		} else {
			GtkWidget *dest = settings.show_previewer_in_sidebar
					? sidebar.html_viewer_widget
					: widgets.html_previewer_text;
			main_entry_display(dest, settings.devotionalmod,
					   text, prettybuf, TRUE);
		}
		g_free(text);
		if (widgets.entry_devotional)
			gtk_entry_set_text(GTK_ENTRY(widgets.entry_devotional), buf);
		if (widgets.button_devotional_date) {
		gchar *local = format_devot_date_local(buf);
		gtk_button_set_label(GTK_BUTTON(widgets.button_devotional_date), local);
		g_free(local);
		}
	}
	g_free(prettybuf);
}  
