// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/bookmarks.hpp"
#include "xml_document.hpp"
#include "preference_file.hpp"
#include <glib.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
namespace xiphos::core {
BookmarkSnapshot parse_bookmarks(const std::string &source) {
  auto document=detail::parse_xml_document(source,"SwordBookmarks");BookmarkSnapshot result;
  auto account=[&](const std::string &text){
    if(text.size()>maximum_bookmark_text_bytes||text.size()>maximum_bookmark_snapshot_bytes-result.snapshot_bytes)
      throw std::invalid_argument("Bookmark text exceeds bounds");
    result.snapshot_bytes+=text.size();
  };
  auto attribute=[&](xmlNode *node,const char *name)->std::optional<std::string>{
    const std::unique_ptr<xmlChar,decltype(xmlFree)> value(xmlGetProp(node,BAD_CAST name),xmlFree);
    if(!value)return std::nullopt;std::string text(reinterpret_cast<const char *>(value.get()));account(text);return text;
  };
  const std::unique_ptr<gchar,decltype(&g_free)> revision(g_compute_checksum_for_data(G_CHECKSUM_SHA256,
      reinterpret_cast<const guchar *>(source.data()),source.size()),g_free);
  if(!revision)throw std::bad_alloc();result.revision=revision.get();account(result.revision);
  auto *root=xmlDocGetRootElement(document.get());result.syntax_version=attribute(root,"syntaxVersion");
  std::function<void(xmlNode *,std::optional<std::size_t>,std::size_t)> visit;
  visit=[&](xmlNode *container,std::optional<std::size_t> parent,std::size_t depth){
    std::size_t position=0;
    for(auto *node=container->children;node;node=node->next){
      if(node->type!=XML_ELEMENT_NODE)continue;
      const bool folder=!xmlStrcmp(node->name,BAD_CAST "Folder"),bookmark=!xmlStrcmp(node->name,BAD_CAST "Bookmark");
      if(!folder&&!bookmark){++result.ignored_subtrees;continue;}
      if(depth>maximum_bookmark_depth||result.items.size()==maximum_bookmark_items)
        throw std::invalid_argument("Bookmark hierarchy exceeds bounds");
      BookmarkItem item;item.kind=folder?BookmarkKind::folder:BookmarkKind::bookmark;item.parent=parent;item.position=position++;
      if(folder){item.caption=attribute(node,"caption");item.color=attribute(node,"color");item.label=item.caption;}
      else {
        item.module=attribute(node,"modulename");item.key=attribute(node,"key");
        item.module_description=attribute(node,"moduledescription");item.description=attribute(node,"description");
        if(!item.description)item.label=item.key;
        else if(!item.description->empty())item.label=item.description;
        else if(item.key&&item.module)item.label=*item.key+", "+*item.module;
      }
      if(item.label)account(*item.label);
      const auto index=result.items.size();result.items.push_back(std::move(item));
      if(folder)visit(node,index,depth+1);
      else for(auto *child=node->children;child;child=child->next)if(child->type==XML_ELEMENT_NODE)++result.ignored_subtrees;
    }
  };
  visit(root,std::nullopt,1);return result;
}
BookmarkSnapshot read_bookmarks(const std::string &path) {
  if(path.empty()||path.find('\0')!=std::string::npos)throw std::invalid_argument("Bookmark path is invalid");
  const auto file=std::filesystem::u8path(path);
  if(!std::filesystem::is_regular_file(file))throw std::invalid_argument("Bookmarks require a regular selected file");
  return parse_bookmarks(detail::read_preference_file(file,detail::maximum_xml_document_bytes));
}
}
