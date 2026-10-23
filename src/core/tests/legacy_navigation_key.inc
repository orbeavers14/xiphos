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

// Frozen complete get_valid_key from 69db801.
char *BackEnd::get_valid_key(const char *module_name, const char *key)
{
	SWModule *mod = get_SWModule(module_name);
	if (!mod)
		return strdup("Gen.1.1");

	VerseKey *vkey = (VerseKey *)(SWKey *)(*mod);
	char *mykey;

	int t = module_type(module_name);
	if ((t == TEXT_TYPE) || (t == COMMENTARY_TYPE))
		vkey->setAutoNormalize(1);
	vkey->setText(key);

	// what's with this flagrant anglocentrism?
	if (sword_locale && !strcmp(sword_locale, "en"))
		mykey = strdup((char *)vkey->getShortText());
	else
		mykey = strdup((char *)vkey->getText());
	return mykey;
}
