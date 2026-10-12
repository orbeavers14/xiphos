/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "xiphos/core/lexicon.h"
#include <string.h>
int lexicon_c_boundary_check(void) {
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
