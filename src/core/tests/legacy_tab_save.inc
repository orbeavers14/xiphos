// Frozen from 14e1829; complete gui_save_tabs, explicit existing-file host.
/*
 * Xiphos Bible Study Tool
 * tabbed_browser.c - functions to facilitate tabbed browsing of different passages at once
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

void gui_save_tabs(const gchar *filename)
{
	xmlDocPtr xml_doc;
	xmlNodePtr root_node;
	xmlNodePtr cur_node;
	xmlNodePtr section_node;
	gchar *file;
	GList *tmp = NULL;

	if (NULL == filename)
		filename = default_tab_filename;

	if (g_access(filename, F_OK) == 0) {
		/* we're done, just copy for local use that can be free'd */
		file = g_strdup(filename);
	} else {
		gchar *tabs_dir = g_strdup_printf("%s/tabs/", settings.gSwordDir);
		if (g_access(tabs_dir, F_OK) == -1) {
			if ((g_mkdir(tabs_dir, S_IRWXU)) == -1) {
				gui_generic_warning_modal(_("Can't create tabs dir."));
				return;
			}
		}
		file = g_strdup_printf("%s%s", tabs_dir, filename);
		g_free(tabs_dir);
	}

	xml_doc = xmlNewDoc((const xmlChar *)"1.0");

	if (xml_doc == NULL) {
		gui_generic_warning_modal("Tabs document not created successfully.");
		return;
	}

	root_node = xmlNewNode(NULL, (const xmlChar *)"Xiphos_Tabs");
	//xml_attr =
	xmlNewProp(root_node, (const xmlChar *)"Version",
		   (const xmlChar *)VERSION);
	xmlDocSetRootElement(xml_doc, root_node);

	section_node = xmlNewChild(root_node, NULL,
				   (const xmlChar *)"tabs", NULL);

	for (tmp = g_list_first(passage_list); tmp != NULL;
	     tmp = g_list_next(tmp)) {

		PASSAGE_TAB_INFO *pt = (PASSAGE_TAB_INFO *)tmp->data;

		cur_node = xmlNewChild(section_node,
				       NULL, (const xmlChar *)"tab",
				       NULL);
		xmlNewProp(cur_node, (const xmlChar *)"text_mod",
			   (const xmlChar *)pt->text_mod);
		xmlNewProp(cur_node, (const xmlChar *)"commentary_mod",
			   (const xmlChar *)pt->commentary_mod);
		xmlNewProp(cur_node, (const xmlChar *)"dictlex_mod",
			   (const xmlChar *)pt->dictlex_mod);
		xmlNewProp(cur_node, (const xmlChar *)"book_mod",
			   (const xmlChar *)pt->book_mod);
		xmlNewProp(cur_node,
			   (const xmlChar *)"text_commentary_key",
			   (const xmlChar *)pt->text_commentary_key);
		xmlNewProp(cur_node, (const xmlChar *)"dictlex_key",
			   (const xmlChar *)pt->dictlex_key);
		xmlNewProp(cur_node, (const xmlChar *)"book_offset",
			   (const xmlChar *)pt->book_offset);
		xmlNewProp(cur_node, (const xmlChar *)"comm_showing",
			   (const xmlChar *)
			   true_false2yes_no(pt->comm_showing));
		xmlNewProp(cur_node, (const xmlChar *)"showtexts",
			   (const xmlChar *)
			   true_false2yes_no(pt->showtexts));
		xmlNewProp(cur_node, (const xmlChar *)"showpreview",
			   (const xmlChar *)
			   true_false2yes_no(pt->showpreview));
		xmlNewProp(cur_node, (const xmlChar *)"showcomms",
			   (const xmlChar *)
			   true_false2yes_no(pt->showcomms));
		xmlNewProp(cur_node, (const xmlChar *)"showdicts",
			   (const xmlChar *)
			   true_false2yes_no(pt->showdicts));
		xmlNewProp(cur_node, (const xmlChar *)"showparallel",
			   (const xmlChar *)
			   true_false2yes_no(pt->showparallel));
	}
	xmlSaveFormatFile(file, xml_doc, 1);
	g_free(file);
	xmlFreeDoc(xml_doc);
}

