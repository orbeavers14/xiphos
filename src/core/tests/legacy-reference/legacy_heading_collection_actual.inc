char *CurrentHeadingBackend::get_entry_attribute(const char *level1,
				   const char *level2,
				   const char *level3,
				   bool render)
{
    const auto value = xiphos::core::entry_attribute(*display_mod, level1, level2, level3, render);
    return value.empty() ? NULL : strdup(value.c_str());
}

void
actual_cache_header(ModuleCache::CacheVerse &cVerse,
	    SWModule &mod,
	    GLOBAL_OPS *ops, CurrentHeadingBackend *be)
{
	int x = 0;
	gchar heading[32];
	const gchar *preverse;
	SWBuf preverse2;
	GString *text = g_string_new("");

	cVerse.SetHeader("");

	sprintf(heading, "%d", x);
	while ((preverse = be->get_entry_attribute("Heading", "Preverse",
						   heading)) != NULL) {
		preverse2 = mod.renderText(preverse);
		g_string_printf(text,
				"%s",
				(((ops->strongs || ops->lemmas) ||
				  ops->morphs)
				     ? block_render(preverse2.c_str())
				     : preverse2.c_str()));
		text = CleanupContent(text, ops, mod.getName(), false);

		cVerse.AppendHeader(text->str);
		g_free((gchar *)preverse);
		++x;
		sprintf(heading, "%d", x);
	}
	g_string_free(text, TRUE);
}

