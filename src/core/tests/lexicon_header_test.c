/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "xiphos/core/lexicon.h"
#include <string.h>
int lexicon_c_boundary_check(void) {
    if (xiphos_core_lexicon_role("GreekDef") != XIPHOS_LEXICON_GREEK_DEF ||
        xiphos_core_lexicon_role("HebrewDef") != XIPHOS_LEXICON_HEBREW_DEF ||
        xiphos_core_lexicon_role("GreekParse") != XIPHOS_LEXICON_GREEK_PARSE ||
        xiphos_core_lexicon_role("HebrewParse") != XIPHOS_LEXICON_HEBREW_PARSE ||
        xiphos_core_lexicon_role("greekdef") || xiphos_core_lexicon_role(0)) return 0;
    char greek[] = "Original Greek";
    XiphosLexiconPreferences preferences = {greek, "Original Hebrew", 0, 0, 0};
    const char *selected = xiphos_core_select_lexicon(XIPHOS_LEXICON_MAIN,
        XIPHOS_LEXICON_STRONGS, "Greek", &preferences, 0);
    if (selected != greek) return 0;
    if (strcmp(xiphos_core_select_lexicon(XIPHOS_LEXICON_MAIN,
        XIPHOS_LEXICON_STRONGS, 0, &preferences, 0), "InvStrongsRealGreek")) return 0;
    return !xiphos_core_select_lexicon(XIPHOS_LEXICON_DIALOG,
        XIPHOS_LEXICON_STRONGS, 0, &preferences, 0);
}
