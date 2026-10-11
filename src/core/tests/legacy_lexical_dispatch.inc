// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2000-2026 Xiphos Developer Team
// Frozen complete main click/hover handlers from CrossWire baseline c9976a8.
static gint show_morph(const char *module_name,
		       const gchar *stype,
		       const gchar *svalue,
		       gboolean clicked)
{
	const gchar *modbuf = NULL;

	if (!strcmp(stype, "Greek") ||
	    strstr(stype, "x-Robinson") ||
	    strstr(stype, "robinson") ||
	    strstr(stype, "packard") ||
	    strstr(stype, "Robinson")) {
		int testament = backend->get_key_testament(module_name, settings.currentverse);
		if (testament == 2) {
            modbuf = settings.morph_greek_lex_nt; // Utilise votre variable NT
        } else {
            modbuf = settings.morph_greek_lex_ot; // Utilise votre variable OT
        }
        
        // Sécurité si les réglages sont vides
        if (!modbuf || !backend->is_module(modbuf)) {
            modbuf = (testament == 2) ? "Robinson" : "Packard";
		}
	}

	if (strstr(stype, "oshm") ){
		if (backend->is_module("OSHM")) {
			modbuf=settings.morph_heb_lex;
		}
	}

	if (clicked) {
		main_display_dictionary(modbuf, (gchar *)svalue);
	} else {
		gchar *mybuf = main_get_rendered_text(modbuf, (gchar *)svalue);
		if (mybuf) {
			main_information_viewer(modbuf,
						mybuf,
						(gchar *)svalue,
						"showMorph",
						(gchar *)stype,
						NULL,
						NULL);
			g_free(mybuf);
		}
	}
	return 1;
}

static gint show_strongs(const gchar *stype, const gchar *svalue,
			 gboolean clicked)
{
	const gchar *modbuf = NULL;

	if (stype && (*stype != '\0')) {
		if (!strcmp(stype, "Greek"))
			modbuf = settings.lex_greek;
		else
			modbuf = settings.lex_hebrew;
	} else
		modbuf = "InvStrongsRealGreek";

	if (clicked) {
		main_display_dictionary(modbuf, (gchar *)svalue);
	} else {
		gchar *mybuf = main_get_rendered_text(modbuf, (gchar *)svalue);
		if (mybuf) {
			main_information_viewer(
			    modbuf,
			    mybuf,
			    (gchar *)svalue,
			    "showStrongs",
			    (gchar *)stype,
			    NULL,
			    NULL);
			g_free(mybuf);
		}
	}

	return 1;
}
