const char *BackEnd::get_osisref_from_key(const char *module, const char *key)
{
	ModMap::iterator it;

	// we need an OSISRef in module-sensitive context.
	// fallback positions: we hope for a supplied module.
	// if not, assume KJV (v11n historical reasons; not anglocentric).
	// if not, use the 1st module...by which time we're just guessing.

	it = main_mgr->Modules.find((module && *module) ? module : "KJV");
	if (it == main_mgr->Modules.end()) {
		it = main_mgr->Modules.begin();
		if (it == main_mgr->Modules.end())
			return strdup("");
	}

	SWModule *mod = it->second;
	VerseKey *vkey = (VerseKey *)(SWKey *)(*mod);
	vkey->setText(key);
	return strdup(vkey->getOSISRef());
}
