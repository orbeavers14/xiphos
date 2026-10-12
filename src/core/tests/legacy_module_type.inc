/*
 * Xiphos Bible Study Tool
 * sword_main.cc -
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

// Frozen complete module_type function from CrossWire c9976a8.
int FrozenBackend::module_type(const char *mod_name)
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

