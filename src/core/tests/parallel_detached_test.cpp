// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/parallel.hpp"
#include <glib.h>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <limits>
#include <stdexcept>
#include <iostream>
using SWBuf=std::string;
struct GtkWidget { bool realized; } widget;
struct Settings {
 gchar **parallel_list;gchar *MainWindowModule,*cvparallel;
 bool parallel_italic_headings,justify_margins,imageresize;
 const char *bible_bg_color,*bible_text_color,*link_color,*bible_verse_num_color;
 int verse_num_font_size,base_font_size,intCurVerse;
} settings;
static struct Widgets { GtkWidget *html_parallel_dialog; } widgets;
static std::vector<std::string> calls;
static std::string copy(const char *s){return s?s:"<null>";}
static void call(const char *kind,const char *a=nullptr,const char *b=nullptr){calls.push_back(std::string(kind)+":"+copy(a)+":"+copy(b));}
static std::map<std::string,std::string> aliases={{"A","TextA"},{"B","TextB"},{"RTL","RtlText"},{"Gone","Missing"},{"文字","TextA"}};
static std::map<std::string,std::string> labels={{"TextA","A"},{"TextB","B"},{"RtlText","RTL"}};
const char *main_abbrev_to_name(const char *name){call("resolve",name);const auto found=aliases.find(copy(name));return found==aliases.end()?nullptr:found->second.c_str();}
const char *main_name_to_abbrev(const char *name){call("label",name);const auto found=labels.find(copy(name));return found==labels.end()?nullptr:found->second.c_str();}
struct SWModule {
 std::string id;
 const char *getRenderHeader(){call("render-header",id.c_str());return ".original-fixture { color: navy; }";}
};
static SWModule moduleA{"TextA"},moduleB{"TextB"},moduleRtl{"RtlText"};
static struct Backend {
 SWModule *get_SWModule(const char *name){call("control",name);return copy(name)=="TextA"?&moduleA:copy(name)=="TextB"?&moduleB:copy(name)=="RtlText"?&moduleRtl:nullptr;}
} engine;
static Backend *backend=&engine;
static bool gtk_widget_get_realized(GtkWidget *w){call("realized");return w->realized;}
static void *gtk_widget_get_window(GtkWidget *){call("window");return nullptr;}
static void gui_generic_warning(const char *text){call("warning",text);}
static char *AnalyzeForImageSize(char *html,int scale,void *){call("image-scale",std::to_string(scale).c_str());return html;}
static void HtmlOutput(char *html,GtkWidget *,void *,char *anchor){call("html",html);call("anchor",anchor);}
// Characterize orchestration independently of the larger SDK interpolation
// routine. Its current complete source is audited separately in the contract.
static void interpolate_parallel_display(SWModule *control,char *name,SWBuf &text,gchar *key,gint count,gint fraction){
 if(control->id!=name)throw std::runtime_error("Wrong resolved control identity");
 call("interpolate",name,key);call("columns",std::to_string(count).c_str(),std::to_string(fraction).c_str());
 text+="<tr><td>Original interpolation fixture prose.</td></tr>";settings.intCurVerse=17;
}
#define _(value) (value)
#define GTK_WIDGET(value) (value)
#define GDK_WINDOW(value) (value)
namespace frozen {
#include "legacy_parallel_detached.inc"
}
#undef HTML_START
#undef STICKY_MODNAMES
namespace actual {
#include "legacy_parallel_detached_actual.inc"
}
#undef HTML_START
#undef STICKY_MODNAMES
using Handler=void (*)();
static std::vector<std::string> run(Handler handler,const std::vector<std::string> &modules,bool present,bool realized,bool list,bool italic,bool justify,bool images){
 std::vector<gchar *> pointers;for(const auto &module:modules)pointers.push_back(const_cast<char *>(module.c_str()));pointers.push_back(nullptr);
 widget.realized=realized;widgets.html_parallel_dialog=present?&widget:nullptr;
 settings={list?pointers.data():nullptr,const_cast<char *>("TextB"),const_cast<char *>("John 3:17"),italic,justify,images,"#ffffff","#000000","#0000ff","#ff0000",1,2,99};
 calls.clear();handler();return calls;
}
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static bool has(const std::vector<std::string> &trace,const std::string &prefix){for(const auto &line:trace)if(line.compare(0,prefix.size(),prefix)==0)return true;return false;}
int main(){try{
 const std::vector<std::vector<std::string>> sets={{},{"TextA"},{"A","B"},{"Missing","TextA"},{"Gone","B"},{"A","Missing","A"},{"A","","B"},{"文字","RTL"},{"A","B","A","B","A","B","A","B"},{"A","A","A","A","A","A","A","A","A","A"}};
 std::size_t comparisons=0;
 for(const auto &modules:sets)for(bool present:{false,true})for(bool realized:{false,true})for(bool list:{false,true})for(bool italic:{false,true})for(bool justify:{false,true})for(bool images:{false,true}){
  check(run(actual::main_update_parallel_page_detached,modules,present,realized,list,italic,justify,images)==run(frozen::main_update_parallel_page_detached,modules,present,realized,list,italic,justify,images),"Complete detached parallel orchestration differs");++comparisons;
 }
 const auto missing=run(actual::main_update_parallel_page_detached,{"Missing","TextA"},true,true,true,false,false,false);
 check(has(missing,"warning:")&&!has(missing,"html:")&&!has(missing,"control:TextB:"),"Missing first module aborts; never silently substitutes main/another selected module");
 const auto empty=run(actual::main_update_parallel_page_detached,{},true,true,true,false,false,false);
 check(has(empty,"control:<null>:")&&has(empty,"warning:")&&!has(empty,"html:"),"Non-null empty set attempts null first control and produces warning");
 const auto ordered=run(actual::main_update_parallel_page_detached,{"A","Missing","A"},true,true,true,false,false,true);
 check(has(ordered,"control:TextA:")&&has(ordered,"columns:3:33")&&has(ordered,"label:A:")&&has(ordered,"label:Missing:")&&has(ordered,"image-scale:2:")&&has(ordered,"anchor:17:"),"Raw slots including missing/duplicates retain width/order; resolved first controls; post-interpolation selected verse supplies anchor");
 const auto ten=run(actual::main_update_parallel_page_detached,sets.back(),true,true,true,false,false,true);
 check(has(ten,"columns:10:10")&&has(ten,"image-scale:6:"),"Current renderer does not enforce eight-module cap");
 for(unsigned count=0;count<=256;++count){const auto layout=xiphos::core::parallel_layout(count);check(layout.column_percent==(count?100/count:100)&&layout.image_scale==count*2/3,"Shared layout matches retained arithmetic across real counts");}
 const auto large=xiphos::core::parallel_layout(std::numeric_limits<std::size_t>::max());
 const auto limit=std::numeric_limits<std::size_t>::max();
 check(large.column_percent==0&&large.image_scale==limit-limit/3-(limit%3?1:0),"Layout calculation avoids intermediate overflow and retains exact integer result");
 std::cout<<"Complete detached parallel orchestration: "<<comparisons<<" traces; control/alias gates, raw order/count, headings/CSS, selected anchor and shared layout agree\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
