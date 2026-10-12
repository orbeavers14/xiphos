/*
 * Xiphos Bible Study Tool
 * url.cc - support functions
 *
 * Copyright (C) 2004-2026 Xiphos Developer Team
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
// Frozen complete main note/reference handlers from CrossWire c9976a8.
static gint show_note(const gchar *module, const gchar *passage,
		      const gchar *stype, const gchar *svalue, gboolean clicked)
{
	gchar *tmpbuf = NULL;
	gchar *buf = NULL;
	gchar *work_buf = NULL;
	GString *str = g_string_new(NULL);
	GList *vlist = NULL, *chaser = NULL;

	if (!in_url || !svalue || !*svalue)
		return 1;

	if (!backend->is_module((gchar *)module))
		module = settings.MainWindowModule;

	if (strlen(passage) < 5)
		passage = settings.currentverse;

	//
	// if we are asking for a note/xref in n:0,
	// we must stop autonormalization for a moment.
	//
	int stop_autonorm = ((strstr(passage, ":0")) != NULL);

	if (stop_autonorm) {
		SWMgr *mgr = backend->get_mgr();
		backend->display_mod = mgr->Modules[module];
		backend->display_mod->setKey(passage);
		VerseKey *vkey = (VerseKey *)(SWKey *)(*backend->display_mod);
		int t = backend->module_type(module);
		if ((t == TEXT_TYPE) || (t == COMMENTARY_TYPE))
			vkey->setAutoNormalize(0);
		vkey->setChapter(vkey->getChapter() + 1);
		vkey->setVerse(0);
	} else
		backend->set_module_key((gchar *)module, (gchar *)passage);

	if (strchr(stype, 'x') && clicked) {
		tmpbuf = backend->get_entry_attribute("Footnote",
						      (gchar *)svalue,
						      "refList");
		if (tmpbuf) {
			main_display_verse_list_in_sidebar(settings.currentverse,
							   (gchar *)module,
							   tmpbuf);
			g_free(tmpbuf);
		}
	} else if (strchr(stype, 'n') && !clicked) {
		tmpbuf = backend->get_entry_attribute("Footnote",
						      (gchar *)svalue,
						      "body");
		buf = backend->render_this_text((gchar *)module, (gchar *)tmpbuf);
		if (tmpbuf)
			g_free(tmpbuf);
		if (buf) {
			main_information_viewer((gchar *)module,
						buf,
						(gchar *)svalue,
						"showNote",
						(gchar *)stype,
						NULL,
						NULL);
			if (buf)
				g_free(buf);
		}
	} else if (strchr(stype, 'x') && !clicked) {
		tmpbuf = backend->get_entry_attribute("Footnote",
						      (gchar *)svalue,
						      "refList");
		if (settings.xrefs_in_verse_list) {
			main_display_verse_list_in_sidebar(settings.currentverse,
							   (gchar *)module,
							   tmpbuf);
			g_free(tmpbuf);
		} else {
			vlist = chaser = backend->parse_verse_list(module, tmpbuf, settings.currentverse);
			while (chaser != NULL) {
				buf = g_strdup_printf(
				    "<a href=\"sword://%s/%s\">"
				    "<font color=\"%s\">%s,</font></a><br/>",
				    (gchar *)module,
				    (const char *)chaser->data,
				    settings.bible_text_color,
				    (const char *)chaser->data);
				str = g_string_append(str, buf);
				g_free(buf);
				g_free((char *)chaser->data);
				chaser = g_list_next(chaser);
			}
			g_list_free(vlist);
			g_free(tmpbuf);

			buf = g_strdup_printf("<a href=\"sword://%s/%s\">"
					      "<font color=\"%s\">%s%s</font></a><br/>",
					      (gchar *)module,
					      settings.currentverse,
					      settings.bible_text_color,
					      _("Back to "),
					      settings.currentverse);
			str = g_string_append(str, buf);
			g_free(buf);

			main_information_viewer((gchar *)module,
						str->str,
						(gchar *)svalue,
						"showNote",
						(gchar *)stype,
						NULL,
						NULL);
		}
	}

	if (work_buf)
		g_free(work_buf);
	g_string_free(str, TRUE);
	return 1;
}

static gint show_ref(const gchar *module, const gchar *list, gboolean clicked)
{
	if (!clicked)
		return 1;

	if (!backend->is_module(module))
		module = settings.MainWindowModule;
	main_display_verse_list_in_sidebar(settings.currentverse,
					   (gchar *)module,
					   (gchar *)list);
	return 1;
}
