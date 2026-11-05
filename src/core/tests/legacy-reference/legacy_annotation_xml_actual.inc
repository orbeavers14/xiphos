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

