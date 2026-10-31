// Frozen from c973bc7; complete control-key coordinate readers.
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
char *BackEnd::key_get_book(const char *module_name, const char *key)
{
	SWModule *mod = get_SWModule(module_name);
	if (!mod)
		return strdup("Genesis");

	VerseKey *vkey = (VerseKey *)(SWKey *)(*mod);

	vkey->setAutoNormalize(1);
	vkey->setText(key);
	char *s = strdup((char *)vkey->getBookName());
	return s;
}
int BackEnd::key_get_chapter(const char *module_name, const char *key)
{
	SWModule *mod = get_SWModule(module_name);
	if (!mod)
		return 1;

	VerseKey *vkey = (VerseKey *)(SWKey *)(*mod);

	vkey->setAutoNormalize(1);
	vkey->setText(key);
	int c = vkey->getChapter();
	return c;
}
int BackEnd::key_get_verse(const char *module_name, const char *key)
{
	SWModule *mod = get_SWModule(module_name);
	if (!mod)
		return 1;

	VerseKey *vkey = (VerseKey *)(SWKey *)(*mod);

	vkey->setAutoNormalize(1);
	vkey->setText(key);
	int v = vkey->getVerse();
	return v;
}
