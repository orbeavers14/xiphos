// Frozen from 23aba44; complete replacement table/cache fill/check functions.
/*
 * Xiphos Bible Study Tool
 * display.cc -
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

#define NUM_REPLACE 6

struct replace
{
	gchar c;
	gchar *s;
} replacement[NUM_REPLACE] = {
      // & must be first to avoid double-encoding
      {'&', (gchar *)"&amp;"},
      {'<', (gchar *)"&lt;"},
      {'>', (gchar *)"&gt;"},
      {'\n', (gchar *)"<br />"},
      {'"', (gchar *)"&quot;"},
      {'\'', (gchar *)"&apos;"},
};


void
markedCacheFill(const gchar *modname, gchar *key)
{
	gchar *s, *t, *err, *mhold;
	char *key_book = g_strdup(main_get_osisref_from_key((const char *)modname,
							    (const char *)key));
	// free the old cache.  first free contents, then the map itself.
	MC::iterator it;
	for (it = marked_cache.begin();
	     it != marked_cache.end();
	     ++it) {
		marked_element *e = (*it).second;
		g_free(e->module);
		g_string_free(e->annotation, TRUE);
		delete e;
	}
	marked_cache.clear();

	// tear apart the key (e.g. "Gen.1.1") just to get the book.
	*(s = strrchr(key_book, '.')) = '\0';
	*(t = strrchr(key_book, '.')) = '\0';

	// remember exactly what chapter this cache is for
	g_free(marked_cache_modname);
	g_free(marked_cache_book);
	marked_cache_modname = g_strdup(modname);
	marked_cache_book = g_strdup(key_book);

	// load up the annotation content
	if (xml_set_section_ptr("osisrefmarkedverses") && xml_get_label()) {
		marked_element *e = new marked_element;
		do {
			e->module = xml_get_label();
			s = xml_get_list();
			e->annotation = g_string_new(s);
			g_free(s);

			gchar *m = e->module;
			mhold = g_strdup(m);

			// tear apart "NASB Gen.1.1"
			if ((s = strrchr(m, '.')) == NULL) // rightmost dot
				goto fail;
			*s = '\0';
			if ((t = strrchr(m, '.')) == NULL) // .chapter.verse
				goto fail;
			*t = '\0';

			e->chapter_verse = (1000 * atoi(t + 1)) + atoi(s + 1);
			if ((s = strchr(m, ' ')) == NULL) // leftmost space
				goto fail;
			*(s++) = '\0';
			e->book = s;
			// now properly delimited: module & book, plus numeric c:v.

			g_free(mhold);

			// for fast reference: is this annotation relevant?
			if ((*m && (strcasecmp(m, modname) != 0)) ||
			    (strcasecmp(e->book, key_book) != 0)) {
				// junk: re-use same element in next loop.
				g_free(e->module);
				g_string_free(e->annotation, TRUE);
			} else {
				// replace embedded badness characters.
				for (int i = 0; i < NUM_REPLACE; ++i) {
					for (s = strchr(e->annotation->str, replacement[i].c);
					     s;
					     s = strchr(s + 1, replacement[i].c)) {
						(void)g_string_erase(e->annotation,
								     s - (e->annotation->str), 1);
						(void)g_string_insert(e->annotation,
								      s - (e->annotation->str),
								      replacement[i].s);
					}
				}
				// valid: insert + get fresh one to work with.
				marked_cache[e->chapter_verse] = e;
				e = new marked_element;
			}
		} while (xml_next_item() && xml_get_label());
		// remove extra element that we necessarily have at loop's end.
		delete e;
	}
	g_free(key_book);
	return;

fail:
	err = g_strdup_printf(_("Improperly encoded personal annotation label:\n'%s'"),
			      mhold);
	gui_generic_warning(err);
	g_free(err);
	g_free(key_book);
	return;
}

marked_element *
markedCacheCheck(int thisChapterVerse)
{
	MC::iterator it = marked_cache.find(thisChapterVerse);
	if (it != marked_cache.end())
		return (*it).second;
	return NULL;
}

