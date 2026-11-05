
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

