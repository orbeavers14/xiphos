
char **get_parallel_set(const gchar *name)
{
	char *key, *value;

	if (!name || !*name)
		return NULL;

	key = g_strdup_printf("parallel_set_%s", name);
	value = xml_get_value("modules", key);
	g_free(key);

	if (!value || !*value) {
		g_free(value);
		return NULL;
	}

	char **result = g_strsplit(value, ",", -1);
	g_free(value);
	return result;
}

void save_parallel_set(const gchar *name, gchar **modules)
{
	char *key, *value;

	if (!name || !*name || !modules)
		return;

	key = g_strdup_printf("parallel_set_%s", name);
	value = g_strjoinv(",", modules);
	xml_set_or_create_value("modules", key, value);
	g_free(key);
	g_free(value);
}

char *name_to_key(const char *name)
{
	char *key = g_strdup(name);
	for (char *p = key; *p; ++p)
		if (*p == ' ')
			*p = '_';
	return key;
}

char *key_to_name(const char *key)
{
	char *name = g_strdup(key);
	for (char *p = name; *p; ++p)
		if (*p == '_')
			*p = ' ';
	return name;
}
