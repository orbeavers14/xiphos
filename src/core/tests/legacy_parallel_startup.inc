// Frozen from 95bdfbb; complete contiguous parallel block in load_settings_structure.
// This is not the complete settings loader.
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
parallels = xml_get_value("modules", "parallels");
	/* if no parallels are known, convert old fixed parallel set */
	if (!parallels || (*parallels == '\0')) {
		char intN[] = "intN", i, *newhold;

		g_free(parallels); /* in case it was real but empty */
		parallels = g_strdup("");
		for (i = '1'; i <= '5'; ++i) { /* the old set */
			intN[3] = i;
			char *oldparallel = xml_get_value("modules", intN);
			if (oldparallel && *oldparallel) {
				newhold =
				    g_strconcat(parallels, oldparallel,	",", NULL);
				g_free(parallels);
				parallels = newhold;
				g_free(oldparallel);
				xml_remove_node("modules", intN, NULL); /* ? */
			}
		}

		/* trim off the last comma */
		if (*parallels) {
			*(parallels + strlen(parallels) - 1) = '\0';
			xml_set_new_element("modules", "parallels", parallels);
		}
	}
	if (parallels && *parallels)
		settings.parallel_list = g_strsplit(parallels, ",", -1);
	else
		settings.parallel_list = NULL;
	g_free(parallels);
	/* parallel sets */
	settings.parallel_set_names = xml_get_value("modules", "parallel_set_names");
	settings.parallel_set_current = xml_get_value("modules", "parallel_set_current");
	/* load active parallel set at startup */
	if (settings.parallel_set_current && *settings.parallel_set_current) {
		char **set_modules = get_parallel_set(settings.parallel_set_current);
		if (set_modules) {
			g_strfreev(settings.parallel_list);
			settings.parallel_list = set_modules;
		}
	}
	