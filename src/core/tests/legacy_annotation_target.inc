// Frozen from f6d9bf4; complete get_osisref_from_key on safe exact verse modules.
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
const char *BackEnd::get_osisref_from_key(const char *module, const char *key)
{
	ModMap::iterator it;

	// we need an OSISRef in module-sensitive context.
	// fallback positions: we hope for a supplied module.
	// if not, assume KJV (v11n historical reasons; not anglocentric).
	// if not, use the 1st module...by which time we're just guessing.

	it = main_mgr->Modules.find((module && *module) ? module : "KJV");
	if (it == main_mgr->Modules.end()) {
		it = main_mgr->Modules.begin();
		if (it == main_mgr->Modules.end())
			return strdup("");
	}

	SWModule *mod = it->second;
	VerseKey *vkey = (VerseKey *)(SWKey *)(*mod);
	vkey->setText(key);
	return strdup(vkey->getOSISRef());
}

