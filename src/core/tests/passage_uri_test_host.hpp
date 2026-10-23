// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <glib.h>
#include <algorithm>
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
static std::function<void(const char *)> display_output;
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
void main_display_bible(const char *module,const char *key) { call("bible",module,key);call("anchor",settings.special_anchor);if(recurse)calls.push_back("reentry:"+std::to_string(recurse()));if(display_output)display_output("bible"); }
void main_display_commentary(const char *module,const char *key) { call("commentary",module,key);call("anchor",settings.special_anchor);if(display_output&&settings.comm_showing)display_output("commentary"); }
void main_display_dictionary(const char *module,const char *key) { call("dictionary",module,key);if(display_output)display_output("dictionary"); }
void main_display_book(const char *module,const char *key) { call("book",module,key);call("anchor",settings.special_anchor);if(display_output)display_output("book"); }
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
