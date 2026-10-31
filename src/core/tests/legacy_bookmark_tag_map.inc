// Frozen from 7102d57; complete foreground, clear/build and lookup functions.
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

static const char *text_color_for_bg(const gchar *hex_color)
{
	if (!hex_color || hex_color[0] != '#' || strlen(hex_color) < 7)
		return "#000000";
	int r = 0, g = 0, b = 0;
	sscanf(hex_color + 1, "%02x%02x%02x", &r, &g, &b);
	/* relative luminance (ITU-R BT.709) */
	double lum = 0.2126 * r + 0.7152 * g + 0.0722 * b;
	return (lum < 128.0) ? "#FFFFFF" : "#000000";
}
static void free_tag_color_map(void)
{
	if (tag_color_map) {
		g_hash_table_destroy(tag_color_map);
		tag_color_map = NULL;
	}
}
static void build_tag_color_map(VerseKey *vk)
{
	free_tag_color_map();
	tag_color_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	extern GtkTreeStore *model;
	if (!settings.tag_colorize || !model || !vk)
		return;

	GtkTreeIter root;
	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &root))
		return;

	/* Resolve everything against a saved copy of the key's text, and
	 * restore it when done, so the live render position is never
	 * left disturbed once building the map is finished. */
	gchar *saved_pos = g_strdup((const char *)vk->getText());

	GQueue *stack = g_queue_new();
	GQueue *colors = g_queue_new();
	GtkTreeIter child;
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(model), &child, &root)) {
		do {
			GtkTreeIter *copy = g_new(GtkTreeIter, 1);
			*copy = child;
			g_queue_push_tail(stack, copy);
			g_queue_push_tail(colors, NULL);
		} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &child));
	}

	while (!g_queue_is_empty(stack)) {
		GtkTreeIter *iter = (GtkTreeIter *)g_queue_pop_head(stack);
		gchar *inherited = (gchar *)g_queue_pop_head(colors);
		gchar *node_color = NULL, *node_key = NULL, *node_module = NULL, *node_label = NULL;
		gtk_tree_model_get(GTK_TREE_MODEL(model), iter,
				   COL_COLOR,       &node_color,
				   COL_KEY,         &node_key,
				   COL_MODULE,      &node_module,
				   COL_DESCRIPTION, &node_label,
				   -1);
		gchar *escaped_label = (node_label
					? g_uri_escape_string(node_label, NULL, TRUE)
					: NULL);

		const gchar *effective = (node_color && *node_color)
			? node_color : inherited;
		if (node_key) {
			/* Let Sword resolve the (possibly multi-reference, possibly
			 * partial) bookmark key into individual, fully-qualified
			 * verses -- same mechanism used for the verse-list popup.
			 * Only do this for bookmarks that actually target Bible
			 * text (an empty module means "applies to all Bible
			 * modules", the long-standing convention here) or a
			 * commentary (also verse-keyed). Bookmarks pointing at a
			 * dictionary or genbook module (e.g. Josephus section
			 * numbers, Strong's numbers) have keys that are NOT verse
			 * references at all; feeding them to Sword's verse-list
			 * parser doesn't fail cleanly -- it silently reinterprets
			 * the bare number/text as some arbitrary chapter:verse,
			 * producing spurious highlighted verses elsewhere in the
			 * Bible (reported: stray highlights in Genesis 1). */
			gboolean is_scripture_key = TRUE;
			if (node_module && *node_module) {
				int mtype = backend->module_type(node_module);
				if ((mtype != TEXT_TYPE) && (mtype != COMMENTARY_TYPE))
					is_scripture_key = FALSE;
			}
			if (effective && is_scripture_key) {
				// footnote elimination, like "Song.2.4#n17".
				// remove thml (#) and osis (!) subverse indicators.
				// must do this before Sword parse gets hold of it.
				gchar *punc;
				while ((punc = strpbrk(node_key, "#!"))) {
					*(punc++) = ' ';
					while ((punc = strpbrk(punc, "n0123456789"))) {
						*(punc++) = ' ';
					}
				}

				GList *verses = backend->parse_verse_list(
					settings.MainWindowModule, node_key,
					(char *)settings.currentverse);
				for (GList *l = verses; l; l = l->next) {
					VerseKey vk2;
					vk2.setLocale(vk->getLocale());
					vk2.setText((const char *)l->data);
					gchar *ref2 = g_strdup_printf("%s.%d.%d",
						vk2.getOSISBookName(),
						vk2.getChapter(),
						vk2.getVerse());

					/* first color wins for display of a given verse
					 * but all other occurrences will be appended. */
					const char *tc_fg = text_color_for_bg(effective);
					gchar *element = (gchar *)g_hash_table_lookup(tag_color_map, ref2);
					gchar *str = g_strconcat((element ? element : ""),
								 effective,
								 "-",		// arbitrary separators.
								 tc_fg,
								 "-",
								 (escaped_label ? escaped_label : ""),
								 "@:@:@",	// magic string, see main_info_viewer().
								 NULL);
					// insertion may supercede previous instance, freeing it.
					// ownership of newly-allocated str is given to the map.
					// (parse_verse_list() itself now expands hyphenated
					// ranges like "15:1-4" into one list element per verse,
					// so no further manual range expansion is needed here
					// -- doing so used to double up entries for every verse
					// after the first one in a range.)
					g_hash_table_insert(tag_color_map, ref2, str);
				}
				for (GList *l = verses; l; l = l->next)
					g_free(l->data);
				g_list_free(verses);
			}
			g_free(node_key);
		} else {
			if (gtk_tree_model_iter_children(GTK_TREE_MODEL(model),
							 &child, iter)) {
				do {
					GtkTreeIter *copy = g_new(GtkTreeIter, 1);
					*copy = child;
					g_queue_push_tail(stack, copy);
					g_queue_push_tail(colors,
						effective ? g_strdup(effective) : NULL);
				} while (gtk_tree_model_iter_next(
						GTK_TREE_MODEL(model), &child));
			}
		}
		g_free(escaped_label);
		g_free(node_label);
		g_free(node_color);
		g_free(node_module);
		g_free(inherited);
		g_free(iter);
	}
	g_queue_free_full(stack, g_free);
	g_queue_free_full(colors, g_free);

	vk->setText(saved_pos);
	g_free(saved_pos);
}
static gchar *get_tag_color_for_versekey(VerseKey *vk)
{
	if (!settings.tag_colorize || !tag_color_map || !vk)
		return NULL;
	gchar *osisref = g_strdup_printf("%s.%d.%d",
		vk->getOSISBookName(),
		vk->getChapter(),
		vk->getVerse());
	gchar *color = (gchar *)g_hash_table_lookup(tag_color_map, osisref);
	g_free(osisref);
	return color ? g_strdup(color) : NULL;
}
