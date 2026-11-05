char *BackEnd::get_valid_key(const char *module_name, const char *key)
{
	SWModule *mod = get_SWModule(module_name);
	if (!mod)
		return strdup("Gen.1.1");

	VerseKey *vkey = (VerseKey *)(SWKey *)(*mod);
	int t = module_type(module_name);
	return strdup(xiphos::core::navigation_key_text(*vkey, key,
	    (t == TEXT_TYPE) || (t == COMMENTARY_TYPE), sword_locale).c_str());
}
