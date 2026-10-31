// Frozen from b00c37a; complete XML/tree getter, insert and recursive load functions.
/*
 * Xiphos Bible Study Tool
 * bookmarks_treeview.c - gui for bookmarks using treeview
 *
 * Copyright (C) 2003-2026 Xiphos Developer Team
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

static void get_xml_folder_data(xmlNodePtr cur, BOOKMARK_DATA *data)
{
	xmlChar *folder;
	gchar *color;

	folder = xmlGetProp(cur, (const xmlChar *)"caption");
	data->caption = g_strdup((char *)folder);
	data->key = NULL;
	data->module = NULL;
	data->module_desc = NULL;
	data->description = NULL;
	data->is_leaf = FALSE;
	data->opened = bm_pixbufs->pixbuf_opened;
	data->closed = bm_pixbufs->pixbuf_closed;

	/* color is optional — NULL when the folder has no tag color assigned */
	color = xml_get_folder_color(cur);
	data->color = color ? g_strdup(color) : NULL;
	if (color)
		xmlFree((xmlChar *)color);
}

static void get_xml_bookmark_data(xmlNodePtr cur, BOOKMARK_DATA *data)
{
	xmlChar *mod1;
	xmlChar *key;
	xmlChar *caption;
	xmlChar *mod_desc;
	xmlChar *description;
	gchar buf[500];

	data->opened = bm_pixbufs->pixbuf_helpdoc;
	data->closed = NULL;
	mod1 = xmlGetProp(cur, (const xmlChar *)"modulename");
	key = xmlGetProp(cur, (const xmlChar *)"key");
	mod_desc = xmlGetProp(cur, (const xmlChar *)"moduledescription");
	description = xmlGetProp(cur, (const xmlChar *)"description");
	caption = xmlGetProp(cur, (const xmlChar *)"description");
	if ((char *)caption) {
		if (strlen((char *)caption) > 0) {
			data->caption = g_strdup((char *)caption);
		} else {
			sprintf(buf, "%s, %s", key, mod1);
			data->caption = g_strdup((char *)buf);
		}
	} else
		data->caption = g_strdup((char *)key);
	data->key = g_strdup((char *)key);
	data->module = g_strdup((char *)mod1);
	data->description = g_strdup((char *)description);
	data->module_desc = g_strdup((char *)mod_desc);
	data->is_leaf = TRUE;
	data->color = NULL;  /* leaves (bookmarks) never carry a color */
}

void gui_add_item_to_tree(GtkTreeIter *iter, GtkTreeIter *parent,
			  BOOKMARK_DATA *data)
{
	gtk_tree_store_append(GTK_TREE_STORE(model), iter, parent);

	gtk_tree_store_set(GTK_TREE_STORE(model), iter,
			   COL_OPEN_PIXBUF, data->opened,
			   COL_CLOSED_PIXBUF, data->closed,
			   COL_CAPTION, data->caption,
			   COL_KEY, data->key,
			   COL_MODULE, data->module,
			   COL_MODULE_DESC, data->module_desc,
			   COL_DESCRIPTION, data->description,
			   COL_COLOR, data->color,
			   -1);
}

static void add_node(xmlNodePtr cur, GtkTreeIter *parent)
{
	GtkTreeIter iter;
	BOOKMARK_DATA *p = NULL;
	xmlNodePtr work = NULL;

	for (work = cur->xmlChildrenNode; work; work = work->next) {
		if (!xmlStrcmp(work->name, (const xmlChar *)"Bookmark")) {
			p = g_new(BOOKMARK_DATA, 1);
			get_xml_bookmark_data(work, p);
			gui_add_item_to_tree(&iter, parent, p);
		} else if (!xmlStrcmp(work->name, (const xmlChar *)"Folder")) {
			p = g_new(BOOKMARK_DATA, 1);
			get_xml_folder_data(work, p);
			gui_add_item_to_tree(&iter, parent, p);
			add_node(work, &iter);
		}
	}
}

void gui_parse_bookmarks(GtkTreeView *tree, const xmlChar *file,
			 GtkTreeIter *parent)
{
	xmlNodePtr cur = NULL;
	BOOKMARK_DATA *p = NULL;
	GtkTreeIter iter;
	GtkTreePath *path;

	cur = xml_load_bookmark_file(file);

	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)"Bookmark")) {
			p = g_new(BOOKMARK_DATA, 1);
			get_xml_bookmark_data(cur, p);
			gui_add_item_to_tree(&iter, parent, p);
		} else {
			p = g_new(BOOKMARK_DATA, 1);
			get_xml_folder_data(cur, p);
			if (p->caption) {
				gui_add_item_to_tree(&iter, parent, p);
			}
			add_node(cur, &iter);
		}
		cur = cur->next;
	}

	xml_free_bookmark_doc();
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), parent);
	gtk_tree_view_expand_to_path(tree, path);
	gtk_tree_path_free(path);
}

