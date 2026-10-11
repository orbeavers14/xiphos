/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XIPHOS_CORE_LEXICON_H
#define XIPHOS_CORE_LEXICON_H
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { XIPHOS_LEXICON_MAIN = 0, XIPHOS_LEXICON_DIALOG = 1 } XiphosLexiconOrigin;
typedef enum { XIPHOS_LEXICON_STRONGS = 0, XIPHOS_LEXICON_MORPHOLOGY = 1 } XiphosLexiconAction;
typedef struct {
    const char *greek, *hebrew, *greek_morph_nt, *greek_morph_ot, *hebrew_morph;
} XiphosLexiconPreferences;
typedef struct {
    void *user;
    int (*module_exists)(void *user, const char *module);
    int (*source_testament)(void *user);
} XiphosLexiconContext;
/* Legacy main/dialog selection only. Result borrows a preference string or
 * static fallback; copy before preferences change. NULL means no selection.
 * Does not require selected module availability (legacy main fallbacks).
 * Callbacks are synchronous and invoked only by branches that need them.
 * Invalid origin/action/preferences and unsupported NULL types return NULL.
 * Caller owns source context and rendering/history/presentation policy. */
const char *xiphos_core_select_lexicon(int origin, int action, const char *type,
    const XiphosLexiconPreferences *preferences, const XiphosLexiconContext *context);
#ifdef __cplusplus
}
#endif
#endif
