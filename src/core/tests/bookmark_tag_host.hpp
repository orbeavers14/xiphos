// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "bookmark_tree_host.hpp"
#include "xiphos/core/bookmark_tags.hpp"
#include "xiphos/core/reference_list.hpp"
#include <rawtext.h>
#include <versekey.h>
#include <map>
#include <swmgr.h>
#include "xiphos/core/presentation.hpp"
using namespace sword;
using GtkTreeStore=void;
static struct Settings{bool tag_colorize=true;const char *MainWindowModule="Text",*currentverse="John3:16";}settings;
static bool gtk_tree_model_get_iter_first(void *,GtkTreeIter *iter){if(rows.empty())return false;iter->index=0;return true;}
static bool gtk_tree_model_iter_children(void *,GtkTreeIter *child,GtkTreeIter *parent){for(size_t i=0;i<rows.size();++i)if(rows[i].parent==parent->index){child->index=i;return true;}return false;}
static bool gtk_tree_model_iter_next(void *,GtkTreeIter *iter){const auto parent=rows.at(iter->index).parent;for(size_t i=iter->index+1;i<rows.size();++i)if(rows[i].parent==parent){iter->index=i;return true;}return false;}
static void gtk_tree_model_get(void *,GtkTreeIter *iter,...){va_list args;va_start(args,iter);int field;const auto &row=rows.at(iter->index);while((field=va_arg(args,int))!=-1){auto **out=va_arg(args,gchar **);const std::optional<std::string> *value=nullptr;switch(field){case COL_COLOR:value=&row.color;break;case COL_KEY:value=&row.key;break;case COL_MODULE:value=&row.module;break;case COL_DESCRIPTION:value=&row.description;break;default:throw std::runtime_error("Unexpected tag read column");}*out=*value?g_strdup((*value)->c_str()):nullptr;}va_end(args);}
enum{TEXT_TYPE=1,COMMENTARY_TYPE=2};
struct TagCall{std::string module,key,context;bool operator==(const TagCall &other)const{return module==other.module&&key==other.key&&context==other.context;}};
struct BackEnd{
 SWModule &module;std::vector<TagCall> calls;SWMgr *catalog=nullptr;
 int module_type(const char *id){if(catalog){auto *m=catalog->getModule(id);if(!m)return 0;return std::string(m->getType())=="Biblical Texts"?TEXT_TYPE:regular_commentary(m->getType(),m->getConfigEntry("ModDrv"))?COMMENTARY_TYPE:0;}return !strcmp(id,"Text")?TEXT_TYPE:!strcmp(id,"Notes")?COMMENTARY_TYPE:0;}
 GList *parse_verse_list(const char *id,const char *key,char *context){calls.push_back({id,key,context});GList *list=nullptr;for(const auto &reference:parse_reference_list(module,key,context).references)list=g_list_append(list,g_strdup(reference.c_str()));return list;}
};
static BackEnd *backend;
namespace frozen_tags {
void *model=reinterpret_cast<void *>(1);
static GHashTable *tag_color_map=nullptr;
#include "legacy_bookmark_tag_map.inc"
}
namespace actual_tags {
void *model=reinterpret_cast<void *>(1);
static GHashTable *tag_color_map=nullptr;
#include "legacy_bookmark_tag_map_actual.inc"
}
static std::map<std::string,std::string> copied_map(GHashTable *map){std::map<std::string,std::string> result;GHashTableIter iter;gpointer key,value;g_hash_table_iter_init(&iter,map);while(g_hash_table_iter_next(&iter,&key,&value))result.emplace(static_cast<char *>(key),static_cast<char *>(value));return result;}
static std::map<std::string,std::string> planned_map(const std::vector<BookmarkTagCandidate> &candidates,RawText &module,std::vector<TagCall> &calls){std::map<std::string,std::string> result;for(const auto &candidate:candidates){if(candidate.module&&!candidate.module->empty()&&*candidate.module!="Text"&&*candidate.module!="Notes")continue;
 calls.push_back({"Text",candidate.key,settings.currentverse});const auto parsed=parse_reference_list(module,candidate.key,settings.currentverse);
 auto *escaped=candidate.description?g_uri_escape_string(candidate.description->c_str(),nullptr,TRUE):nullptr;
 for(const auto &reference:parsed.references){VerseKey key;key.setText(reference.c_str());const std::string osis=std::string(key.getOSISBookName())+"."+std::to_string(key.getChapter())+"."+std::to_string(key.getVerse());result[osis]+=candidate.color+"-"+candidate.foreground+"-"+(escaped?escaped:"")+"@:@:@";}
 g_free(escaped);
 }return result;}
