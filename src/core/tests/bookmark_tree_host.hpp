// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/bookmarks.hpp"
#include <glib.h>
#include <libxml/parser.h>
#include <cstdarg>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
using GtkTreeView=void;
struct GtkTreeIter{std::size_t index;};struct GtkTreePath{};
struct BOOKMARK_DATA{gchar *caption,*key,*module,*module_desc,*description,*color;int is_leaf;void *opened,*closed;};
static struct Pixbufs{void *pixbuf_opened=nullptr,*pixbuf_closed=nullptr,*pixbuf_helpdoc=nullptr;}pixbufs,*bm_pixbufs=&pixbufs;
static void *model=nullptr;
enum{COL_OPEN_PIXBUF,COL_CLOSED_PIXBUF,COL_CAPTION,COL_KEY,COL_MODULE,COL_MODULE_DESC,COL_DESCRIPTION,COL_COLOR};
#define GTK_TREE_STORE(x) (x)
#define GTK_TREE_MODEL(x) (x)
#define _(x) (x)
struct Row{std::optional<std::size_t> parent;std::optional<std::string> label,key,module,module_description,description,color;bool operator==(const Row &o)const{return parent==o.parent&&label==o.label&&key==o.key&&module==o.module&&module_description==o.module_description&&description==o.description&&color==o.color;}};
static std::vector<Row> rows;
static void gtk_tree_store_append(void *,GtkTreeIter *iter,GtkTreeIter *parent){iter->index=rows.size();Row row;row.parent=parent?std::make_optional(parent->index):std::nullopt;rows.push_back(std::move(row));}
static void gtk_tree_store_set(void *,GtkTreeIter *iter,...){va_list args;va_start(args,iter);int field;auto &row=rows.at(iter->index);while((field=va_arg(args,int))!=-1){if(field==COL_OPEN_PIXBUF||field==COL_CLOSED_PIXBUF){(void)va_arg(args,void *);continue;}auto *text=va_arg(args,gchar *);auto copied=text?std::make_optional(std::string(text)):std::nullopt;switch(field){case COL_CAPTION:row.label=std::move(copied);break;case COL_KEY:row.key=std::move(copied);break;case COL_MODULE:row.module=std::move(copied);break;case COL_MODULE_DESC:row.module_description=std::move(copied);break;case COL_DESCRIPTION:row.description=std::move(copied);break;case COL_COLOR:row.color=std::move(copied);break;default:throw std::runtime_error("Unexpected retained tree column");}}va_end(args);}
static GtkTreePath *gtk_tree_model_get_path(void *,GtkTreeIter *){return new GtkTreePath;}
static void gtk_tree_view_expand_to_path(void *,GtkTreePath *){}
static void gtk_tree_path_free(GtkTreePath *path){delete path;}
namespace frozen {
static xmlDocPtr bookmark_doc;
#include "legacy_bookmark_xml.inc"
#include "legacy_bookmark_tree.inc"
}
namespace actual {
static xmlDocPtr bookmark_doc;
#include "legacy_bookmark_xml_actual.inc"
#include "legacy_bookmark_tree_actual.inc"
}
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class F>static void rejects(F f){try{f();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected bookmark rejection");}
static std::vector<Row> load(const std::string &path,void (*call)(GtkTreeView *,const xmlChar *,GtkTreeIter *)){rows.clear();call(nullptr,BAD_CAST path.c_str(),nullptr);return rows;}
static std::string bytes(const std::filesystem::path &p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
