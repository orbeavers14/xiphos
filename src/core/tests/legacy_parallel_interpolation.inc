// Frozen from c973bc7; complete parallel heading and interpolation routines.
/*
 * Xiphos Bible Study Tool
 * parallel_view.cc - support for displaying multiple modules
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

void get_heading(SWBuf &text, BackEnd *p, gint modidx)
{
	const gchar *preverse;
	gchar heading[8];

	int x = 0;
	sprintf(heading, "%d", x);
	while ((preverse = p->get_entry_attribute("Heading", "Preverse",
						  heading)) != NULL) {
		const gchar *preverse2, *buf;

		preverse2 = p->render_this_text(settings.parallel_list[modidx], preverse);
		buf = g_strdup_printf("<br/><b>%s</b><br/><br/>",
				      (preverse2 ? preverse2 : ""));

		text += buf;

		g_free((gchar *)preverse2);
		g_free((gchar *)preverse);
		g_free((gchar *)buf);
		++x;
		sprintf(heading, "%d", x);
	}
}
static void interpolate_parallel_display(SWModule *control,
					 char     *control_name,
					 SWBuf    &text,
					 gchar    *key,
					 gint     parallel_count,
					 gint     fraction)
{
	// we are guaranteed that control, the module whose v11n
	// controls our verse limit and display choices, exists.

	gchar *utf8str, *textColor, *tmpkey, tmpbuf[256];
	const gchar *bgColor;
	gchar str[500];
	gint cur_verse, cur_chapter, verse, modidx;
	char *cur_book;
	MOD_FONT **mf;
	gboolean *is_rtol, *is_module, *is_bible_text;

	if (!gtk_widget_get_realized(GTK_WIDGET(widgets.notebook_bible_parallel)))
		return;

	// need #verses to process in this chapter.

	VerseKey *vkey = (VerseKey *)control->createKey();
	int xverses;

	vkey->setAutoNormalize(1);
	vkey->setText(key);
	xverses = (vkey->getVerseMax());
	delete vkey;

	is_module = g_new(gboolean, parallel_count);
	is_rtol = g_new(gboolean, parallel_count);
	is_bible_text = g_new(gboolean, parallel_count);
	mf = g_new(MOD_FONT *, parallel_count);

	// quick cache of fonts/rtol/type info.
	for (modidx = 0; modidx < parallel_count; ++modidx) {
		gchar *mod = settings.parallel_list[modidx];
		// might have an abbrev. get the real.
		const char *real_mod = main_abbrev_to_name(mod);
		if (real_mod)
			mod = (gchar *)real_mod;

		// determine module presence once each.
		is_module[modidx] = backend->is_module(mod);

		if (is_module[modidx]) {
			is_rtol[modidx] = main_is_mod_rtol(mod);
			mf[modidx] = get_font(mod);
			is_bible_text[modidx] =
			    (main_get_mod_type(mod) == TEXT_TYPE);
		} else {
			gui_generic_warning((unknown_parallel + (SWBuf)mod).c_str());
		}
	}

	// frankly, we're faking it here.
	// we have potentially variable v11n among the parallel modules.

	// but we must validate a key in some vaguely consistent manner.
	// arbitrarily, we have picked the 1st.
	// it's consistent, but very possibly wrong for all but the 1st.
	tmpkey = backend_p->get_valid_key(control_name, key);

	cur_book = backend_p->key_get_book(control_name, tmpkey);
	cur_chapter = backend_p->key_get_chapter(control_name, tmpkey);
	cur_verse = backend_p->key_get_verse(control_name, tmpkey);
	settings.intCurVerse = cur_verse;

	// if alternating && bg color is white, substitute a milder grey for harsh black bg.
	if (settings.alternation && !strcmp(settings.bible_bg_color, white))
		bg_choice = grey;	// avoid blaring visual contrast
	else
		bg_choice = settings.bible_text_color;

	for (verse = 1; verse <= xverses; ++verse) {
		snprintf(tmpbuf, 255, "%s %d:%d", cur_book, cur_chapter, verse);
		free(tmpkey);
		tmpkey = backend_p->get_valid_key(control_name, tmpbuf);

		text += "<tr valign=\"top\">";

		// alternate background colors.
		bgColor = (settings.alternation && (verse % 2 == 0))
			? bg_choice
			: settings.bible_bg_color;

		for (modidx = 0; modidx < parallel_count; modidx++) {
			gchar *mod = settings.parallel_list[modidx];
			// might have an abbrev. get the real.
			const char *real_mod = main_abbrev_to_name(mod);
			if (real_mod)
				mod = (gchar *)real_mod;

			if (is_module[modidx]) {

				// mark current verse properly.
				if ((verse == cur_verse) && is_bible_text[modidx])
					textColor = settings.currentverse_color;
				else
					textColor = (settings.alternation && (verse % 2 == 0))
						  ? settings.bible_bg_color
						  : settings.bible_text_color;

				const gchar *newurl = main_url_encode(tmpkey);
				gchar *num = main_format_number(verse);
				snprintf(str, 499,
					 "<td width=\"%d%%\" bgcolor=\"%s\">"
					 "<a name=\"%d\">%s</a>"
					 "<a href=\"passagestudy.jsp?action=showParallel&"
					 "type=verse&value=%s\">"
					 "<font color=\"%s\" size=\"%+d\">%s. </font></a>"
					 "<font face=\"%s\" size=\"%+d\" color=\"%s\">",
					 fraction, bgColor,
					 verse,
					 ((verse == cur_verse) ? "<hr><hr>" : ""),
					 newurl,
					 settings.bible_verse_num_color,
					 settings.verse_num_font_size + settings.base_font_size,
					 num,
					 mf[modidx]->old_font,
					 mf[modidx]->old_font_size_value,
					 textColor);
				g_free((gchar *)newurl);
				g_free(num);
				text += str;

				if (is_rtol[modidx])
					text += "<br/><div align=right>";

				backend_p->set_module_key(mod, tmpkey);
				get_heading(text, backend_p, modidx);

				utf8str = backend_p->get_render_text(mod, tmpkey);
				if (utf8str) {
					text += utf8str;
					g_free(utf8str);
				}

				if (is_rtol[modidx])
					text += "</div>";
			}

			text += "</font></td>";
		}

		text += "</tr>";
	}
	g_free(tmpkey);
	g_free(cur_book);

	/* clear bookkeeping data */
	for (modidx = 0; modidx < parallel_count; ++modidx)
		if (is_module[modidx])
			free_font(mf[modidx]);
	g_free(mf);
	g_free(is_rtol);
	g_free(is_module);
	g_free(is_bible_text);
}
