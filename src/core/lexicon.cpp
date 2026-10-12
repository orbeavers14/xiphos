// SPDX-License-Identifier: GPL-2.0-or-later
// Extracted from main/url.cc and main/module_dialogs.cc.
// Copyright (C) 2000-2026 Xiphos Developer Team
#include "xiphos/core/lexicon.h"
#include <cstring>
namespace {
bool installed(const XiphosLexiconContext *context, const char *module) {
  return module && context && context->module_exists && context->module_exists(context->user, module);
}
}
const char *xiphos_core_select_lexicon(int origin, int action, const char *type,
    const XiphosLexiconPreferences *preferences, const XiphosLexiconContext *context) {
  if (!preferences || (origin != XIPHOS_LEXICON_MAIN && origin != XIPHOS_LEXICON_DIALOG)) return nullptr;
  if (action == XIPHOS_LEXICON_STRONGS) {
    if (origin == XIPHOS_LEXICON_MAIN) {
      if (!type || !*type) return "InvStrongsRealGreek";
      return std::strcmp(type, "Greek") == 0 ? preferences->greek : preferences->hebrew;
    }
    if (!type) return nullptr;
    if (std::strcmp(type, "Greek") == 0) return preferences->greek;
    if (std::strcmp(type, "Hebrew") == 0) return preferences->hebrew;
    return nullptr;
  }
  if (action != XIPHOS_LEXICON_MORPHOLOGY || !type) return nullptr;
  const bool greek = std::strcmp(type, "Greek") == 0 || std::strstr(type, "x-Robinson") ||
    std::strstr(type, "robinson") || std::strstr(type, "packard") ||
    (origin == XIPHOS_LEXICON_MAIN && std::strstr(type, "Robinson"));
  const char *selected = nullptr;
  if (greek) {
    if (!context || !context->source_testament) return nullptr;
    const bool nt = context->source_testament(context->user) == 2;
    selected = nt ? preferences->greek_morph_nt : preferences->greek_morph_ot;
    const char *fallback = nt ? "Robinson" : "Packard";
    if (!installed(context, selected))
      selected = origin == XIPHOS_LEXICON_MAIN || installed(context, fallback) ? fallback : nullptr;
  }
  if (origin == XIPHOS_LEXICON_MAIN) {
    // Deliberately independent: Hebrew can override a Greek selection.
    if (std::strstr(type, "oshm") && installed(context, "OSHM")) selected = preferences->hebrew_morph;
  } else if (!greek && (std::strcmp(type, "Hebrew") == 0 || std::strstr(type, "x-HebMorph"))) {
    if (installed(context, preferences->hebrew_morph)) selected = preferences->hebrew_morph;
  }
  return selected;
}

unsigned xiphos_core_lexicon_role(const char *feature) {
  if (!feature) return 0;
  if (std::strcmp(feature, "GreekDef") == 0) return XIPHOS_LEXICON_GREEK_DEF;
  if (std::strcmp(feature, "HebrewDef") == 0) return XIPHOS_LEXICON_HEBREW_DEF;
  if (std::strcmp(feature, "GreekParse") == 0) return XIPHOS_LEXICON_GREEK_PARSE;
  if (std::strcmp(feature, "HebrewParse") == 0) return XIPHOS_LEXICON_HEBREW_PARSE;
  return 0;
}
