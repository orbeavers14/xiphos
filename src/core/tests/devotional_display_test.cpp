// SPDX-License-Identifier: GPL-2.0-or-later
#include <glib.h>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
static std::vector<std::string> trace;
static std::string copy(const char *s){return s?s:"<null>";}
static void log(const char *a,const char *b=nullptr,const char *c=nullptr){trace.push_back(std::string(a)+":"+copy(b)+":"+copy(c));}
struct GtkWidget{std::string text;};
static GtkWidget entry,button,target,preview,side;
static struct Settings{char *devotionalmod;bool show_previewer_in_sidebar;} settings;
static struct Widgets{GtkWidget *entry_devotional,*button_devotional_date,*html_previewer_text;} widgets;
static struct Sidebar{GtkWidget *html_viewer_widget;} sidebar;
static bool available,rendered;
static int displayed_module=7;
struct SWDisplay{virtual ~SWDisplay(){};virtual void display(int value){log("display",std::to_string(value).c_str());}};
struct Backend{
 SWDisplay *dictDisplay;int *display_mod;
 char *get_render_text(const char *module,const char *key){log("render",module,key);return rendered?g_strdup("Original devotional fixture prose."):nullptr;}
 void set_module_key(const char *module,const char *key){log("position",module,key);}
} backend_value;
static Backend *backend=&backend_value;
struct GTKEntryDisp:SWDisplay{GTKEntryDisp(GtkWidget *widget,Backend *){log("temporary-display",widget==&target?"target":"other");}};
static GList first={const_cast<char *>("InstalledDevotional"),nullptr,nullptr};
enum{DEVOTION_LIST};
static GList *get_list(int){log("list");return available?&first:nullptr;}
static void xml_set_value(const char *,const char *,const char *key,const char *value){log("xml",key,value);}
static void gui_reassign_strdup(char **dest,gchar *value){g_free(*dest);*dest=g_strdup(value);}
static void gui_generic_warning(const char *value){log("warning",value);}
static const char *gtk_entry_get_text(GtkWidget *widget){return widget->text.c_str();}
static void gtk_entry_set_text(GtkWidget *widget,const char *value){widget->text=value;log("entry",value);}
static void gtk_button_set_label(GtkWidget *widget,const char *value){widget->text=value;log("button",value);}
static char *format_devot_date_local(const char *value){log("local-format",value);return g_strdup_printf("Local<%s>",value);}
static void main_entry_display(GtkWidget *widget,const char *module,const char *text,const char *title,bool flag){log(widget==&side?"sidebar":"preview",module,title);log("body",text,flag?"true":"false");}
static time_t fixed_time(time_t *){log("clock");return 0;}
static tm *fixed_localtime(const time_t *){static tm value={};value.tm_mon=6;value.tm_mday=9;return &value;}
// Intercept indexing, not the date decision: never reproduce the retained
// out-of-bounds access when an arbitrary five-byte entry produces month 0/99.
static struct MonthNames{
 const char *operator[](int index)const{
  log("month-index",std::to_string(index).c_str());
  static const char *names[]={"January","February","March","April","May","June","July","August","September","October","November","December"};
  return index>=0&&index<12?names[index]:"<invalid-month>";
 }
} month_names;
#define _(value) (value)
#define gettext(value) (value)
#define GTK_ENTRY(value) (value)
#define GTK_BUTTON(value) (value)
#define time fixed_time
#define localtime fixed_localtime
namespace frozen{
#include "legacy_devotional_display.inc"
}
namespace actual{
#include "legacy_devotional_display_actual.inc"
}
#undef time
#undef localtime
using Handler=void (*)(GtkWidget *);
static std::vector<std::string> run(Handler handler,const char *module,const char *date,bool installed,bool has_text,bool has_entry,bool has_button,bool has_target,bool use_sidebar){
 settings={g_strdup(module),use_sidebar};widgets={has_entry?&entry:nullptr,has_button?&button:nullptr,&preview};sidebar={&side};entry.text=date;button.text="previous";available=installed;rendered=has_text;
 SWDisplay saved;backend->dictDisplay=&saved;backend->display_mod=&displayed_module;trace.clear();handler(has_target?&target:nullptr);
 log("restored",backend->dictDisplay==&saved?"yes":"no");log("final",settings.devotionalmod,entry.text.c_str());log("label",button.text.c_str());auto result=trace;g_free(settings.devotionalmod);settings.devotionalmod=nullptr;return result;
}
static bool has(const std::vector<std::string>&events,const std::string &prefix){for(const auto &event:events)if(event.compare(0,prefix.size(),prefix)==0)return true;return false;}
static void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
int main(){try{
 std::size_t comparisons=0;
 for(const char *module:std::vector<const char *>{nullptr,"ChosenDevotional"})
 for(const char *date:{"","07.09","12.31","02.30","99.99","00.00","ab.cd","07x09","-1.01","7.09","07.09x","07.99"})
 for(bool installed:{false,true})for(bool text:{false,true})for(bool entry:{false,true})for(bool button:{false,true})for(bool target:{false,true})for(bool sidebar:{false,true}){
  check(run(actual::main_display_devotional,module,date,installed,text,entry,button,target,sidebar)==run(frozen::main_display_devotional,module,date,installed,text,entry,button,target,sidebar),"Complete devotional render trace differs");++comparisons;
 }
 const auto bad=run(actual::main_display_devotional,"ChosenDevotional","99.99",true,true,true,true,true,false);
 check(has(bad,"month-index:98:")&&has(bad,"render:ChosenDevotional:99.99")&&has(bad,"restored:yes:"),"Shape-only dispatched date requests out-of-range month indexing in retained renderer; guarded host records without UB");
 const auto missing=run(actual::main_display_devotional,nullptr,"",false,false,false,false,false,false);
 check(has(missing,"warning:")&&has(missing,"clock:")&&has(missing,"render:<null>:07.09"),"No installed devotional warns but continues to clock/render seam");
 const auto fallback=run(actual::main_display_devotional,nullptr,"bad",true,true,true,true,false,true);
 check(has(fallback,"xml:devotional:InstalledDevotional")&&has(fallback,"sidebar:InstalledDevotional:<b>July 9</b>")&&has(fallback,"entry:07.09:")&&has(fallback,"button:Local<07.09>:"),"First installed fallback, local today and sidebar title/entry/button publication agree");
 const auto empty=run(actual::main_display_devotional,"ChosenDevotional","07.09",true,false,true,true,true,false);
 check(!has(empty,"temporary-display:")&&!has(empty,"entry:")&&!has(empty,"local-format:"),"No rendered text leaves entry/button/display untouched");
 std::cout<<"Complete devotional renderer: "<<comparisons<<" frozen/current traces, guarded month-index hazard, installed fallback, target/preview and publication order agree\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
