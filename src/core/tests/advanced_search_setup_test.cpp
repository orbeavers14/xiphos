// SPDX-License-Identifier: GPL-2.0-or-later
// Exercises actual retained setup/export/bookmark routines with UI/storage
// boundaries stubbed. No SDK corpus, GUI acceptance or file writes are implied.
#include "xiphos/core/export_item.h"
#include <glib.h>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstring>
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
struct Widget{bool active=false;const char *text="";};
Widget custom,last,range_widget,entry,regexp,phrase,words,indexed;
struct Widgets{Widget *rb_custom_range,*rb_last,*combo_range,*search_entry,*rb_regexp,*rb_exact_phrase,*rb_words,*rb_optimized;}search1{&custom,&last,&range_widget,&entry,&regexp,&phrase,&words,&indexed};
#define GTK_TOGGLE_BUTTON(value) (value)
#define GTK_ENTRY(value) (value)
#define GTK_BIN(value) (value)
#define _(value) (value)
gboolean gtk_toggle_button_get_active(Widget *widget){return widget->active;}
Widget *gtk_bin_get_child(Widget *widget){return widget;}
const gchar *gtk_entry_get_text(Widget *widget){return widget->text;}
const int TEXT_TYPE=0,COMMENTARY_TYPE=1;
std::vector<std::string> events;
struct Backend{
 int count=7;std::string scope="last";
 void clear_scope(){events.push_back("clear_scope");scope="none";}
 void clear_search_list(){events.push_back("clear_list");count=0;}
 int module_type(char *module){events.push_back(std::string("type:")+module);return std::string(module)=="Text"?TEXT_TYPE:std::string(module)=="Commentary"?COMMENTARY_TYPE:9;}
 void set_range(char *module,char *range){events.push_back(std::string("range:")+(module?module:"NULL")+":"+range);}
 void set_scope2range(){events.push_back("use_range");scope="range";}
 void set_scope2last_search(){events.push_back("use_last");scope="last";}
}owner,*backend=&owner,*backendSearch=&owner;
bool found_range=true;char original_range[]="John1:1-John1:3";
const gchar *xml_get_list_from_label(const char *section,const char *item,const char *label){events.push_back(std::string("xml:")+section+":"+item+":"+label);return found_range?original_range:nullptr;}
#include "legacy_advanced_search_setup_actual.inc"
struct RESULTS{gchar *module,*key;};
GList *list_for_bookmarking=nullptr;
gboolean main_is_Bible_key(const char *,const char *key){return std::string(key)!="skip";}
void item(GString *out,const gchar *description,const gchar *module,const gchar *key,gboolean scripture,bool html){
 events.push_back(std::string(html?"html:":"plain:")+module+":"+key+(scripture?":body":":keys"));
 char *formatted=xiphos_export_item(description,module,key,scripture?"Original body":nullptr,html?1:0);check(formatted,"Format characterized export row");g_string_append(out,formatted);xiphos_export_item_free(formatted);
}
void gui_set_html_item(GString *out,const gchar *description,const gchar *module,const gchar *key,gboolean scripture){item(out,description,module,key,scripture,true);}
void gui_set_plain_text_item(GString *out,const gchar *description,const gchar *module,const gchar *key,gboolean scripture){item(out,description,module,key,scripture,false);}
#include "legacy_advanced_search_export_actual.inc"
struct Settings{const char *searchText;}settings{"Original query"};
void gui_bookmark_dialog(const char *name,const char *module,const char *keys){events.push_back(std::string("bookmark:")+(module?module:"NULL")+":"+keys+":"+name);}
#include "legacy_advanced_search_bookmarks_actual.inc"
struct Lists {
 GList *base=nullptr;
 explicit Lists(const std::vector<std::vector<std::pair<std::string,std::string>>> &groups){
  for(const auto &group:groups){GList *items=nullptr;for(const auto &entry:group){auto *value=g_new(RESULTS,1);value->module=g_strdup(entry.first.c_str());value->key=g_strdup(entry.second.c_str());items=g_list_append(items,value);}base=g_list_append(base,items);}
  list_for_bookmarking=base;
 }
 ~Lists(){for(auto *group=base;group;group=group->next){auto *items=static_cast<GList *>(group->data);for(auto *node=items;node;node=node->next){auto *value=static_cast<RESULTS *>(node->data);g_free(value->module);g_free(value->key);g_free(value);}g_list_free(items);}g_list_free(base);list_for_bookmarking=nullptr;}
};
int main(){try{
 range_widget.text="Original range";entry.text="Original <query>";
 auto setup=[&](std::vector<const char *> modules,bool use_range,bool use_last,bool xml){
  GList *list=nullptr;for(const auto *module:modules)list=g_list_append(list,const_cast<char *>(module));
  owner.count=7;owner.scope="last";custom.active=use_range;last.active=use_last;found_range=xml;events.clear();set_up_dialog_search(list);g_list_free(list);
 };
 setup({"Text","Commentary"},true,false,true);
 check(events==std::vector<std::string>{"clear_scope","type:Text","type:Commentary","clear_list","xml:ranges:range:Original range","range:Text:John1:1-John1:3","use_range"}&&owner.scope=="range"&&!owner.count,"Valid advanced range uses first selected module after clearing old scope/list");
 setup({"Commentary","Text"},true,true,true);check(events[5]=="range:Commentary:John1:1-John1:3"&&owner.scope=="range","First selected provider and custom-over-last radio precedence are preserved");
 setup({"Text","Dictionary","Commentary"},true,true,true);check(events==std::vector<std::string>{"clear_scope","type:Text","type:Dictionary"}&&owner.scope=="none"&&owner.count==7,"Mixed family disables custom range globally without choosing last or clearing stored keys");
 setup({"Alias","Text"},true,false,true);check(events==std::vector<std::string>{"clear_scope","type:Alias"},"Range setup sees raw selected names before later abbreviation resolution");
 setup({"Text"},true,false,false);check(events==std::vector<std::string>{"clear_scope","type:Text","clear_list","xml:ranges:range:Original range"}&&owner.scope=="none"&&!owner.count,"Missing saved range clears stored keys without selecting bounds");
 setup({"Text"},false,true,true);check(events==std::vector<std::string>{"clear_scope","use_last"}&&owner.count==7,"Last mode reuses stored scope without type lookup");
 setup({"Text"},false,false,true);check(events==std::vector<std::string>{"clear_scope","clear_list"}&&!owner.count,"Unbounded setup clears old stored keys");
 setup({},true,false,true);check(events==std::vector<std::string>{"clear_scope","clear_list","xml:ranges:range:Original range","range:NULL:John1:1-John1:3","use_range"},"Empty custom module list attempts NULL provider in legacy setup; new batch rejects empty input instead");
 for(bool html:{false,true})for(bool scripture:{false,true}){
  Lists lists({{{"Text","John1:1"},{"Text","John1:2"}},{{"Commentary","John1:3"},{"Commentary","John1:4"}}});
  GString *out=g_string_new("");events.clear();check(main_export_current_adv_search(out,html,scripture),"Eligible export reports true");
  check(events==std::vector<std::string>{std::string(html?"html:":"plain:")+"Text:John1:2"+(scripture?":body":":keys"),std::string(html?"html:":"plain:")+"Commentary:John1:4"+(scripture?":body":":keys")},"Current export uses first eligible result only as header, formatting remaining rows in each group");
  const std::string output=out->str;
  check(output.find("John1:1")==std::string::npos&&output.find("John1:3")==std::string::npos&&output.find("Original <query>")!=std::string::npos,"Current first-match omission and raw header text are recorded explicitly");
  if(html)check(output.find("<b>Original <query></b><br/>Attribute<br/><br/><ul><b>Text</b>")!=std::string::npos&&output.find("</body>")==std::string::npos,"Orchestrator has raw header/no body footer; caller supplies final document close");
  g_string_free(out,TRUE);
 }
 {
  Lists lists({{{"Text","skip"},{"Text","John1:1"}}});GString *out=g_string_new("prefix");events.clear();
  check(main_export_current_adv_search(out,FALSE,FALSE)&&events.empty()&&std::string(out->str)=="prefixOriginal <query>\nAttribute\n\nText\n","Only first eligible result produces a header with no item, even after skipped keys");g_string_free(out,TRUE);
 }
 {
  Lists lists({{},{{"Text","skip"}}});GString *out=g_string_new("");events.clear();
  check(!main_export_current_adv_search(out,TRUE,TRUE)&&events.empty()&&std::string(out->str).find("</ul></ul>")!=std::string::npos,"Empty/ineligible groups still append closing lists and report false");g_string_free(out,TRUE);
 }
 const std::vector<std::string> labels{"Regular expression","Exact phrase","Multi-word","Optimized (\"Lucene\")","Attribute"};
 for(unsigned mode=0;mode<5;++mode){
  regexp.active=mode==0;phrase.active=mode<=1;words.active=mode<=2;indexed.active=mode<=3;
  Lists lists({{{"Text","John1:1"}}});GString *out=g_string_new("");main_export_current_adv_search(out,FALSE,FALSE);check(std::string(out->str)=="Original <query>\n"+labels[mode]+"\n\nText\n","Export labels follow legacy radio precedence");g_string_free(out,TRUE);
 }
 {
  Lists lists({{{"Text","John1:1"},{"Text","skip"},{"Text","John1:1"},{"Text","John1:2"}},{{"Commentary","John1:3"}}});events.clear();main_save_current_adv_search_as_bookmarks();
  check(events==std::vector<std::string>{"bookmark:Text:John1:1; John1:1; John1:2:Search result Text: Original query","bookmark:Commentary:John1:3:Search result Commentary: Original query"}&&!list_for_bookmarking,"Bookmark preparation includes first key/duplicates with semicolon spacing but advances legacy global list to NULL");
 }
 std::cout<<"Actual advanced range setup, export orchestration quirks and bookmark preparation characterized without SDK/file writes passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
