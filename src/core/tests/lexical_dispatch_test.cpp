// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/lexicon.h"
#include <glib.h>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
struct Settings {
  const char *lex_greek="GreekPreferred", *lex_hebrew="HebrewPreferred";
  const char *morph_greek_lex_nt="NTPreferred", *morph_greek_lex_ot="OTPreferred", *morph_heb_lex="HebrewMorph";
  const char *currentverse="Committed source";
} settings;
std::vector<std::string> calls;
std::string copy(const char *value) { return value ? value : "<null>"; }
struct BackEnd {
  std::set<std::string> installed; int testament=1; bool renders=true;
  int get_key_testament(const char *module,const char *key) {
    calls.push_back("context:"+copy(module)+":"+copy(key)); return testament;
  }
  bool is_module(const char *module) {
    calls.push_back("installed:"+copy(module)); return module && installed.count(module);
  }
} engine;
BackEnd *backend=&engine;
void main_display_dictionary(const char *module,const char *key) {
  calls.push_back("dictionary:"+copy(module)+":"+copy(key));
}
gchar *main_get_rendered_text(const char *module,const char *key) {
  calls.push_back("render:"+copy(module)+":"+copy(key));
  if (!engine.renders || !module || !engine.installed.count(module)) return nullptr;
  return g_strdup(("owned HTML:"+copy(module)+":"+copy(key)).c_str());
}
void main_information_viewer(const char *module,const char *text,const char *key,
                             const char *action,const char *type,const char *morph_text,const char *morph) {
  calls.push_back("preview:"+copy(module)+":"+copy(text)+":"+copy(key)+":"+
    copy(action)+":"+copy(type)+":"+copy(morph_text)+":"+copy(morph));
}
namespace frozen {
#include "legacy_lexical_dispatch.inc"
}
namespace actual {
#include "legacy_lexical_dispatch_actual.inc"
}
void check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
template<class Frozen,class Actual> void compare(Frozen original,Actual adapted) {
  calls.clear(); const auto expected_result=original(); const auto expected=calls;
  calls.clear(); const auto result=adapted();
  check(result==expected_result && calls==expected,"actual lexical click/hover route differs from frozen baseline");
}
int main() {
  try {
    std::size_t comparisons=0;
    const char *modules[]={"NTPreferred","OTPreferred","HebrewMorph","Robinson","Packard","OSHM"};
    for (unsigned mask=0; mask<64; ++mask) {
      engine.installed={"GreekPreferred","HebrewPreferred","InvStrongsRealGreek"};
      for (unsigned i=0;i<6;++i) if(mask&(1u<<i)) engine.installed.insert(modules[i]);
      for (int testament : {1,2}) for (int state : {0,1,2}) {
        engine.testament=testament;
        settings.morph_greek_lex_nt=state==0 ? "NTPreferred" : state==1 ? nullptr : "";
        settings.morph_greek_lex_ot=state==0 ? "OTPreferred" : state==1 ? nullptr : "";
        settings.morph_heb_lex=state==0 ? "HebrewMorph" : state==1 ? nullptr : "";
        for (const char *type : {"Greek","Hebrew","x-Robinson","robinson","Robinson","packard","oshm","x-HebMorph","robinson-oshm","lemma.fixture",""})
          for (const char *key : {"alpha","café","straße","N-NSM"})
            for (bool clicked : {false,true}) for (bool renders : {false,true}) {
              engine.renders=renders;
              compare([&] { return frozen::show_morph("SourceModule",type,key,clicked); },
                      [&] { return actual::show_morph("SourceModule",type,key,clicked); });
              compare([&] { return frozen::show_strongs(type,key,clicked); },
                      [&] { return actual::show_strongs(type,key,clicked); });
              comparisons+=2;
            }
      }
    }
    for (const char *greek : {"GreekPreferred","",static_cast<const char *>(nullptr)})
      for (const char *hebrew : {"HebrewPreferred","",static_cast<const char *>(nullptr)}) {
        settings.lex_greek=greek; settings.lex_hebrew=hebrew;
        for (const char *type : {"Greek","Hebrew","lemma.fixture","",static_cast<const char *>(nullptr)})
          for (bool clicked : {false,true}) {
            compare([&] { return frozen::show_strongs(type,"café",clicked); },
                    [&] { return actual::show_strongs(type,"café",clicked); }); ++comparisons;
          }
      }
    settings.lex_greek="GreekPreferred"; engine.renders=true;
    calls.clear(); actual::show_strongs("Greek","café",false);
    check(calls.size()==2 && calls[0]=="render:GreekPreferred:café" &&
      calls[1].find("preview:GreekPreferred:")==0,"hover reads raw Unicode into information viewer");
    calls.clear(); actual::show_strongs("Greek","café",true);
    check(calls.size()==1 && calls[0]=="dictionary:GreekPreferred:café","click delegates raw command to dictionary handler");
    std::cout<<comparisons<<" complete frozen/actual main lexical dispatch comparisons\n";
    return 0;
  } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
