// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/dictionary.hpp"
#include <glib.h>
#include <ctime>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
static std::vector<std::string> trace;
static std::string copy(const char *value) { return value ? value : "<null>"; }
static void log(const char *action,const char *a=nullptr,const char *b=nullptr) { trace.push_back(std::string(action)+":"+copy(a)+":"+copy(b)); }
struct Widget { std::string text; };
struct Settings {
 bool havedict; char *DictWindowModule; char *devotionalmod;
 bool comm_showing,showtexts,showpreview,showcomms,showdicts;
} settings;
static Widget strongs,dict_entry,dev_entry,notebook,dev_html;
struct Widgets { Widget *all_strongs,*entry_dict,*entry_devotional,*notebook_dict_devot,*html_devotional; } widgets;
struct Module { const char *feature; const char *getConfigEntry(const char *) { return feature; } } module;
struct Backend {
 bool installed;
 bool is_module(const char *name) { log("installed",name); return installed; }
 Backend *get_mgr() { return this; }
 Module *getModule(const char *name) { log("module",name); return &module; }
} backend_object;
static Backend *backend=&backend_object;
static void gtk_widget_show(Widget *) { log("strongs-show"); }
static void gtk_widget_hide(Widget *) { log("strongs-hide"); }
static void gui_reassign_strdup(char **target,gchar *value) { g_free(*target);*target=g_strdup(value); }
static void xml_set_value(const char *,const char *,const char *key,const char *value) { log("xml",key,value); }
static void gtk_entry_set_text(Widget *entry,const char *text) { entry->text=text;log(entry==&dev_entry?"dev-key":"dict-key",text); }
static const char *gtk_entry_get_text(Widget *entry) { return entry->text.c_str(); }
static void gtk_notebook_set_current_page(Widget *,int page) { log("page",std::to_string(page).c_str()); }
static void main_display_devotional(Widget *) { log("dev-render",settings.devotionalmod,widgets.entry_devotional?dev_entry.text.c_str():"<no-entry>"); }
static void gui_notebook_dict_goto_dict() { log("dict-pane"); }
static void main_dictionary_entry_changed(const char *name) { log("dict-refresh",name); }
static void gtk_widget_activate(Widget *) { log("dict-activate",dict_entry.text.c_str()); }
static void main_dict_history_add(const char *name,const char *key) { log("history",name,key); }
static void gui_update_tab_struct(const char *,const char *,const char *name,const char *,const char *key,const char *,bool,bool,bool,bool,bool) { log("tab",name,key); }
// Deterministic local civil date, independent of host clock/time zone.
static time_t fixed_time(time_t *) { log("clock");return 0; }
static tm *fixed_localtime(const time_t *) { static tm date={};date.tm_mon=6;date.tm_mday=9;return &date; }
#define XI_message(arguments) ((void)0)
#define GTK_ENTRY(value) (value)
#define GTK_NOTEBOOK(value) (value)
#define time fixed_time
#define localtime fixed_localtime
namespace frozen {
#include "legacy_dictionary_display.inc"
}
namespace actual {
#include "legacy_dictionary_display_actual.inc"
}
#undef time
#undef localtime
using Handler=void (*)(const char *,const char *);
static std::vector<std::string> run(Handler handler,const char *name,const char *key,const char *current,const char *feature,bool available,bool installed,bool entry,const char *old="GRACE") {
 settings={available,g_strdup(current),g_strdup("PreviousDev"),true,true,true,true,true};
 widgets={&strongs,&dict_entry,entry?&dev_entry:nullptr,&notebook,&dev_html};
 dict_entry.text=old;dev_entry.text="previous-date";module.feature=feature;backend_object.installed=installed;trace.clear();
 handler(name,key);
 log("final",settings.DictWindowModule,settings.devotionalmod);log("entries",dict_entry.text.c_str(),dev_entry.text.c_str());
 auto result=trace;g_free(settings.DictWindowModule);g_free(settings.devotionalmod);settings.DictWindowModule=nullptr;settings.devotionalmod=nullptr;return result;
}
static void check(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
static bool has(const std::vector<std::string> &events,const std::string &prefix) { for(const auto &event:events)if(event.compare(0,prefix.size(),prefix)==0)return true;return false; }
int main(){try{
 std::size_t comparisons=0;
 for(const char *name:std::vector<const char *>{nullptr,"Target"})for(const char *current:std::vector<const char *>{nullptr,"Target","Other"})
 for(const char *feature:std::vector<const char *>{nullptr,"HebrewDef","GreekDef","DailyDevotion","Other"})
 for(const char *key:std::vector<const char *>{nullptr,"Grace","grace","straße","07.09","99.99","7.09","07/09"," 07.09","07.09x","","\xff"})
 for(bool available:{false,true})for(bool installed:{false,true})for(bool entry:{false,true}) {
  check(run(actual::main_display_dictionary,name,key,current,feature,available,installed,entry)==run(frozen::main_display_dictionary,name,key,current,feature,available,installed,entry),"Complete dictionary/devotional display trace differs");++comparisons;
 }
 // Compare complete dispatch for every five-byte numeric date shape and each
 // prefix, exercising bounded short-string reads under the sanitizer builds.
 for(int value=0;value<10000;++value) {
  std::string key="00.00";int digits=value;
  for(int index:{4,3,1,0}) { key[index]=static_cast<char>('0'+digits%10);digits/=10; }
  check(run(actual::main_display_dictionary,"Target",key.c_str(),"Other","DailyDevotion",true,true,true)==
        run(frozen::main_display_dictionary,"Target",key.c_str(),"Other","DailyDevotion",true,true,true),"Complete numeric date-shape dispatch differs");++comparisons;
 }
 for(const char *key:{"","0","00","00.","00.0","00.00","00.000"}) {
  check(run(actual::main_display_dictionary,"Target",key,"Other","DailyDevotion",true,true,true)==
        run(frozen::main_display_dictionary,"Target",key,"Other","DailyDevotion",true,true,true),"Short/long date prefix dispatch differs");++comparisons;
 }
 using xiphos::core::dictionary_display_route;using xiphos::core::DictionaryDisplayRoute;
 check(dictionary_display_route(nullptr,"Target","DailyDevotion",nullptr)==DictionaryDisplayRoute::dictionary &&
       dictionary_display_route("Other",nullptr,"DailyDevotion",nullptr)==DictionaryDisplayRoute::dictionary &&
       dictionary_display_route("Other","Target",nullptr,nullptr)==DictionaryDisplayRoute::dictionary &&
       dictionary_display_route("Other","Target","DailyDevotion",nullptr)==DictionaryDisplayRoute::devotional_today,
       "Shared route handles absent module/feature/key without clock or dereference");
 const auto switched=run(actual::main_display_dictionary,"Target","bad","Other","DailyDevotion",true,true,true);
 check(has(switched,"clock:")&&has(switched,"dev-render:Target:07.09")&&!has(switched,"history:"),"Switched devotional invalid date uses deterministic today and returns before dictionary/history");
 const auto shape=run(actual::main_display_dictionary,"Target","99.99","Other","DailyDevotion",true,true,true);
 check(!has(shape,"clock:")&&has(shape,"dev-render:Target:99.99"),"Retained date rule checks shape only, not calendar validity");
 for(const char *current:std::vector<const char *>{nullptr,"Target"}) {
  const auto same=run(actual::main_display_dictionary,"Target","bad",current,"DailyDevotion",true,true,true);
  check(!has(same,"clock:")&&!has(same,"dev-render:")&&has(same,"history:Target:BAD"),"Absent/current dictionary module takes ordinary dictionary path even for devotional feature");
 }
 const auto refresh=run(actual::main_display_dictionary,"Target",nullptr,"Target",nullptr,true,true,true);
 check(has(refresh,"dict-refresh:Target:")&&has(refresh,"history:Target:GRACE"),"Null key defaults to Grace; matching uppercase entry refreshes and records history");
 const auto invalid=run(actual::main_display_dictionary,"Target","\xff","Other",nullptr,true,true,true);
 check(has(invalid,"xml:dict:Target")&&has(invalid,"dict-pane:")&&!has(invalid,"history:")&&!has(invalid,"tab:"),"Invalid UTF-8 returns after module/pane changes but before history/tab publication");
 std::cout<<"Complete dictionary/devotional display: "<<comparisons<<" frozen/current traces; date-shape, local today, module-switch and history ordering agree\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
