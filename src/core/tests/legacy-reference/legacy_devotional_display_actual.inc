
void main_display_devotional(GtkWidget *target_widget)
{
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
	const auto date = xiphos::core::devotional_date(
	    widgets.entry_devotional
	        ? gtk_entry_get_text(GTK_ENTRY(widgets.entry_devotional)) : "",
	    loctime->tm_mon + 1, loctime->tm_mday);
	const char *buf = date.key.c_str();
	prettybuf = g_strdup_printf("<b>%s %d</b>",
	                            gettext(month_names[date.month - 1]), date.day);

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