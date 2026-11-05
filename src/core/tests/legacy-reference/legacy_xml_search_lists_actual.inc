
char *xml_get_list_from_label(const char *section, const char *item,
			      const char *label)
{
	xmlNodePtr cur = NULL;

	if ((cur =
		 xml_find_section(xml_settings_doc, "Xiphos",
				  section)) != NULL) {

		cur = cur->xmlChildrenNode;

		while (cur != NULL) {

			if (!xmlStrcmp(cur->name, (const xmlChar *)item)) {
				xmlChar *prop_label =
				    xmlGetProp(cur,
					       (const xmlChar *)"label");
				XI_message(("\nprop_label: %s\nlabel: %s",
					    prop_label, label));
				if (!xmlStrcmp(prop_label,
					       (const xmlChar *)label)) {
					return (char *)xmlGetProp(cur,
								  (const xmlChar *)"list");
				}
			}
			if (cur->next)
				cur = cur->next;
			else
				break;
		}
	}
	return NULL;
}

