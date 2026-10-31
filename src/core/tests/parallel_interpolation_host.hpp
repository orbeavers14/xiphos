// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/render.hpp"
#include "xiphos/core/entry_attribute.hpp"
#include "xiphos/core/navigation_key.hpp"
#include "xiphos/core/parallel.hpp"
#include <glib.h>
#include <swmgr.h>
#include <swmodule.h>
#include <versekey.h>
#include <rawtext.h>
#include <rawcom.h>
#include <rawld.h>
#include <markupfiltmgr.h>
#include <map>
#include <vector>
#include <memory>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <stdexcept>
using namespace sword;
enum { TEXT_TYPE=1,COMMENTARY_TYPE=2 };
static const char *sword_locale="en";
struct RenderedCell{std::string module,requested,matched,html;bool operator==(const RenderedCell &other)const{return module==other.module&&requested==other.requested&&matched==other.matched&&html==other.html;}};
static std::vector<RenderedCell> rendered;
static std::vector<std::string> calls;
static std::string copy(const char *s){return s?s:"<null>";}
static void call(const char *kind,const char *a=nullptr,const char *b=nullptr){calls.push_back(std::string(kind)+":"+copy(a)+":"+copy(b));}
static std::map<std::string,std::string> aliases={{"A","TextA"},{"Alt","TextAlt"},{"RTL","RtlText"},{"N","Notes"},{"Gone","Missing"}};
static struct Settings{gchar **parallel_list;bool alternation;gchar *bible_bg_color,*bible_text_color,*currentverse_color,*bible_verse_num_color;int verse_num_font_size,base_font_size,intCurVerse;} settings;
static bool realized=true;
static struct Widgets{void *notebook_bible_parallel;}widgets;
static gchar white[]="#FFFFFF",grey[]="#606060",black[]="#000000",green[]="#00AA00",red[]="#AA0000",*bg_choice;
static SWBuf unknown_parallel("Unknown parallel module: ");
struct MOD_FONT{gchar *old_font;int old_font_size_value;};
static MOD_FONT *get_font(const char *id){call("font",id);return new MOD_FONT{g_strdup("Original Serif"),2};}
static void free_font(MOD_FONT *font){g_free(font->old_font);delete font;}
static bool main_is_mod_rtol(const char *id){call("direction",id);return std::string(id)=="RtlText";}
static const char *main_abbrev_to_name(const char *id){call("alias",id);const auto found=aliases.find(copy(id));return found==aliases.end()?nullptr:found->second.c_str();}
static void gui_generic_warning(const char *text){call("warning",text);}
static const gchar *main_url_encode(const char *key){call("url",key);return g_uri_escape_string(key,nullptr,false);}
static gchar *main_format_number(int verse){return g_strdup_printf("%d",verse);}
static bool gtk_widget_get_realized(void *){call("realized");return realized;}
#define GTK_WIDGET(value) (value)
struct BackEnd{
 SWMgr &manager;SWModule *display_mod=nullptr;
 explicit BackEnd(SWMgr &m):manager(m){}
 SWModule *get_SWModule(const char *id){call("module",id);return id?manager.getModule(id):nullptr;}
 bool is_module(const char *id){return get_SWModule(id)!=nullptr;}
 int module_type(const char *id){auto *module=get_SWModule(id);return module&&std::string(module->getType())=="Biblical Texts"?TEXT_TYPE:COMMENTARY_TYPE;}
 char *get_valid_key(const char *,const char *);
 char *key_get_book(const char *,const char *);int key_get_chapter(const char *,const char *);int key_get_verse(const char *,const char *);
 void set_module_key(const char *id,const char *key){call("position",id,key);display_mod=get_SWModule(id);if(display_mod)display_mod->setKey(key);}
 char *get_entry_attribute(const char *a,const char *b,const char *c){call("attribute",a,c);const auto value=xiphos::core::entry_attribute(*display_mod,a,b,c,true);return value.empty()?nullptr:g_strdup(value.c_str());}
 char *render_this_text(const char *id,const char *text){call("heading-render",id,text);auto *module=get_SWModule(id);return module?g_strdup(module->renderText(text).c_str()):nullptr;}
 char *get_render_text(const char *id,const char *key){call("render",id,key);auto *module=get_SWModule(id);if(!module)return nullptr;const auto text=xiphos::core::render_entry(*module,key);call("matched",id,module->getKeyText());rendered.push_back({id,key,module->getKeyText(),text});return g_strdup(text.c_str());}
};
#include "legacy_navigation_key.inc"
#include "legacy_parallel_coordinates.inc"
static BackEnd *backend,*backend_p;
static int main_get_mod_type(const char *id){call("type",id);return backend->module_type(id);}
namespace frozen {
#include "legacy_parallel_interpolation.inc"
}
namespace actual {
#include "legacy_parallel_interpolation_actual.inc"
}
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static void fixture(const std::string &root){
 // This disposable fixture retains configure-time conf files across runs. Drop
 // only its SDK-generated catalog cache when regenerating module data/metadata.
 std::remove((root+"/mods.d/modules-conf.cache").c_str());
 for(const auto *id:{"TextA","TextAlt","RtlText","Notes","Dictionary"}){
 const std::string path=root+"/"+id,canon=std::string(id)=="TextAlt"?"KJVA":std::string(id)=="RtlText"?"Synodal":"KJV";const bool note=std::string(id)=="Notes",dictionary=std::string(id)=="Dictionary";
 if(dictionary){const auto entries=path+"/entries";check(RawLD::createModule(entries.c_str())==0,"Create original non-verse fixture");RawLD module(entries.c_str());module.setKey("alpha");module.setEntry("Original dictionary fixture.");continue;}
 check((note?RawCom::createModule(path.c_str()):RawText::createModule(path.c_str(),canon.c_str()))==0,"Create original parallel module");
 std::unique_ptr<SWModule> module;if(note)module.reset(new RawCom(path.c_str()));else module.reset(new RawText(path.c_str(),nullptr,nullptr,nullptr,ENC_UTF8,DIRECTION_LTR,FMT_OSIS,"en",canon.c_str()));
 for(const auto *ref:{"Genesis1:1","John3:16","John3:17","Psalms3:1","Psalms3:2"}){
  module->setKey(ref);const auto body=std::string("<title type=\"section\" subType=\"x-preverse\">Original ")+id+" heading.</title><p>Original "+id+" parallel prose for "+ref+".</p>";module->setEntry(body.c_str());
 }
 if(canon=="KJVA"){module->setKey("Tob1:1");module->setEntry("<p>Original TextAlt Tobit prose.</p>");}
}}
using Handler=void (*)(SWModule *,char *,SWBuf &,gchar *,gint,gint);
struct Trace{std::vector<std::string> calls;std::string html;int selected;std::vector<RenderedCell> cells;bool operator==(const Trace &other)const{return calls==other.calls&&html==other.html&&selected==other.selected&&cells==other.cells;}};
static Trace run(Handler handler,BackEnd &owner,const std::vector<std::string> &ids,const char *reference,bool visible,bool alternating,bool headings,const char *locale){
 for(const auto &entry:owner.manager.Modules){auto &module=*entry.second;module.getEntryAttributes().clear();module.setProcessEntryAttributes(true);auto *key=dynamic_cast<VerseKey *>(module.getKey());if(key){key->setAutoNormalize(true);key->setIntros(false);key->setText("John3:23");key->popError();}}
 owner.manager.setGlobalOption("Headings",headings?"On":"Off");
 std::vector<gchar *> list;for(const auto &id:ids)list.push_back(const_cast<char *>(id.c_str()));list.push_back(nullptr);
 settings={list.data(),alternating,white,black,green,red,1,2,99};realized=visible;sword_locale=locale;backend=backend_p=&owner;owner.display_mod=nullptr;
 const auto first=aliases.find(ids.front());const auto control=first==aliases.end()?ids.front():first->second;auto *module=owner.manager.getModule(control.c_str());check(module,"Explicit control exists");calls.clear();rendered.clear();SWBuf text;
 std::vector<char> input(reference,reference+std::strlen(reference)+1);handler(module,const_cast<char *>(control.c_str()),text,input.data(),static_cast<int>(ids.size()),xiphos::core::parallel_layout(ids.size()).column_percent);
 return {calls,text.c_str(),settings.intCurVerse,rendered};
}
