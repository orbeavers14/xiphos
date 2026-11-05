GList *BackEnd::parse_verse_list(const char *module_name, const char *list, char *current_key)
{
	SWModule *mod = get_SWModule(module_name);
	if (!mod || !list || !current_key)
		return NULL;

	// Preserve the legacy caller's current-key mutation; the shared parser
	// works on a clone and never changes that module's key flags.
	mod->getKey()->setText(current_key);
	GList *retlist = NULL;
	try {
		const auto parsed = xiphos::core::parse_reference_list(*mod, list, current_key);
		for (const auto &reference : parsed.references)
			retlist = g_list_append(retlist, strdup(reference.c_str()));
	} catch (...) {
		return NULL;
	}
	return retlist;
}

