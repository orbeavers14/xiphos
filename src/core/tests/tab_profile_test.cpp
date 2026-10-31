// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/tab_profile.hpp"
#include <glib.h>
#include <glib/gstdio.h>
#include <libxml/parser.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
struct PASSAGE_TAB_INFO {
 gchar *text_mod,*commentary_mod,*dictlex_mod,*book_mod,*text_commentary_key,*dictlex_key,*book_offset;
 int comm_showing,showtexts,showpreview,showcomms,showdicts,showparallel;
 int history_items,current_history_item,first_back_click;void *paratab;
};
static GList *passage_list=nullptr;
static struct Settings {
 gchar *MainWindowModule,*CommWindowModule,*DictWindowModule,*book_mod,*currentverse,*dictkey,*gSwordDir;
 long book_offset;int browsing=1,showparatab=0,showtexts=1,showpreview=0,showcomms=1,showdicts=0;
}settings;
static struct Widgets{void *page=nullptr;}widgets;
static bool stop_refresh,sync_on;
static const char *default_tab_filename="unused";
static std::vector<std::string> events;
#define _(text) (text)
#define GTK_BOX(x) (x)
#define XI_message(...) ((void)0)
static void gui_generic_warning_modal(const char *){events.push_back("warning");}
static void gui_save_tabs(const char *){events.push_back("save");}
static void _save_off_tab(const char *){events.push_back("save-off");}
static void *gui_create_parallel_tab(){events.push_back("parallel");return reinterpret_cast<void *>(1);}
static void gtk_box_pack_start(void *,void *,int,int,int){events.push_back("pack");}
static void gtk_widget_hide(void *){events.push_back("hide");}
static void gui_parallel_tab_sync(char *){events.push_back("sync");}
static void main_add_tab_history_item(PASSAGE_TAB_INFO *){events.push_back("history");}
static void notebook_main_add_page(PASSAGE_TAB_INFO *){events.push_back("page");}
static void gui_recompute_view_menu_choices(){events.push_back("menu");}
static void set_current_tab(PASSAGE_TAB_INFO *){events.push_back("current");}
static void gui_reassign_strdup(gchar **target,const gchar *source){g_free(*target);*target=g_strdup(source);}
static int yes_no2true_false(const gchar *s){return s&&!strcmp(s,"yes");}
namespace frozen {
#include "legacy_tab_load.inc"
}
namespace actual {
#include "legacy_tab_load_actual.inc"
}
static const gchar *true_false2yes_no(int value){return value?"yes":"no";}
#define VERSION "fixture-version"
namespace frozen_save {
#include "legacy_tab_save.inc"
}
namespace actual_save {
#include "legacy_tab_save_actual.inc"
}
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class F>static void rejects(F f){try{f();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected tab profile rejection");}
static std::optional<std::string> copied(const char *s){return s?std::make_optional(std::string(s)):std::nullopt;}
struct Loaded {std::vector<SavedTab> tabs;std::vector<TabVisibility> visibility;std::vector<std::string> events;};
static Loaded load(const std::string &path,void (*call)(const char *)){
 settings={g_strdup("DefaultText"),g_strdup("DefaultNotes"),g_strdup("DefaultDict"),g_strdup("DefaultBook"),g_strdup("John1:1"),g_strdup("Key"),g_strdup("unused"),0,1,0,1,0,1,0};events.clear();call(path.c_str());
 Loaded result;result.events=events;
 for(auto *p=passage_list;p;p=p->next){auto *tab=static_cast<PASSAGE_TAB_INFO *>(p->data);SavedTab saved;
  saved.text_module=copied(tab->text_mod);saved.commentary_module=copied(tab->commentary_mod);saved.dictionary_module=copied(tab->dictlex_mod);saved.book_module=copied(tab->book_mod);saved.reference=copied(tab->text_commentary_key);saved.dictionary_key=copied(tab->dictlex_key);saved.book_offset_text=copied(tab->book_offset);
  saved.commentary_showing=tab->comm_showing?"yes":"no";saved.show_parallel=tab->showparallel?"yes":"no";result.tabs.push_back(std::move(saved));result.visibility.push_back({tab->showtexts!=0,tab->showpreview!=0,tab->showcomms!=0,tab->showdicts!=0});
  g_free(tab->text_mod);g_free(tab->commentary_mod);g_free(tab->dictlex_mod);g_free(tab->book_mod);g_free(tab->text_commentary_key);g_free(tab->dictlex_key);g_free(tab->book_offset);g_free(tab);
 }
 g_list_free(passage_list);passage_list=nullptr;
 for(auto *s:{settings.MainWindowModule,settings.CommWindowModule,settings.DictWindowModule,settings.book_mod,settings.currentverse,settings.dictkey,settings.gSwordDir})g_free(s);
 return result;
}
static void same(const SavedTab &a,const SavedTab &b){check(a.text_module==b.text_module&&a.commentary_module==b.commentary_module&&a.dictionary_module==b.dictionary_module&&a.book_module==b.book_module&&a.reference==b.reference&&a.dictionary_key==b.dictionary_key&&a.book_offset_text==b.book_offset_text,"Owned raw modules/keys/offset match complete retained tab load");}
static void visibility(TabVisibility a,TabVisibility b){check(a.texts==b.texts&&a.preview==b.preview&&a.commentaries==b.commentaries&&a.dictionaries==b.dictionaries,"All-four visibility fallback follows retained showtexts sentinel");}
int main(int argc,char **argv){try{
 check(argc==2,"Disposable tabs root required");const auto root=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(root);const auto file=root/"tabs.xml";
 std::size_t comparisons=0;
 for(const auto *flag:{"yes","no","YES","","anything"})for(bool showtexts:{false,true})for(bool preview:{false,true}){
  const auto tab=std::string("<tab text_mod='Text' commentary_mod='Notes' dictlex_mod='Dictionary' book_mod='Book' text_commentary_key='John3:16' dictlex_key='café &amp; key' book_offset=' 12tail' comm_showing='")+flag+"' showparallel='"+flag+"'"+(showtexts?std::string(" showtexts='")+flag+"'":"")+(preview?std::string(" showpreview='")+flag+"'":"")+" showcomms='no' showdicts='yes'/>";
  const std::string source="<Xiphos_Tabs Version='legacy'><tabs><!--keep-->"+tab+"<unknown/>"+tab+"</tabs><other/><tabs>"+tab+"</tabs></Xiphos_Tabs>";{std::ofstream out(file);out<<source;}
  const auto baseline=load(file.u8string(),frozen::gui_load_tabs),current=load(file.u8string(),actual::gui_load_tabs);const auto owned=read_saved_tab_session(file.u8string());
  check(owned.tabs.size()==3&&current.tabs.size()==3&&baseline.tabs.size()==3&&current.events==baseline.events,"Complete frozen/current loader trace and tab ordering");
  for(std::size_t i=0;i<owned.tabs.size();++i){same(owned.tabs[i],baseline.tabs[i]);same(current.tabs[i],baseline.tabs[i]);visibility(saved_tab_visibility(owned.tabs[i],{true,false,true,false}),baseline.visibility[i]);check(saved_tab_yes(owned.tabs[i].commentary_showing)==saved_tab_yes(baseline.tabs[i].commentary_showing)&&saved_tab_yes(owned.tabs[i].show_parallel)==saved_tab_yes(baseline.tabs[i].show_parallel),"Exact yes token for commentary/parallel");}
  check(owned.version=="legacy","Owned Version attribute");++comparisons;
 }
 const auto empty=parse_saved_tab_session("<Xiphos_Tabs><tabs><tab text_mod='' book_offset='-1'/><tab/></tabs></Xiphos_Tabs>");check(empty.tabs.size()==2&&empty.tabs[0].text_module==""&&!empty.tabs[1].text_module&&empty.tabs[0].book_offset_text=="-1","Empty/missing module and raw invalid offset remain distinct");
 check(parse_saved_tab_session("<Xiphos_Tabs/>").tabs.empty(),"Empty document does not manufacture fallback tabs");
 for(const auto &bad:std::vector<std::string>{"","<Xiphos/>","<Xiphos_Tabs>","<!DOCTYPE Xiphos_Tabs [<!ENTITY x 'hidden'>]><Xiphos_Tabs/>",std::string("<Xiphos_Tabs/>\0tail",18)})rejects([&]{parse_saved_tab_session(bad);});
 std::string many="<Xiphos_Tabs><tabs>";for(std::size_t i=0;i<maximum_saved_tabs;++i)many+="<tab/>";check(parse_saved_tab_session(many+"</tabs></Xiphos_Tabs>").tabs.size()==maximum_saved_tabs,"Exact tab count bound");rejects([&]{parse_saved_tab_session(many+"<tab/></tabs></Xiphos_Tabs>");});
 rejects([&]{parse_saved_tab_session("<Xiphos_Tabs><tabs><tab text_mod='"+std::string(maximum_saved_tab_text_bytes+1,'x')+"'/></tabs></Xiphos_Tabs>");});

 auto bytes=[](const std::filesystem::path &p){std::ifstream in(p,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});};
 const auto source="<Xiphos_Tabs custom='keep' Version='old'><!--keep--><tabs custom='section'><tab custom='first' text_mod='Old'><unknown/></tab><other/></tabs><tabs><tab custom='second'/><tab custom='surplus'/></tabs><unrelated/></Xiphos_Tabs>";
 SavedTabSession replacement;replacement.version="new&<>文字";SavedTab first;first.text_module="Text&<>文字";first.reference="John3:16";first.book_offset_text=" 12tail";first.show_texts="";first.show_preview="yes";replacement.tabs={first,SavedTab{}};
 const auto serialized=serialize_saved_tab_session(replacement,source);const auto roundtrip=parse_saved_tab_session(serialized);
 check(roundtrip.version==replacement.version&&roundtrip.tabs.size()==2&&roundtrip.tabs[0].text_module==first.text_module&&roundtrip.tabs[0].reference==first.reference&&roundtrip.tabs[0].show_texts==""&&!roundtrip.tabs[1].text_module,"Raw optional known attributes round trip with XML literals");
 for(const auto *kept:{"custom=\"keep\"","<!--keep-->","custom=\"section\"","custom=\"first\"","<unknown/>","<other/>","custom=\"second\"","<unrelated/>"})check(serialized.find(kept)!=std::string::npos,"Tab export preserves unrelated positional nodes/attributes");
 check(serialized.find("surplus")==std::string::npos&&serialized.find("text_mod=\"Old\"")==std::string::npos,"Surplus owned tabs and replaced known attributes removed");
 replacement.tabs.push_back(first);check(parse_saved_tab_session(serialize_saved_tab_session(replacement,serialized)).tabs.size()==3,"New owned tabs append after existing tabs in final section");
 const auto fresh=root/"fresh-tabs.xml";std::filesystem::remove(fresh);const auto saved=save_saved_tab_session(fresh.u8string(),replacement);check(saved.tabs.size()==3&&saved.version==replacement.version,"Missing selected file saves prepared owned snapshot");
 const auto permissions=std::filesystem::status(fresh).permissions(),before=permissions;const auto original=bytes(fresh);
 auto invalid=replacement;invalid.tabs[0].text_module=std::string("A\0B",3);rejects([&]{save_saved_tab_session(fresh.u8string(),invalid);});
 invalid=replacement;invalid.version=std::string(1,'\1');rejects([&]{save_saved_tab_session(fresh.u8string(),invalid);});
 invalid=replacement;invalid.tabs.resize(maximum_saved_tabs+1);rejects([&]{save_saved_tab_session(fresh.u8string(),invalid);});
 check(bytes(fresh)==original&&std::filesystem::status(fresh).permissions()==before,"Invalid export preserves source bytes and permissions");
 replacement.tabs.clear();const auto cleared=save_saved_tab_session(fresh.u8string(),replacement);check(cleared.tabs.empty()&&std::filesystem::status(fresh).permissions()==permissions,"Zero-tab explicit save removes all owned tab records");
 // Complete retained writer characterization on explicit existing paths.
 for(int count:{0,1,3})for(int flags:{0,1,2}){
  PASSAGE_TAB_INFO tab{};tab.text_mod=const_cast<char *>("Text&<>文字");tab.commentary_mod=const_cast<char *>("Notes");tab.dictlex_mod=const_cast<char *>("Dictionary");tab.book_mod=const_cast<char *>("Book");tab.text_commentary_key=const_cast<char *>("John3:16");tab.dictlex_key=const_cast<char *>("key");tab.book_offset=const_cast<char *>("42");tab.comm_showing=tab.showtexts=tab.showpreview=tab.showcomms=tab.showdicts=tab.showparallel=flags;
  for(int i=0;i<count;++i)passage_list=g_list_append(passage_list,&tab);
  {std::ofstream out(file);out<<"placeholder";}frozen_save::gui_save_tabs(file.u8string().c_str());const auto expected=bytes(file);
  actual_save::gui_save_tabs(file.u8string().c_str());check(bytes(file)==expected,"Complete frozen/current writer bytes match");
  const auto native=parse_saved_tab_session(expected);check(native.tabs.size()==static_cast<std::size_t>(count)&&native.version=="fixture-version","Read complete legacy saved file");
  const auto exported=parse_saved_tab_session(serialize_saved_tab_session(native));check(exported.tabs.size()==native.tabs.size()&&exported.version==native.version,"Native export round trips complete legacy writer metadata");
  for(std::size_t i=0;i<native.tabs.size();++i){same(native.tabs[i],exported.tabs[i]);check(saved_tab_yes(native.tabs[i].show_parallel)==(flags!=0),"Legacy any nonzero flag writes yes");visibility(saved_tab_visibility(native.tabs[i],{}),saved_tab_visibility(exported.tabs[i],{}));}
  g_list_free(passage_list);passage_list=nullptr;
 }
 std::cout<<"Tab profiles: "<<comparisons<<" complete frozen/current loader comparisons, raw attributes, visibility fallback and bounds passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
