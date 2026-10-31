// Frozen from 5e45425; complete named parallel storage/name helpers.
/*
 * Xiphos Bible Study Tool
 * settings.c - Guess what?
 *
 * Copyright (C) 2000-2026 Xiphos Developer Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */
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
