// SPDX-License-Identifier: GPL-2.0-or-later
#include <glib.h>
#include "xiphos/core/note_presentation.h"
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#define _(value) (value)
static std::vector<std::string> calls;
static std::string copy(const char *text) { return text ? text : "<null>"; }
struct Settings { const char *MainWindowModule="CurrentModule", *currentverse="John 3:16", *bible_text_color="blue"; bool xrefs_in_verse_list=true; } settings;
static int in_url=1;
enum { TEXT_TYPE=1, COMMENTARY_TYPE=2 };
struct SWKey {};
struct VerseKey : SWKey {
 void setAutoNormalize(int value) { calls.push_back("normalize:"+std::to_string(value)); }
 int getChapter() { calls.push_back("chapter"); return 3; }
 void setChapter(int value) { calls.push_back("set-chapter:"+std::to_string(value)); }
 void setVerse(int value) { calls.push_back("set-verse:"+std::to_string(value)); }
};
struct FakeModule {
 VerseKey key;
 operator SWKey *() { return &key; }
 void setKey(const char *value) { calls.push_back("heading-key:"+copy(value)); }
};
struct SWMgr { std::map<std::string,FakeModule *> Modules; } manager;
struct BackEnd {
 FakeModule module; FakeModule *display_mod=nullptr;
 int attribute_state=0, parsed_count=2, type=TEXT_TYPE; bool render_success=true;
 bool is_module(const char *name) { calls.push_back("installed:"+copy(name)); return name && (std::strcmp(name,"Available")==0 || std::strcmp(name,"CurrentModule")==0); }
 SWMgr *get_mgr() { calls.push_back("manager"); return &manager; }
 int module_type(const char *name) { calls.push_back("module-type:"+copy(name)); return type; }
 void set_module_key(const char *name,const char *key) { calls.push_back("key:"+copy(name)+":"+copy(key)); }
 char *get_entry_attribute(const char *group,const char *id,const char *field) {
  calls.push_back("attribute:"+copy(group)+":"+copy(id)+":"+copy(field));
  if(attribute_state==0) return nullptr;
  if(attribute_state==1) return g_strdup("");
  return g_strdup(std::strcmp(field,"body")==0 ? "Original café note" : "John 3:17;John 3:18");
 }
 char *render_this_text(const char *name,const char *body) { calls.push_back("render:"+copy(name)+":"+copy(body)); return render_success ? g_strdup(("rendered:"+copy(body)).c_str()) : nullptr; }
 GList *parse_verse_list(const char *name,const char *list,const char *context) {
  calls.push_back("parse:"+copy(name)+":"+copy(list)+":"+copy(context));
  GList *result=nullptr;
  for(int i=0;i<parsed_count;++i) result=g_list_append(result,g_strdup(i==0 ? "John 3:17" : "John 3:18"));
  return result;
 }
} engine;
static BackEnd *backend=&engine;
void main_display_verse_list_in_sidebar(const char *context,const char *name,const char *list) { calls.push_back("sidebar:"+copy(context)+":"+copy(name)+":"+copy(list)); }
void main_information_viewer(const char *name,const char *html,const char *value,const char *action,const char *type,const char *extra,const char *morph) { calls.push_back("preview:"+copy(name)+":"+copy(html)+":"+copy(value)+":"+copy(action)+":"+copy(type)+":"+copy(extra)+":"+copy(morph)); }
// Track only the handler's GString lifecycle, and reclaim frozen early-return
// leaks after comparison so a large oracle run does not accumulate them.
static std::set<GString *> live_strings;
GString *tracked_new(const char *initial) { auto *result=g_string_new(initial); live_strings.insert(result); return result; }
char *tracked_free(GString *value,gboolean release) { live_strings.erase(value); return g_string_free(value,release); }
#undef g_string_free
#define g_string_new tracked_new
#define g_string_free tracked_free
namespace frozen {
#include "legacy_note_dispatch.inc"
}
namespace actual {
#include "legacy_note_dispatch_actual.inc"
}
#undef g_string_new
#undef g_string_free
static void check(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
template<class Frozen,class Actual> void compare(Frozen baseline,Actual current,bool early=false) {
 calls.clear(); const auto original=baseline(); const auto expected=calls;
 check(live_strings.size()==(early ? 1u : 0u),"frozen allocation behavior recorded");
 for(auto *value:live_strings) g_string_free(value,TRUE);
 live_strings.clear();
 calls.clear(); const auto result=current();
 check(result==original && calls==expected,"note/reference trace differs from baseline");
 check(live_strings.empty(),"actual early-return GString allocation is avoided");
}
int main() {
 try {
  manager.Modules={{"Available",&engine.module},{"CurrentModule",&engine.module}};
  std::size_t comparisons=0;
  for(int active:{0,1}) for(const char *module:{static_cast<const char *>(nullptr),"Missing","Available"})
   for(const char *passage:{"", "Jn3", "John 3:16", "John 3:0"}) for(const char *type:{"n","x","nx","xn","other","","X"})
   for(const char *value:{static_cast<const char *>(nullptr),"","1","café"}) for(bool clicked:{false,true})
   for(int attribute:{0,1,2}) for(bool renders:{false,true}) for(bool list_mode:{false,true})
   for(int count:{0,1,3}) for(int module_type:{TEXT_TYPE,COMMENTARY_TYPE}) {
    in_url=active; engine.attribute_state=attribute; engine.render_success=renders;
    settings.xrefs_in_verse_list=list_mode; engine.parsed_count=count; engine.type=module_type;
    compare([&] { return frozen::show_note(module,passage,type,value,clicked); },[&] { return actual::show_note(module,passage,type,value,clicked); },!active || !value || !*value);
    ++comparisons;
   }
  for(const char *module:{static_cast<const char *>(nullptr),"Missing","Available"})
   for(const char *list:{static_cast<const char *>(nullptr),"","John 3:17;John 3:18","café"}) for(bool clicked:{false,true}) {
    compare([&] { return frozen::show_ref(module,list,clicked); },[&] { return actual::show_ref(module,list,clicked); }); ++comparisons;
   }
  std::cout<<"Complete note/reference dispatch: "<<comparisons<<" frozen/actual traces and GString lifecycles agree\n";
 } catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
