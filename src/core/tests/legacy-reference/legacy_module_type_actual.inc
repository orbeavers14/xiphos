int CurrentBackend::module_type(const char *mod_name)
{
	//	if ((!mod_name) || (strlen(mod_name) < 2))

	SWModule *mod = get_SWModule(mod_name);

	if (mod) {

		if (!strcmp(mod->getType(), TEXT_MODS)) {
			return TEXT_TYPE;
		}

		if (!strcmp(mod->getType(), COMM_MODS)) {
			if (!strcmp((char *)mod->getConfigEntry("ModDrv"), "RawFiles"))
				return PERCOM_TYPE;
			return COMMENTARY_TYPE;
		}

		if (!strcmp(mod->getType(), DICT_MODS)) {
			return DICTIONARY_TYPE;
		}

		if (!strcmp(mod->getType(), BOOK_MODS)) {
			if (mod->getConfigEntry("GSType") &&
			    !strcmp((char *)mod->getConfigEntry("GSType"), "PrayerList"))
				return PRAYERLIST_TYPE;
			return BOOK_TYPE;
		}
	}
	return -1;
}

