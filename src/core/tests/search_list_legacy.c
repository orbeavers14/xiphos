/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Compile the retained C writer without translating its identifiers or logic. */
#include <libxml/parser.h>
#include <stdio.h>
#include <string.h>
#define _(value) (value)
static xmlDocPtr xml_settings_doc;
#include "legacy_xml_profile_actual.inc"
#include "legacy_xml_search_list_save_actual.inc"
char *legacy_search_list_edit(const char *source, int kind, const char *label, const char *value)
{
	xml_settings_doc = xmlReadMemory(source, (int)strlen(source), NULL, NULL, XML_PARSE_NONET);
	if (!xml_settings_doc)
		return NULL;
	xml_set_list_item(kind ? "ranges" : "modlists", kind ? "range" : "modlist", label, value);
	xmlChar *result = NULL;
	int length = 0;
	xmlDocDumpFormatMemoryEnc(xml_settings_doc, &result, &length, "UTF-8", 0);
	xmlFreeDoc(xml_settings_doc);
	xml_settings_doc = NULL;
	return (char *)result;
}
