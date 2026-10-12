/*
 * Xiphos Bible Study Tool
 * mod_global_ops.c - setup for SWORD global options and
 *                    a few of our own in the gui
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include <swmgr.h>
#include <swmodule.h>
#include "xiphos/core/engine_options.hpp"

#include "gui/parallel_view.h"
#include "gui/utilities.h"

#include "main/global_ops.hh"
#include "main/lists.h"
#include "main/module_dialogs.h"
#include "main/settings.h"
#include "main/parallel_view.h"
#include "main/sword.h"

#include "backend/sword_main.hh"

#include "gui/debug_glib_null.h"

void _set_global_textual(const char *option,
			 const char *choice)
{
	SWMgr *mgr = backend->get_mgr();
	mgr->setGlobalOption(option, choice);
}

/******************************************************************************
 * Name
 *   main_save_module_options
 *
 * Synopsis
 *   #include "main/global_ops.h"
 *
 *   int main_save_module_options(char * mod_name, char * option,
 *				    int choice)
 *
 * Description
 *
 *
 * Return value
 *   int
 */

int main_save_module_options(const char *mod_name,
			     const char *option,
			     int choice)
{
	gchar *buf = g_strdup_printf("%s/modops.conf", settings.gSwordDir);
	SWConfig module_options(buf);

	module_options.load();
	g_free(buf);

	module_options[mod_name][option] = (choice ? "On" : "Off");

	module_options.save();
	return true;
}

/******************************************************************************
 * Name
 *  main_get_option
 *
 * Synopsis
 *   #include "main/mod_global_ops.h"
 *
 *   void main_get_one_option(const char *mod_name, const char *op)
 *
 * Description
 *   obtain a single option
 *
 * Return value
 *   int
 */

int
main_get_one_option(const char *mod_name, const char *op)
{
	gchar *buf = g_strdup_printf("%s/modops.conf", settings.gSwordDir);
	SWConfig module_options(buf);

	module_options.load();
	g_free(buf);

	return gui_of2tf(module_options[mod_name][op].c_str());
}

/******************************************************************************
 * Name
 *  main_set_global_options
 *
 * Synopsis
 *   #include "main/global_ops.h"
 *
 *   void main_set_global_options(GLOBAL_OPS * ops)
 *
 * Description
 *   set module global options
 *
 * Return value
 *   void
 */

void main_set_global_options(GLOBAL_OPS *ops)
{
	xiphos::core::EngineOptions values;
	values.strongs = ops->strongs != 0;
	values.morphs = ops->morphs != 0;
	values.footnotes = ops->footnotes != 0;
	values.greekaccents = ops->greekaccents != 0;
	values.lemmas = ops->lemmas != 0;
	values.scripturerefs = ops->scripturerefs != 0;
	values.hebrewpoints = ops->hebrewpoints != 0;
	values.hebrewcant = ops->hebrewcant != 0;
	values.italic_headings = ops->italic_headings != 0;
	values.words_in_red = ops->words_in_red != 0;
	values.xlit = ops->xlit != 0;
	values.enumerated = ops->enumerated != 0;
	values.glosses = ops->glosses != 0;
	values.morphseg = ops->morphseg != 0;
	values.transliteration = ops->transliteration != 0;
	values.variants_primary = ops->variants_primary != 0;
	values.variants_secondary = ops->variants_secondary != 0;
	values.variants_all = ops->variants_all != 0;
	xiphos::core::apply_engine_options(*backend->get_mgr(), values);
}

/******************************************************************************
 * Name
 *    main_new_globals
 *
 * Synopsis
 *   #include "main/global_ops.h"
 *
 *   GLOBAL_OPS *main_new_globals(gchar *mod_name)
 *
 * Description
 *   obtain all options for the module.
 *
 * Return value
 *   GLOBAL_OPS *
 */

GLOBAL_OPS *main_new_globals(const gchar *mod_name)
{
	GLOBAL_OPS *ops;
	gchar *buf = g_strdup_printf("%s/modops.conf", settings.gSwordDir);
	SWConfig module_options(buf);

	module_options.load();
	g_free(buf);

	ops = g_new0(GLOBAL_OPS, 1);
	xiphos::core::EngineOptionPreferences preferences;
	for (const auto &item : module_options[mod_name])
		preferences.emplace(item.first.c_str(), item.second.c_str());
	const auto values = xiphos::core::engine_options_from_preferences(preferences);
	ops->strongs = values.strongs;
	ops->morphs = values.morphs;
	ops->lemmas = values.lemmas;
	ops->transliteration = values.transliteration;
	ops->italic_headings = values.italic_headings;
	ops->xlit = values.xlit;
	ops->enumerated = values.enumerated;
	ops->morphseg = values.morphseg;

	ops->variants_all = values.variants_all;
	ops->variants_primary = values.variants_primary;
	ops->variants_secondary = values.variants_secondary;

	// special case options, default on:
	ops->words_in_red = values.words_in_red;
	ops->headings =
	    (module_options[mod_name]["Headings"] != "Off");
	ops->footnotes = values.footnotes;
	ops->scripturerefs = values.scripturerefs;
	ops->xrefnotenumbers =
	    (module_options[mod_name]["XrefNoteNumbers"] != "Off");
	ops->display_chapter_N =
	    (module_options[mod_name]["Display Chapter N"] != "Off");

	// more special case default on: heb/grk support.
	ops->greekaccents = values.greekaccents;
	ops->hebrewpoints = values.hebrewpoints;
	ops->hebrewcant = values.hebrewcant;

	// more special case default on: japanese glosses.
	ops->glosses = values.glosses;

	// we prefer and assume paragraph layout.
	// we take user preference, and alter our default for
	// those modules showing proper configuration for it.
	ops->verse_per_line =
	    ((*(module_options[mod_name]["style"].c_str()) == 'v')
		 ? 1
		 : ((*(module_options[mod_name]["style"].c_str()) == 'p')
			? 0
			: (main_check_for_option(mod_name, "Feature", "NoParagraphs")
			       ? 1
			       : 0))); // default to paragraph style.

	// options not from configuration: xiphos-specific capability.
	ops->image_content =
	    (*(module_options[mod_name]["Image Content"].c_str()) == '\0')
		? -1 // "unknown"; otherwise, it's like the others.
		: gui_of2tf(module_options[mod_name]["Image Content"].c_str());

	ops->respect_font_faces =
	    (*(module_options[mod_name]["Respect Font Faces"].c_str()) == '\0')
		? -1 // "unknown"; otherwise, it's like the others.
		: gui_of2tf(module_options[mod_name]["Respect Font Faces"].c_str());

	// special case, xiphos-specific feature: commentary whole chapter.
	ops->commentary_by_chapter =
	    ((backend->module_type(mod_name) == COMMENTARY_TYPE) &&
	     (module_options[mod_name]["Commentary by Chapter"] != "Off"));

	ops->doublespace =
	    gui_of2tf(module_options[mod_name]["Doublespace"].c_str());

	return ops;
}
