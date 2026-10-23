// SPDX-License-Identifier: GPL-2.0-or-later
#include <glib.h>
#include "xiphos/core/passage_uri.hpp"
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#define XI_message(value) ((void)0)
#define GTK_NOTEBOOK(value) (value)
enum { TYPE_URI, BLANK, MODULE, KEY, TYPE_NOTE, NOTE_NUM };
enum { TEXT_TYPE=1, COMMENTARY_TYPE, PERCOM_TYPE, DICTIONARY_TYPE, BOOK_TYPE, PRAYERLIST_TYPE };
static std::vector<std::string> calls;
static std::string copy(const char *s) { return s ? s : "<null>"; }
static void call(const char *kind,const char *a=nullptr,const char *b=nullptr) { calls.push_back(std::string(kind)+":"+copy(a)+":"+copy(b)); }
struct Settings { gchar *MainWindowModule=nullptr,*special_anchor=nullptr; const char *currentverse="John 3:16"; gboolean comm_showing=false,addhistoryitem=false; } settings;
struct Widgets { void *notebook_comm_book=nullptr; } widgets;
static std::map<std::string,int> types={{"Text",TEXT_TYPE},{"Commentary",COMMENTARY_TYPE},{"Percom",PERCOM_TYPE},{"Dictionary",DICTIONARY_TYPE},{"Book",BOOK_TYPE},{"Prayer",PRAYERLIST_TYPE}};
static std::map<std::string,std::string> aliases={{"T","Text"},{"D","Dictionary"},{"文字 café","Text"},{"Gone","Missing"}};
static int page=0; static bool render=true,tag=false,nav=true;
static std::function<int()> recurse;
static std::set<gchar **> live_buffers;
static gchar **tracked_split(const gchar *s,const gchar *delimiter,gint limit) { auto *p=g_strsplit(s,delimiter,limit);live_buffers.insert(p);return p; }
static void tracked_free(gchar **p) { live_buffers.erase(p);g_strfreev(p); }
const char *main_abbrev_to_name(const char *s) { call("alias",s);auto i=aliases.find(copy(s));return i==aliases.end()?nullptr:i->second.c_str(); }
struct Backend {
 bool is_module(const char *s) { call("installed",s);return types.count(copy(s)); }
 int module_type(const char *s) { call("type",s);auto i=types.find(copy(s));return i==types.end()?0:i->second; }
} engine;
static Backend *backend=&engine;
int main_get_mod_type(const char *s) { call("main-type",s);auto i=types.find(copy(s));return i==types.end()?0:i->second; }
void gui_set_statusbar(const char *s) { call("status",s); }
gchar *bookmark_get_tag_info_for_key(const gchar *s) { call("bookmark",s);return tag?g_strdup("Original bookmark tag"):nullptr; }
void alert_url_not_found(const char *s) { call("alert",s); }
char *main_get_rendered_text(const char *module,const char *key) { call("render",module,key);return render?g_strdup("Original preview prose"):nullptr; }
void main_information_viewer(const char *module,const char *html,const char *key,const char *,const char *,const char *,const char *) { call("preview",module,key);call("preview-html",html); }
void main_display_verse_list_in_sidebar(const char *context,const char *module,const char *list) { call("sidebar-context",context);call("sidebar",module,list); }
const char *main_update_nav_controls(const char *module,const char *key) { call("navigate",module,key);return nav?g_strdup(("canonical:"+copy(key)).c_str()):nullptr; }
void main_display_bible(const char *module,const char *key) { call("bible",module,key);call("anchor",settings.special_anchor);if(recurse)calls.push_back("reentry:"+std::to_string(recurse())); }
void main_display_commentary(const char *module,const char *key) { call("commentary",module,key);call("anchor",settings.special_anchor); }
void main_display_dictionary(const char *module,const char *key) { call("dictionary",module,key); }
void main_display_book(const char *module,const char *key) { call("book",module,key);call("anchor",settings.special_anchor); }
void main_keep_bibletext_dialog_in_sync(char *key) { call("sync",key); }
void editor_sync_with_main() { call("editor"); }
int gtk_notebook_get_current_page(void *) { call("page");return page; }
void gtk_notebook_set_current_page(void *,int value) { calls.push_back("set-page:"+std::to_string(value));page=value;settings.special_anchor=nullptr; }
#define g_strsplit tracked_split
#define g_strfreev tracked_free
namespace frozen {
#include "legacy_passage_uri.inc"
}
namespace actual {
#include "legacy_passage_uri_actual.inc"
}
#undef g_strsplit
#undef g_strfreev
static void check(bool ok,const char *why) { if(!ok)throw std::runtime_error(why); }
struct Trace { int result,page; bool commentary,history,anchor;std::size_t buffers;std::vector<std::string> calls;
 bool operator==(const Trace &b)const{return result==b.result&&page==b.page&&commentary==b.commentary&&history==b.history&&anchor==b.anchor&&buffers==b.buffers&&calls==b.calls;}
};
template<class Handler> Trace run(Handler handler,const std::string &uri,bool clicked,const char *main,bool commentary,int notebook,bool nested) {
 calls.clear();settings.MainWindowModule=const_cast<char *>(main);settings.special_anchor=nullptr;settings.comm_showing=commentary;settings.addhistoryitem=false;page=notebook;
 // Retained hover temporarily writes into the URI: give each run its own mutable buffer.
 std::vector<char> input(uri.begin(),uri.end());input.push_back(0);
 recurse=nested?std::function<int()>([&]{return handler(input.data(),true);}):std::function<int()>();
 const int result=handler(input.data(),clicked);recurse={};
 check(std::string(input.data())==uri,"Retained hover restores its temporary URI separator");
 Trace trace{result,page,bool(settings.comm_showing),bool(settings.addhistoryitem),settings.special_anchor!=nullptr,live_buffers.size(),calls};
 // The frozen multi-reference early return leaks its split buffer. Record that
 // behavior but reclaim it after each trace so characterization is bounded.
 for(auto *p:live_buffers)g_strfreev(p);live_buffers.clear();settings.special_anchor=nullptr;
 return trace;
}
int main(){try {
 // Exhaust short strings against the literal retained split, including empty
 // anchors, repeated markers, UTF-8 bytes and hash-after-bang precedence.
 std::size_t anchor_cases=0;
 const std::string alphabet="ab/#!";
 std::function<void(std::string,int)> anchors = [&](std::string input,int remaining) {
  std::vector<char> expected(input.begin(),input.end()), actual_key=expected;
  expected.push_back(0);actual_key.push_back(0);
  char *old=std::strchr(expected.data(),'#');
  if(!old)old=std::strchr(expected.data(),'!');
  if(old)*old++='\0';
  char *now=xiphos::core::passage_uri_anchor(actual_key.data());
  check(bool(old)==bool(now),"Anchor presence differs");
  check(expected==actual_key,"Anchor split changes retained bytes");
  if(old)check(old-expected.data()==now-actual_key.data() && std::strcmp(old,now)==0,"Anchor ownership/offset differs");
  ++anchor_cases;
  if(remaining)for(char c:alphabet)anchors(input+c,remaining-1);
 };
 anchors("",6);anchors("café/文字",2);
 std::size_t comparisons=0;
 for(const char *main:{"Text","Commentary","Missing"})for(const char *module:{"","Text","T","文字 café","Commentary","Percom","Dictionary","D","Book","Prayer","Missing","Gone"})
 for(const char *reference:{"John3:16","John3:16-18","John3:16;17","John3:16,17","John3:16#tag","John3:16!note","John3:16!earlier#later","café"})
 for(bool clicked:{false,true})for(bool commentary:{false,true})for(int notebook:{0,1})for(bool has_tag:{false,true})for(bool renders:{false,true})for(bool navigates:{false,true}) {
  tag=has_tag;render=renders;nav=navigates;const std::string uri=std::string("sword://")+module+"/"+reference;
  const auto expected=run(frozen::sword_uri,uri,clicked,main,commentary,notebook,false);
  check(run(actual::sword_uri,uri,clicked,main,commentary,notebook,false)==expected,"Complete URI callback trace differs from frozen behavior");++comparisons;
 }
 for(const auto *uri:{"sword://Text/John3:16","sword://Missing/John3:16","sword://Commentary/John3:16","sword://Text"}) {
  tag=false;render=true;nav=true;
  const auto expected=run(frozen::sword_uri,uri,true,"Text",true,1,true);
  check(run(actual::sword_uri,uri,true,"Text",true,1,true)==expected,"URI fallback/reentry trace differs");++comparisons;
 }
 const auto missing=run(actual::sword_uri,"sword://Missing/John3:16",true,"Text",false,0,false);
 check(missing.history&&missing.buffers==0,"Unknown URI routes through main navigation and records history");
 const auto range=run(actual::sword_uri,"sword://T/John3:16-18",true,"Text",false,0,false);
 check(!range.history&&range.buffers==1,"Bible range routes sidebar before final history flag and retains observed legacy split leak");
 std::cout<<"Complete retained URI/preview dispatch: "<<comparisons<<" frozen/actual callback and state traces agree; "<<anchor_cases<<" anchor splits agree\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
