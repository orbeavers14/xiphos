// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/lexicon.h"
#include <cstring>
#include <set>
#include <string>
#include <iostream>
#include <stdexcept>
extern "C" int lexicon_c_boundary_check(void);
using gchar = char;
struct Settings { char *lex_greek, *lex_hebrew, *morph_greek_lex_nt, *morph_greek_lex_ot, *morph_heb_lex; const char *currentverse; };
char greek[]="GreekPreferred", hebrew[]="HebrewPreferred", nt[]="NTPreferred", ot[]="OTPreferred", hm[]="HebrewMorph";
Settings settings{greek, hebrew, nt, ot, hm, "Main context"};
struct BackEnd {
  std::set<std::string> installed; int testament = 1;
  int get_key_testament(const char *module, const char *key) {
    calls += std::string(module) + ":" + key + ";"; return testament;
  }
  bool is_module(const char *module) { calls += std::string("exists:") + (module ? module : "<null>") + ";"; return module && installed.count(module); }
  std::string calls;
};
BackEnd engine; BackEnd *backend = &engine;
struct DIALOG_DATA { BackEnd *backend; const char *mod_name, *key; };
namespace frozen {
#include "legacy_lexical_selection.inc"
}
namespace actual {
#include "legacy_lexical_actual.inc"
}
std::string copied(const char *value) { return value ? value : "<none>"; }
int main() {
 try {
  if (!lexicon_c_boundary_check()) throw std::runtime_error("C11 lexicon boundary failed");
  unsigned count = 0;
  DIALOG_DATA dialog{&engine, "DialogModule", "Dialog context"};
  for (const char *type : {"", "Greek", "Hebrew", "x-Robinson", "robinson", "Robinson", "packard", "oshm", "x-HebMorph", "robinson-oshm", "lemma.fixture"}) {
   for (int testament : {1, 2}) for (unsigned mask = 0; mask < 64; ++mask) {
    engine.testament = testament; engine.installed.clear();
    const char *modules[]={"NTPreferred", "OTPreferred", "HebrewMorph", "Robinson", "Packard", "OSHM"};
    for (unsigned i=0;i<6;++i) if (mask & (1u<<i)) engine.installed.insert(modules[i]);
    for (int state : {0, 1, 2}) {
     char empty[]="";
     settings.morph_greek_lex_nt=state == 0 ? nt : state == 1 ? nullptr : empty; settings.morph_greek_lex_ot=state == 0 ? ot : state == 1 ? nullptr : empty;
     settings.morph_heb_lex=state == 0 ? hm : state == 1 ? nullptr : empty;
     engine.calls.clear(); const auto expectedMain=copied(frozen::main_morph("MainModule",type)); const auto mainCalls=engine.calls;
     engine.calls.clear(); if (copied(actual::main_morph("MainModule",type)) != expectedMain || engine.calls != mainCalls)
       throw std::runtime_error("Main morphology selection/context diverged");
     engine.calls.clear(); const auto expectedDialog=copied(frozen::dialog_morph(&dialog,type)); const auto dialogCalls=engine.calls;
     engine.calls.clear(); if (copied(actual::dialog_morph(&dialog,type)) != expectedDialog || engine.calls != dialogCalls)
       throw std::runtime_error("Dialog morphology selection/context diverged");
     if (copied(actual::main_strongs(type)) != copied(frozen::main_strongs(type)) ||
         copied(actual::dialog_strongs(type)) != copied(frozen::dialog_strongs(type)))
       throw std::runtime_error("Strong's selection diverged");
     count += 4;
    }
   }
  }
  settings.morph_greek_lex_nt=nullptr; settings.morph_greek_lex_ot=nullptr; settings.morph_heb_lex=nullptr;
  for (int greekState : {0, 1, 2}) for (int hebrewState : {0, 1, 2}) {
    char empty[]="";
    settings.lex_greek=greekState == 0 ? greek : greekState == 1 ? nullptr : empty;
    settings.lex_hebrew=hebrewState == 0 ? hebrew : hebrewState == 1 ? nullptr : empty;
    for (const char *type : {"Greek", "Hebrew", "lemma.fixture", ""}) {
      if (copied(actual::main_strongs(type)) != copied(frozen::main_strongs(type)) ||
          copied(actual::dialog_strongs(type)) != copied(frozen::dialog_strongs(type)))
        throw std::runtime_error("Absent/empty Strong's preferences diverged");
      count += 2;
    }
  }
  settings.lex_greek=greek; settings.lex_hebrew=hebrew;
  if (copied(actual::main_strongs(nullptr)) != "InvStrongsRealGreek" ||
      copied(actual::main_strongs("lemma.fixture")) != "HebrewPreferred" ||
      copied(actual::dialog_strongs("lemma.fixture")) != "<none>")
    throw std::runtime_error("Distinct Strong's paths lost");
  settings.morph_greek_lex_nt=nullptr;
  engine.installed.clear(); engine.testament=2; engine.calls.clear();
  if (copied(actual::main_morph("MainModule", "Greek")) != "Robinson" || engine.calls != "MainModule:Main context;")
    throw std::runtime_error("Main missing fallback/context changed");
  engine.calls.clear();
  if (copied(actual::dialog_morph(&dialog,"Greek")) != "<none>" || engine.calls != "DialogModule:Dialog context;exists:Robinson;")
    throw std::runtime_error("Dialog missing fallback/context changed");
  const XiphosLexiconPreferences prefs{greek, hebrew, nt, ot, hm};
  if (xiphos_core_select_lexicon(-1, XIPHOS_LEXICON_STRONGS, "Greek", &prefs, nullptr) ||
      xiphos_core_select_lexicon(XIPHOS_LEXICON_MAIN, -1, "Greek", &prefs, nullptr) ||
      xiphos_core_select_lexicon(XIPHOS_LEXICON_MAIN, XIPHOS_LEXICON_STRONGS, "Greek", nullptr, nullptr) ||
      xiphos_core_select_lexicon(XIPHOS_LEXICON_DIALOG, XIPHOS_LEXICON_STRONGS, nullptr, &prefs, nullptr) ||
      xiphos_core_select_lexicon(XIPHOS_LEXICON_MAIN, XIPHOS_LEXICON_MORPHOLOGY, nullptr, &prefs, nullptr) ||
      xiphos_core_select_lexicon(XIPHOS_LEXICON_MAIN, XIPHOS_LEXICON_MORPHOLOGY, "Greek", &prefs, nullptr))
    throw std::runtime_error("Invalid selection boundary accepted");
  const XiphosLexiconPreferences absent{nullptr, nullptr, nullptr, nullptr, nullptr};
  if (xiphos_core_select_lexicon(XIPHOS_LEXICON_MAIN, XIPHOS_LEXICON_STRONGS, "Greek", &absent, nullptr) ||
      copied(xiphos_core_select_lexicon(XIPHOS_LEXICON_MAIN, XIPHOS_LEXICON_STRONGS, nullptr, &absent, nullptr)) != "InvStrongsRealGreek")
    throw std::runtime_error("Absent Strong's preferences/default changed");
  std::cout << "Frozen/actual lexical selection matches (" << count << " cases)\n";
 } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
