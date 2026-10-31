// Frozen from b00c37a; complete bookmark XML helper functions.
/*
 * Xiphos Bible Study Tool
 * xml.c -
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

xmlNodePtr xml_add_folder_to_parent(xmlNodePtr parent, gchar *caption)
{
	xmlNodePtr cur_node;

	cur_node = xmlNewChild(parent,
			       NULL, (const xmlChar *)"Folder", NULL);
	(void)xmlNewProp(cur_node,
			 (const xmlChar *)"caption",
			 (const xmlChar *)caption);
	return cur_node;
}

xmlNodePtr xml_add_folder_to_parent_colored(xmlNodePtr parent,
					    gchar *caption,
					    const gchar *color)
{
	xmlNodePtr cur_node = xml_add_folder_to_parent(parent, caption);

	if (color && *color)
		(void)xmlNewProp(cur_node,
				 (const xmlChar *)"color",
				 (const xmlChar *)color);
	return cur_node;
}

gchar *xml_get_folder_color(xmlNodePtr node)
{
	return (gchar *)xmlGetProp(node, (const xmlChar *)"color");
}

void xml_add_bookmark_to_parent(xmlNodePtr parent,
				gchar *caption,
				gchar *key,
				gchar *module, const gchar *mod_desc)
{
	xmlNodePtr xml_node;

	/*if (module) {
	   if (strlen(module) > 2)
	   mod_desc = get_module_description(module);
	   } */

	if (mod_desc == NULL)
		mod_desc = " ";

	xml_node = xmlNewChild(parent,
			       NULL, (const xmlChar *)"Bookmark", NULL);
	(void)xmlNewProp(xml_node,
			 (const xmlChar *)"modulename",
			 (const xmlChar *)module);
	(void)xmlNewProp(xml_node, (const xmlChar *)"key",
			 (const xmlChar *)key);
	(void)xmlNewProp(xml_node, (const xmlChar *)"moduledescription",
			 (const xmlChar *)mod_desc);
	(void)xmlNewProp(xml_node, (const xmlChar *)"description",
			 (const xmlChar *)caption);
	/*xml_attr =
	   xmlNewProp(xml_node, "caption",
	   (const xmlChar *) caption); */
}

void xml_free_bookmark_doc(void)
{
	xmlFreeDoc(bookmark_doc);
}

xmlNodePtr xml_load_bookmark_file(const xmlChar *bookmark_file)
{

	xmlNodePtr cur = NULL;

	bookmark_doc = xmlParseFile((const char *)bookmark_file);

	if (bookmark_doc == NULL) {
		fprintf(stderr, _("Document not parsed successfully. \n"));
		return NULL;
	}

	cur = xmlDocGetRootElement(bookmark_doc);
	if (cur == NULL) {
		fprintf(stderr, _("empty document \n"));
		return NULL;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)"SwordBookmarks")) {
		fprintf(stderr,
			_("wrong type, root node != SwordBookmarks\n"));
		xmlFreeDoc(bookmark_doc);
		return NULL;
	}

	cur = cur->xmlChildrenNode;
	return cur;
}

