// Frozen from b589779; complete first-item XML save/remove helpers.
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

static xmlNodePtr xml_find_item(xmlNodePtr parent, const char *item,
				const char *label)
{
	xmlNodePtr cur = NULL;
	xmlChar *prop_label = NULL;

	cur = parent->xmlChildrenNode;

	while (cur != NULL) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)item)) {
			/* no label means any (i.e. first) element matches */
			if (!label)
				return cur;

			/* otherwise we must find the label match. */
			prop_label =
			    xmlGetProp(cur, (const xmlChar *)"label");
			if (!xmlStrcmp(prop_label, (const xmlChar *)label)) {
				return cur;
			}
		}
		if (cur->next)
			cur = cur->next;
		else
			break;
	}
	return NULL;
}

void xml_set_list_item(const char *section, const char *item,
		       const char *label, const char *value)
{
	xmlNodePtr cur = NULL;
	xmlNodePtr cur_item = NULL;
	xmlNodePtr new = NULL;

	if ((cur =
		 xml_find_section(xml_settings_doc, "Xiphos",
				  section)) == NULL) {
		xmlNodePtr root_node =
		    xmlDocGetRootElement(xml_settings_doc);
		cur =
		    xmlNewChild(root_node, NULL, (const xmlChar *)section,
				NULL);
	}

	if (cur) {
		if ((cur_item = xml_find_item(cur, item, label))) {
			xmlSetProp(cur_item, (const xmlChar *)"list",
				   (const xmlChar *)value);
		} else {
			new =
			    xmlNewChild(cur, NULL, (const xmlChar *)item,
					NULL);
			xmlNewProp(new, (const xmlChar *)"label",
				   (const xmlChar *)label);
			xmlNewProp(new, (const xmlChar *)"list",
				   (const xmlChar *)value);
		}
	}
}

void xml_remove_node(const char *section, const char *item,
		     const char *label)
{
	xmlNodePtr cur = NULL;
	xmlNodePtr cur_item = NULL;
	if ((cur =
		 xml_find_section(xml_settings_doc, "Xiphos",
				  section)) != NULL) {
		if ((cur_item = xml_find_item(cur, item, label)) != NULL) {
			xmlUnlinkNode(cur_item);
			xmlFreeNode(cur_item);
		}
	}
}

