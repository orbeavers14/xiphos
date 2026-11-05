static const char *stylefile =
    "style.css"; // default name, per module.
static const char *default_stylefile =
    "default-style.css"; // default name, overall.

const gchar *get_css_references(SWModule &module)
{
	static string css; // static -> safe to return it

	// assume nothing will be available.
	css = "";

	// non-specific CSS for all module displays.
	char *css_file = g_build_filename(settings.gSwordDir,
					  default_stylefile, NULL);

	if (g_file_test(css_file, G_FILE_TEST_EXISTS)) {
		css += (string) "<link rel=\"stylesheet\" type=\"text/css\" href=\""
#ifdef WIN32
		       + "http://127.0.0.1:7878/" // see main.c (sob)
#else
		       + "file:"
#endif
		       + css_file + "\" />";
	}
	g_free(css_file);

	// construct path to module's CSS.
	char *datapath = main_get_mod_config_entry(module.getName(), "AbsoluteDataPath");
	char *prefcss = main_get_mod_config_entry(module.getName(), "PreferredCSSXHTML");

	// module-specific CSS.
	css_file = g_build_filename(datapath, (prefcss ? prefcss : stylefile), NULL);

	if (g_file_test(css_file, G_FILE_TEST_EXISTS)) {
		css += (string) "<link rel=\"stylesheet\" type=\"text/css\" href=\""
#ifdef WIN32
		       + "http://127.0.0.1:7878/" // see main.c (sob)
#else
		       + "file:"
#endif
		       + css_file + "\" />";
	}
	g_free(css_file); // get rid of old one first.

	return css.c_str();
}

//
