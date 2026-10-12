/*
 * Xiphos Bible Study Tool
 * display.cc -
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

// Complete frozen functions, CrossWire c9976a8; postprocessors are harness shims.
char *FrozenHeadingBackend::get_entry_attribute(const char *level1,
				   const char *level2,
				   const char *level3,
				   bool render)
{
	UTF8HTML u2html;
	if (render)
		display_mod->renderText();
	SWBuf attribute2 = display_mod->getEntryAttributes()[level1][level2][level3].c_str();

	u2html.processText(attribute2);

	if (attribute2.length()) {
		return strdup(attribute2.c_str());
	}
	return NULL;
}

void
frozen_cache_header(ModuleCache::CacheVerse &cVerse,
	    SWModule &mod,
	    GLOBAL_OPS *ops, FrozenHeadingBackend *be)
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

