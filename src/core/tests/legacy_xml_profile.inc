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

// Frozen complete helpers from CrossWire c9976a8; names prefixed for comparison.
static xmlNodePtr frozen_xml_find_section(xmlDocPtr doc, const char *type_doc,
				   const char *section)
{
	xmlNodePtr cur = NULL;

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		fprintf(stderr, _("empty document \n"));
		return NULL;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)type_doc)) {
		fprintf(stderr,
			_("wrong type, root node != %s\n"), type_doc);
		return NULL;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)section)) {
			return cur;
		}
		if (cur->next)
			cur = cur->next;
		else
			break;
	}
	return NULL;
}

static xmlNodePtr frozen_xml_find_prop(xmlDocPtr doc, const char *type_doc,
				const char *section, const char *prop)
{
	xmlNodePtr cur = NULL;
	xmlNodePtr item_cur = NULL;

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		fprintf(stderr, _("empty document \n"));
		return NULL;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)type_doc)) {
		fprintf(stderr,
			_("wrong type, root node != %s\n"), type_doc);
		return NULL;
	}

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)section)) {
			item_cur = cur->xmlChildrenNode;
			while (item_cur != NULL) {
				if (!xmlStrcmp(item_cur->name,
					       (const xmlChar *)prop)) {
					return item_cur;
				}
				if (item_cur->next)
					item_cur = item_cur->next;
				else
					break;
			}
		}
		if (cur->next)
			cur = cur->next;
		else
			break;
	}
	return NULL;
}

char *frozen_xml_get_value(const char *section, const char *item)
{
	xmlNodePtr cur = NULL;
	if ((cur =
		 frozen_xml_find_prop(frozen_xml_settings_doc, "Xiphos", section,
			       item)) != NULL) {
		xmlChar *results =
		    xmlNodeListGetString(frozen_xml_settings_doc,
					 cur->xmlChildrenNode, 1);
		if (results)
			return (char *)results;
	}
	return NULL;
}

void frozen_xml_set_value(const char *type_doc, const char *section,
		   const char *item, const char *value)
{
	xmlNodePtr cur = NULL;
	if ((cur =
		 frozen_xml_find_prop(frozen_xml_settings_doc, type_doc, section,
			       item)) != NULL) {
		xmlNodeSetContent(cur, (const xmlChar *)value);
	}
}

void frozen_xml_add_new_item_to_section(char *section, char *item_name,
				 char *value)
{
	xmlNodePtr section_node = NULL;

	section_node =
	    frozen_xml_find_section(frozen_xml_settings_doc, "Xiphos", section);

	if (section_node)
		xmlNewTextChild(section_node, NULL,
				(const xmlChar *)item_name,
				(const xmlChar *)value);
}

void frozen_xml_add_new_section_to_settings_doc(char *section)
{
	xmlNodePtr root_node = NULL;

	root_node = xmlDocGetRootElement(frozen_xml_settings_doc);
	if (root_node == NULL) {
		fprintf(stderr, _("empty document \n"));
		return;
	}
	(void)xmlNewChild(root_node, NULL,
			  (const xmlChar *)section, NULL);
}
