static gint show_morph(const char *module_name,
		       const gchar *stype,
		       const gchar *svalue,
		       gboolean clicked)
{
	const XiphosLexiconPreferences preferences = {settings.lex_greek, settings.lex_hebrew,
	    settings.morph_greek_lex_nt, settings.morph_greek_lex_ot, settings.morph_heb_lex};
	struct Context { BackEnd *engine; const char *module; const char *key; };
	Context source = {backend, module_name, settings.currentverse};
	const XiphosLexiconContext context = {&source,
	    [](void *user, const char *module) -> int {
		return static_cast<Context *>(user)->engine->is_module(module);
	    },
	    [](void *user) -> int {
		auto *source = static_cast<Context *>(user);
		return source->engine->get_key_testament(source->module, source->key);
	    }};
	const gchar *modbuf = xiphos_core_select_lexicon(XIPHOS_LEXICON_MAIN, XIPHOS_LEXICON_MORPHOLOGY,
	    stype, &preferences, &context);

	if (clicked) {
		main_display_dictionary(modbuf, (gchar *)svalue);
	} else {
		gchar *mybuf = main_get_rendered_text(modbuf, (gchar *)svalue);
		if (mybuf) {
			main_information_viewer(modbuf,
						mybuf,
						(gchar *)svalue,
						"showMorph",
						(gchar *)stype,
						NULL,
						NULL);
			g_free(mybuf);
		}
	}
	return 1;
}

static gint show_strongs(const gchar *stype, const gchar *svalue,
			 gboolean clicked)
{
	const XiphosLexiconPreferences preferences = {settings.lex_greek, settings.lex_hebrew,
	    settings.morph_greek_lex_nt, settings.morph_greek_lex_ot, settings.morph_heb_lex};
	const gchar *modbuf = xiphos_core_select_lexicon(XIPHOS_LEXICON_MAIN, XIPHOS_LEXICON_STRONGS,
	    stype, &preferences, NULL);

	if (clicked) {
		main_display_dictionary(modbuf, (gchar *)svalue);
	} else {
		gchar *mybuf = main_get_rendered_text(modbuf, (gchar *)svalue);
		if (mybuf) {
			main_information_viewer(
			    modbuf,
			    mybuf,
			    (gchar *)svalue,
			    "showStrongs",
			    (gchar *)stype,
			    NULL,
			    NULL);
			g_free(mybuf);
		}
	}

	return 1;
}

