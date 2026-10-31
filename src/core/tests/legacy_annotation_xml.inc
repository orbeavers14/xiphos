// Frozen from 23aba44; complete section cursor/getter helpers.
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

static xmlNodePtr xml_find_section(xmlDocPtr doc, const char *type_doc,
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

int xml_set_section_ptr(const char *section)
{
	xmlNodePtr cur = NULL;

	if ((cur =
		 xml_find_section(xml_settings_doc, "Xiphos",
				  section)) != NULL) {
		section_ptr = cur->xmlChildrenNode;
		if (section_ptr)
			return 1;
	}
	return 0;
}

int xml_next_item(void)
{
	while (section_ptr != NULL) {
		section_ptr = section_ptr->next;
		return 1;
	}
	return 0;
}

char *xml_get_label(void)
{
	return (char *)xmlGetProp(section_ptr, (const xmlChar *)"label");
}

char *xml_get_list(void)
{
	return (char *)xmlGetProp(section_ptr, (const xmlChar *)"list");
}

