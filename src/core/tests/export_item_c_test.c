/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <glib.h>
#include <string.h>
#include "xiphos/core/export_item.h"
struct ExportSettings { const char *MainWindowModule; } settings={"Text"};
gboolean main_is_Bible_key(const gchar *module,const gchar *key){return module&&key;}
gint main_get_mod_type(const gchar *module){return module&&strcmp(module,"Text")==0?0:8;}
gchar *main_get_rendered_text(const gchar *module,const gchar *key){(void)module;(void)key;return g_strdup("<b>Original</b>");}
gchar *main_get_striptext(const gchar *module,gchar *key){(void)module;(void)key;return g_strdup("Original");}
void gui_set_html_item(GString *,const gchar *,const gchar *,const gchar *,gboolean);
void gui_set_plain_text_item(GString *,const gchar *,const gchar *,const gchar *,gboolean);
int main(void){
 GString *out=g_string_new("");
 gui_set_html_item(out,"d&","Text","John1:1",TRUE);
 int good=strcmp(out->str,"<li>d&amp;<ul><li>Text John1:1</li><li><b>Original</b></li></ul><br/></li>")==0;
 g_string_assign(out,"");gui_set_plain_text_item(out,"d&","Text","John1:1",TRUE);
 good=good&&strcmp(out->str,"\td&\n\t\tText John1:1\n\t\tOriginal\n\n")==0;
 g_string_assign(out,"");gui_set_html_item(out,"d&","Text","John1:1",FALSE);
 good=good&&strcmp(out->str,"<li>d&amp;<ul><li>Text John1:1</li></ul><br/></li>")==0;
 g_string_free(out,TRUE);
 char *owned=xiphos_export_item("d","Text","key",NULL,0);
 good=good&&owned&&strcmp(owned,"\td\n\t\tText key\n\t\t\n\n")==0;
 xiphos_export_item_free(owned);xiphos_export_item_free(NULL);
 return good?0:1;
}
