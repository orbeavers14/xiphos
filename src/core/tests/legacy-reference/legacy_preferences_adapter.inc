GLOBAL_OPS *main_new_globals(const gchar *mod_name)
{
	GLOBAL_OPS *ops;
	gchar *buf = g_strdup_printf("%s/modops.conf", settings.gSwordDir);
	SWConfig module_options(buf);

	module_options.load();
	g_free(buf);

	ops = g_new0(GLOBAL_OPS, 1);
	xiphos::core::EngineOptionPreferences preferences;
	for (const auto &item : module_options[mod_name])
		preferences.emplace(item.first.c_str(), item.second.c_str());
	const auto values = xiphos::core::engine_options_from_preferences(preferences);
	const auto presentation = xiphos::core::presentation_options_from_preferences(preferences, false);
	ops->strongs = values.strongs;
	ops->morphs = values.morphs;
	ops->lemmas = values.lemmas;
	ops->transliteration = values.transliteration;
	ops->italic_headings = values.italic_headings;
	ops->xlit = values.xlit;
	ops->enumerated = values.enumerated;
	ops->morphseg = values.morphseg;

	ops->variants_all = values.variants_all;
	ops->variants_primary = values.variants_primary;
	ops->variants_secondary = values.variants_secondary;

	// special case options, default on:
	ops->words_in_red = values.words_in_red;
	ops->headings = presentation.headings;
	ops->footnotes = values.footnotes;
	ops->scripturerefs = values.scripturerefs;
	ops->xrefnotenumbers = xiphos::core::note_numbers_from_preferences(preferences);
	ops->display_chapter_N =
	    (module_options[mod_name]["Display Chapter N"] != "Off");

	// more special case default on: heb/grk support.
	ops->greekaccents = values.greekaccents;
	ops->hebrewpoints = values.hebrewpoints;
	ops->hebrewcant = values.hebrewcant;

	// more special case default on: japanese glosses.
	ops->glosses = values.glosses;

	return ops;
}
