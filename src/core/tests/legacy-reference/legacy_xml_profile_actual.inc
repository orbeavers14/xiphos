
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


static xmlNodePtr xml_find_prop(xmlDocPtr doc, const char *type_doc,
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


char *xml_get_value(const char *section, const char *item)
{
	xmlNodePtr cur = NULL;
	if ((cur =
		 xml_find_prop(xml_settings_doc, "Xiphos", section,
			       item)) != NULL) {
		xmlChar *results =
		    xmlNodeListGetString(xml_settings_doc,
					 cur->xmlChildrenNode, 1);
		if (results)
			return (char *)results;
	}
	return NULL;
}


void xml_set_value(const char *type_doc, const char *section,
		   const char *item, const char *value)
{
	xmlNodePtr cur = NULL;
	if ((cur =
		 xml_find_prop(xml_settings_doc, type_doc, section,
			       item)) != NULL) {
		xmlNodeSetContent(cur, (const xmlChar *)value);
	}
}


void xml_add_new_item_to_section(char *section, char *item_name,
				 char *value)
{
	xmlNodePtr section_node = NULL;

	section_node =
	    xml_find_section(xml_settings_doc, "Xiphos", section);

	if (section_node)
		xmlNewTextChild(section_node, NULL,
				(const xmlChar *)item_name,
				(const xmlChar *)value);
}


void xml_add_new_section_to_settings_doc(char *section)
{
	xmlNodePtr root_node = NULL;

	root_node = xmlDocGetRootElement(xml_settings_doc);
	if (root_node == NULL) {
		fprintf(stderr, _("empty document \n"));
		return;
	}
	(void)xmlNewChild(root_node, NULL,
			  (const xmlChar *)section, NULL);
}


void xml_set_or_create_value(const char *section, const char *item,
			     const char *value)
{
	xmlNodePtr cur =
	    xml_find_prop(xml_settings_doc, "Xiphos", section, item);
	if (cur)
		xmlNodeSetContent(cur, (const xmlChar *)value);
	else
		xml_add_new_item_to_section((char *)section,
					    (char *)item,
					    (char *)value);
}


void xml_set_new_element(const char *section, const char *item,
			 const char *content)
{
	xmlNodePtr cur = NULL;

	if ((cur =
		 xml_find_section(xml_settings_doc, "Xiphos",
				  section)) == NULL) {
		xmlNodePtr root_node =
		    xmlDocGetRootElement(xml_settings_doc);
		cur =
		    xmlNewChild(root_node, NULL, (const xmlChar *)section,
				NULL);
	}

	if (cur)
		(void)xmlNewChild(cur, NULL, (const xmlChar *)item,
				  (const xmlChar *)content);
}

