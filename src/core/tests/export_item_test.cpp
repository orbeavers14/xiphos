// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/export_item.h"
#include "xiphos/core/export_item.hpp"
#include <glib.h>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
const int TEXT_TYPE=1,COMMENTARY_TYPE=2,OTHER_TYPE=3;
struct Settings{const char *MainWindowModule;}settings={"Text"};
std::vector<std::string> calls;
bool null_body=false;
std::string rendered="<b>Original café &amp; α</b>",stripped="Original café & α";
gboolean main_is_Bible_key(const gchar *module,const gchar *key){calls.push_back(std::string("key:")+module+":"+key);return std::string(key)!="invalid";}
gint main_get_mod_type(const gchar *module){calls.push_back(std::string("type:")+(module?module:"NULL"));return module&&std::string(module)=="Text"?TEXT_TYPE:module&&std::string(module)=="Commentary"?COMMENTARY_TYPE:OTHER_TYPE;}
gchar *main_get_rendered_text(const gchar *module,const gchar *key){calls.push_back(std::string("html:")+module+":"+key);return null_body?nullptr:g_strdup(rendered.c_str());}
gchar *main_get_striptext(const gchar *module,gchar *key){calls.push_back(std::string("plain:")+module+":"+key);return null_body?nullptr:g_strdup(stripped.c_str());}
#define get_module frozen_get_module
#define gui_set_html_item frozen_html_item
#define gui_set_plain_text_item frozen_plain_item
#include "legacy_export_items.inc"
#undef get_module
#undef gui_set_html_item
#undef gui_set_plain_text_item
#include "legacy_export_items_actual.inc"
int main(){try{
 for(const std::string &description:std::vector<std::string>{"","Original","<>&\"'","café λόγος\nnext",std::string(8192,'x')})
 for(const std::string &module:std::vector<std::string>{"Text","Commentary","Dictionary","","T"})
 for(const std::string &key:std::vector<std::string>{"John1:1","invalid","<key>&\"'","α\tβ"})
 for(bool scripture:{false,true})for(bool html:{false,true})for(int body=0;body<3;++body){
  const bool empty=body==1;null_body=body==2;
  rendered=empty?"":"<b>Original café &amp; α</b>";stripped=empty?"":"Original café & α";
  GString *before=g_string_new("prefix"),*after=g_string_new("prefix");
  calls.clear();if(html)frozen_html_item(before,description.c_str(),module.c_str(),key.c_str(),scripture);else frozen_plain_item(before,description.c_str(),module.c_str(),key.c_str(),scripture);const auto expected_calls=calls;
  calls.clear();if(html)gui_set_html_item(after,description.c_str(),module.c_str(),key.c_str(),scripture);else gui_set_plain_text_item(after,description.c_str(),module.c_str(),key.c_str(),scripture);
  check(std::string(before->str)==after->str&&expected_calls==calls,"Actual retained full helpers preserve frozen bytes and SDK/fallback call order for all format/source choices");
  g_string_free(before,TRUE);g_string_free(after,TRUE);
 }
 const auto html=format_export_item("<>&\"'","Text&","John<1>","<b>Original</b>",true);
 check(html=="<li>&lt;&gt;&amp;&quot;&apos;<ul><li>Text&amp; John&lt;1&gt;</li><li><b>Original</b></li></ul><br/></li>","HTML metadata uses exact GLib escaping while already-rendered body retains markup");
 check(format_export_item("desc","Text","key","",true,true).find("<li></li>")!=std::string::npos&&format_export_item("desc","Text","key","",true,false).find("<li></li>")==std::string::npos,"Absent HTML scripture differs from requested empty rendered body");
 check(format_export_item("desc","Text","key","",false)=="\tdesc\n\t\tText key\n\t\t\n\n","Plain legacy tabs, newlines and empty body remain exact");
 for(int field=0;field<4;++field){std::string fields[]={"desc","Text","key","body"};fields[field]=std::string("bad\0tail",8);try{format_export_item(fields[0],fields[1],fields[2],fields[3],true);throw std::runtime_error("Expected embedded NUL rejection");}catch(const std::invalid_argument &){}}
 check(!xiphos_export_item(nullptr,"Text","key",nullptr,1)&&!xiphos_export_item("desc",nullptr,"key",nullptr,1)&&!xiphos_export_item("desc","Text",nullptr,nullptr,1)&&!xiphos_export_item("desc","Text","key",nullptr,2),"Strict C NULL/format contracts");
 char *owned=xiphos_export_item("desc","Text","key",nullptr,1);check(owned&&std::string(owned)==format_export_item("desc","Text","key","",true,false),"C owns the same formatter bytes");xiphos_export_item_free(owned);xiphos_export_item_free(nullptr);
 std::cout<<"Shared export item formatting matches frozen full helper bytes, metadata escaping, optional bodies and module fallback calls passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
