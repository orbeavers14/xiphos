// SPDX-License-Identifier: GPL-2.0-or-later
// Item templates from src/gtk/export_bookmarks.c, Xiphos Developer Team (2003-2026).
#include "xiphos/core/export_item.h"
#include "xiphos/core/export_item.hpp"
#include <glib.h>
#include <memory>
#include <stdexcept>
namespace xiphos { namespace core {
namespace {
void validate(const std::string &text){if(text.find('\0')!=std::string::npos)throw std::invalid_argument("Export item contains NUL");}
std::string escaped(const std::string &text){
  const std::unique_ptr<gchar,decltype(&g_free)> result(g_markup_escape_text(text.c_str(),-1),g_free);
  return result.get();
}
}
std::string format_export_item(const std::string &description,const std::string &module,
                              const std::string &key,const std::string &scripture,bool html,bool scripture_present){
  validate(description);validate(module);validate(key);validate(scripture);
  if(!html)return "\t"+description+"\n\t\t"+module+" "+key+"\n\t\t"+(scripture_present?scripture:std::string{})+"\n\n";
  return "<li>"+escaped(description)+"<ul><li>"+escaped(module)+" "+escaped(key)+"</li>"+
    (scripture_present?"<li>"+scripture+"</li>":std::string{})+"</ul><br/></li>";
}
}}
extern "C" {
char *xiphos_export_item(const char *description,const char *module,const char *key,const char *scripture,int html){
  if(!description||!module||!key||(html!=0&&html!=1))return nullptr;
  try {return g_strdup(xiphos::core::format_export_item(description,module,key,scripture?scripture:"",html!=0,scripture!=nullptr).c_str());}
  catch(...){return nullptr;}
}
void xiphos_export_item_free(char *text){g_free(text);}
}
