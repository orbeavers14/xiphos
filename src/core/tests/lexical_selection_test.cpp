// SPDX-License-Identifier: GPL-2.0-or-later
#include <cstring>
#include <set>
#include <string>
#include <iostream>
#include <stdexcept>
using gchar = char;
struct Settings { char *lex_greek, *lex_hebrew, *morph_greek_lex_nt, *morph_greek_lex_ot, *morph_heb_lex; const char *currentverse; };
char greek[]="GreekPreferred", hebrew[]="HebrewPreferred", nt[]="NTPreferred", ot[]="OTPreferred", hm[]="HebrewMorph";
Settings settings{greek, hebrew, nt, ot, hm, "Main context"};
struct BackEnd {
  std::set<std::string> installed; int testament = 1;
  int get_key_testament(const char *module, const char *key) {
    calls += std::string(module) + ":" + key + ";"; return testament;
  }
  bool is_module(const char *module) { return module && installed.count(module); }
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
  unsigned count = 0;
  DIALOG_DATA dialog{&engine, "DialogModule", "Dialog context"};
  for (const char *type : {"", "Greek", "Hebrew", "x-Robinson", "robinson", "Robinson", "packard", "oshm", "x-HebMorph", "robinson-oshm", "lemma.fixture"}) {
   for (int testament : {1, 2}) for (unsigned mask = 0; mask < 64; ++mask) {
    engine.testament = testament; engine.installed.clear();
    const char *modules[]={"NTPreferred", "OTPreferred", "HebrewMorph", "Robinson", "Packard", "OSHM"};
    for (unsigned i=0;i<6;++i) if (mask & (1u<<i)) engine.installed.insert(modules[i]);
    for (bool empty : {false, true}) {
     settings.morph_greek_lex_nt=empty ? nullptr : nt; settings.morph_greek_lex_ot=empty ? nullptr : ot;
     settings.morph_heb_lex=empty ? nullptr : hm;
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
  if (copied(actual::main_strongs(nullptr)) != "InvStrongsRealGreek" ||
      copied(actual::main_strongs("lemma.fixture")) != "HebrewPreferred" ||
      copied(actual::dialog_strongs("lemma.fixture")) != "<none>")
    throw std::runtime_error("Distinct Strong's paths lost");
  engine.installed.clear(); engine.testament=2; engine.calls.clear();
  if (copied(actual::main_morph("MainModule", "Greek")) != "Robinson" || engine.calls != "MainModule:Main context;")
    throw std::runtime_error("Main missing fallback/context changed");
  engine.calls.clear();
  if (copied(actual::dialog_morph(&dialog,"Greek")) != "<none>" || engine.calls != "DialogModule:Dialog context;")
    throw std::runtime_error("Dialog missing fallback/context changed");
  std::cout << "Frozen/actual lexical selection matches (" << count << " cases)\n";
 } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
