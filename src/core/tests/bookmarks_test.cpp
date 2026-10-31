// SPDX-License-Identifier: GPL-2.0-or-later
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
int main(int argc,char **argv){try{
 check(argc==2,"Disposable bookmark root required");const auto root=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(root);const auto file=root/"bookmarks.xml";std::size_t comparisons=0;
 for(const auto *description:{"", "Original label", "文字 &amp; prose"})for(const auto *color:{"", "#378ADD", "not-a-color"})for(bool attribute:{false,true}){
  const auto leaf=std::string("<Bookmark modulename='MissingModule' key='John 3:16' moduledescription='Original module'")+(attribute?std::string(" description='")+description+"'":"")+" color='ignored'/>";
  const std::string source="<SwordBookmarks syntaxVersion='1.0'><Folder caption='Personal' color='"+std::string(color)+"'>"+leaf+"<Folder caption='Nested'>"+leaf+"</Folder>"+leaf+"</Folder>"+leaf+"</SwordBookmarks>";{std::ofstream out(file);out<<source;}
  const auto expected=load(file.u8string(),frozen::gui_parse_bookmarks);check(load(file.u8string(),actual::gui_parse_bookmarks)==expected,"Complete frozen/current recursive bookmark load matches");
  const auto owned=read_bookmarks(file.u8string());check(owned.items.size()==expected.size()&&owned.syntax_version=="1.0"&&owned.revision.size()==64&&!owned.ignored_subtrees,"Copied hierarchy, syntax version and revision");
  for(size_t i=0;i<expected.size();++i){const auto &item=owned.items[i];const auto &old=expected[i];check(item.parent==old.parent&&item.label==old.label&&item.key==old.key&&item.module==old.module&&item.module_description==old.module_description&&item.description==old.description&&item.color==old.color,"Owned bookmark fields/derived labels agree with complete retained tree load");}
  check(owned.items[0].kind==BookmarkKind::folder&&owned.items[1].kind==BookmarkKind::bookmark&&owned.items[1].position==0&&owned.items[2].position==1&&owned.items[5].position==1,"Depth-first type/parent/sibling order retained");
  check(bytes(file)==source,"Read-only repository preserves exact source bytes");++comparisons;
 }
 auto odd=parse_bookmarks("<SwordBookmarks><Unknown caption='unsupported'><Bookmark/></Unknown><Folder><Bookmark description=''/><Bookmark key='Raw'/><Bookmark description='' key='' modulename=''/></Folder></SwordBookmarks>");
 check(odd.ignored_subtrees==1&&odd.items.size()==4&&!odd.items[0].label&&!odd.items[1].label&&odd.items[2].label=="Raw"&&odd.items[3].label==", ","Unsupported subtrees and unsafe missing-identity captions stay explicit without fabricated navigation");
 check(parse_bookmarks("<SwordBookmarks/>").items.empty(),"Empty bookmark document has no manufactured defaults");
 const auto a=parse_bookmarks("<SwordBookmarks/>");check(a.revision==parse_bookmarks("<SwordBookmarks/>").revision&&a.revision!=parse_bookmarks("<SwordBookmarks />").revision,"Revision describes exact source bytes, including formatting");
 for(const auto &bad:std::vector<std::string>{"","<Xiphos/>","<SwordBookmarks>","<!DOCTYPE SwordBookmarks [<!ENTITY x 'hidden'>]><SwordBookmarks/>",std::string("<SwordBookmarks/>\0tail",22)})rejects([&]{parse_bookmarks(bad);});
 std::string deep="<SwordBookmarks>";for(size_t i=0;i<maximum_bookmark_depth;++i)deep+="<Folder>";std::string closing;for(size_t i=0;i<maximum_bookmark_depth;++i)closing+="</Folder>";check(parse_bookmarks(deep+closing+"</SwordBookmarks>").items.size()==maximum_bookmark_depth,"Exact depth bound");rejects([&]{parse_bookmarks(deep+"<Bookmark/>"+closing+"</SwordBookmarks>");});
 std::string many="<SwordBookmarks>";for(size_t i=0;i<maximum_bookmark_items;++i)many+="<Bookmark/>";check(parse_bookmarks(many+"</SwordBookmarks>").items.size()==maximum_bookmark_items,"Exact item bound");rejects([&]{parse_bookmarks(many+"<Bookmark/></SwordBookmarks>");});
 rejects([&]{parse_bookmarks("<SwordBookmarks><Bookmark description='"+std::string(maximum_bookmark_text_bytes+1,'x')+"'/></SwordBookmarks>");});
 // Owned label copies count toward aggregate, even if the XML itself is bounded.
 std::string expanded="<SwordBookmarks>";for(size_t i=0;i<300;++i)expanded+="<Bookmark description='"+std::string(maximum_bookmark_text_bytes,'x')+"'/>";expanded+="</SwordBookmarks>";rejects([&]{parse_bookmarks(expanded);});
 // Actual/frozen XML writer helper groups keep exact folder/color/leaf encoding.
 auto generated=[](bool current){xmlDocPtr doc=xmlNewDoc(BAD_CAST "1.0");auto *node=xmlNewNode(nullptr,BAD_CAST "SwordBookmarks");xmlDocSetRootElement(doc,node);auto *folder=current?actual::xml_add_folder_to_parent_colored(node,const_cast<gchar *>("Original & folder"),"#123456"):frozen::xml_add_folder_to_parent_colored(node,const_cast<gchar *>("Original & folder"),"#123456");if(current)actual::xml_add_bookmark_to_parent(folder,const_cast<gchar *>("Label"),const_cast<gchar *>("John3:16"),nullptr,nullptr);else frozen::xml_add_bookmark_to_parent(folder,const_cast<gchar *>("Label"),const_cast<gchar *>("John3:16"),nullptr,nullptr);xmlChar *data=nullptr;int size=0;xmlDocDumpMemory(doc,&data,&size);std::string result(reinterpret_cast<char *>(data),size);xmlFree(data);xmlFreeDoc(doc);return result;};
 check(generated(true)==generated(false),"Complete current/frozen bookmark XML helper encoding");const auto generated_snapshot=parse_bookmarks(generated(true));check(generated_snapshot.items[0].caption=="Original & folder"&&generated_snapshot.items[1].module_description==" ","Legacy null module-description writer default retained");
 std::cout<<"Bookmarks: "<<comparisons<<" complete recursive tree comparisons, XML helper encoding, raw hierarchy/labels/colors, read-only revision and bounds passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
