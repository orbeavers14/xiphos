// SPDX-License-Identifier: GPL-2.0-or-later
// Frozen fork selection prefixes at 96068973; original Xiphos authors/notices retained.
const char *main_morph(const char *module_name, const char *stype) {
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

 return modbuf;
}
const char *main_strongs(const char *stype) {
	const gchar *modbuf = NULL;

	if (stype && (*stype != '\0')) {
		if (!strcmp(stype, "Greek"))
			modbuf = settings.lex_greek;
		else
			modbuf = settings.lex_hebrew;
	} else
		modbuf = "InvStrongsRealGreek";

 return modbuf;
}
const char *dialog_morph(DIALOG_DATA *d, const char *type) {
	gchar *modbuf = NULL;
	BackEnd *be = (BackEnd *)d->backend;

	// 1. For Greek
	if (!strcmp(type, "Greek") || strstr(type, "x-Robinson") || strstr(type, "robinson") || strstr(type, "packard")) {
		// Which testament (1 = AT, 2 = NT)
		int testament = be->get_key_testament(d->mod_name, d->key);
		if (testament == 2) {
			// --- If New Testament ---
			if (settings.morph_greek_lex_nt && be->is_module(settings.morph_greek_lex_nt)) {
				modbuf = settings.morph_greek_lex_nt;
			} else if (be->is_module("Robinson")) {
				modbuf = (char *)"Robinson";
			}
		} else {
			// --- If old Testament (Septuagint) ---
			if (settings.morph_greek_lex_ot && be->is_module(settings.morph_greek_lex_ot)) {
				modbuf = settings.morph_greek_lex_ot;
			} else if (be->is_module("Packard")) {
				modbuf = (char *)"Packard";
			}
		}
	}
	// 2. For Hebrew
	else if (!strcmp(type, "Hebrew") || strstr(type, "x-HebMorph")) {
		// Check if user has chosen a heb morph dic.
		if (settings.morph_heb_lex && be->is_module(settings.morph_heb_lex)) {
			modbuf = settings.morph_heb_lex;
		}
	}

	if (!modbuf)
		return 0;

 return modbuf;
}
const char *dialog_strongs(const char *type) {
	const gchar *modbuf = NULL;

	if (!strcmp(type, "Greek"))
		modbuf = settings.lex_greek;
	else if (!strcmp(type, "Hebrew"))
		modbuf = settings.lex_hebrew;

 return modbuf;
}
