const char *main_morph(const char *module_name, const char *stype) {
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

 return modbuf;
}
const char *main_strongs(const char *stype) {
	const XiphosLexiconPreferences preferences = {settings.lex_greek, settings.lex_hebrew,
	    settings.morph_greek_lex_nt, settings.morph_greek_lex_ot, settings.morph_heb_lex};
	const gchar *modbuf = xiphos_core_select_lexicon(XIPHOS_LEXICON_MAIN, XIPHOS_LEXICON_STRONGS,
	    stype, &preferences, NULL);

 return modbuf;
}
const char *dialog_morph(DIALOG_DATA *d, const char *type) {
	const XiphosLexiconPreferences preferences = {settings.lex_greek, settings.lex_hebrew,
	    settings.morph_greek_lex_nt, settings.morph_greek_lex_ot, settings.morph_heb_lex};
	struct Context { BackEnd *engine; const char *module; const char *key; };
	Context source = {(BackEnd *)d->backend, d->mod_name, d->key};
	const XiphosLexiconContext context = {&source,
	    [](void *user, const char *module) -> int {
		return static_cast<Context *>(user)->engine->is_module(module);
	    },
	    [](void *user) -> int {
		auto *source = static_cast<Context *>(user);
		return source->engine->get_key_testament(source->module, source->key);
	    }};
	const gchar *modbuf = xiphos_core_select_lexicon(XIPHOS_LEXICON_DIALOG, XIPHOS_LEXICON_MORPHOLOGY,
	    type, &preferences, &context);
	if (!modbuf)
		return 0;

 return modbuf;
}
const char *dialog_strongs(const char *type) {
	const XiphosLexiconPreferences preferences = {settings.lex_greek, settings.lex_hebrew,
	    settings.morph_greek_lex_nt, settings.morph_greek_lex_ot, settings.morph_heb_lex};
	const gchar *modbuf = xiphos_core_select_lexicon(XIPHOS_LEXICON_DIALOG, XIPHOS_LEXICON_STRONGS,
	    type, &preferences, NULL);

 return modbuf;
}
