// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/bookmarks.hpp"
#include "xml_document.hpp"
#include "preference_file.hpp"
#include <glib.h>
#include <filesystem>
#include <fstream>
#include <array>
#include <algorithm>
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
namespace {
void bookmark_revision(const std::string &revision) {
  if(revision.size()!=64||!std::all_of(revision.begin(),revision.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');}))
    throw std::invalid_argument("Invalid bookmark revision");
}
bool empty_fields(const BookmarkFields &f) {
  return !f.caption&&!f.color&&!f.module&&!f.key&&!f.module_description&&!f.description;
}
void bookmark_fields(const BookmarkEdit &edit) {
  const auto &f=edit.fields;
  for(const auto *text:{&f.caption,&f.color,&f.module,&f.key,&f.module_description,&f.description})
    if(*text)detail::validate_xml_text(**text,maximum_bookmark_text_bytes);
  if(edit.kind==BookmarkKind::folder) {
    if(!f.caption||f.caption->empty()||f.module||f.key||f.module_description||f.description)
      throw std::invalid_argument("Folder edits require a caption and optional color only");
  } else {
    if(f.caption||f.color||!f.key||f.key->empty())throw std::invalid_argument("Bookmark edits require a nonempty key and leaf fields only");
    // Do not create input that would overflow or pass NULL through the retained
    // empty-description sprintf path. Native reads remain safely bounded.
    if(f.description&&f.description->empty()&&(!f.module||f.key->size()+f.module->size()>497))
      throw std::invalid_argument("Empty bookmark description cannot safely round trip through legacy labels");
  }
}
void bookmark_attribute(xmlNode *node,const char *key,const std::optional<std::string> &text) {
  if(text){if(!xmlSetProp(node,BAD_CAST key,BAD_CAST text->c_str()))throw std::bad_alloc();}
  else xmlUnsetProp(node,BAD_CAST key);
}
void apply_bookmark_fields(xmlNode *node,const BookmarkEdit &edit) {
  const auto &f=edit.fields;
  if(edit.kind==BookmarkKind::folder){bookmark_attribute(node,"caption",f.caption);bookmark_attribute(node,"color",f.color);}
  else {bookmark_attribute(node,"modulename",f.module);bookmark_attribute(node,"key",f.key);
    bookmark_attribute(node,"moduledescription",f.module_description);bookmark_attribute(node,"description",f.description);}
}
std::vector<xmlNode *> bookmark_nodes(xmlNode *root) {
  std::vector<xmlNode *> result;
  std::function<void(xmlNode *)> visit=[&](xmlNode *parent){for(auto *node=parent->children;node;node=node->next){
    if(node->type!=XML_ELEMENT_NODE)continue;
    if(!xmlStrcmp(node->name,BAD_CAST "Folder")){result.push_back(node);visit(node);}
    else if(!xmlStrcmp(node->name,BAD_CAST "Bookmark"))result.push_back(node);
  }};visit(root);return result;
}
}
void validate_bookmark_edit(const BookmarkEdit &edit) {
  if(edit.kind!=BookmarkKind::folder&&edit.kind!=BookmarkKind::bookmark)throw std::invalid_argument("Invalid bookmark kind");
  switch(edit.action) {
    case BookmarkAction::insert:
      if(edit.target)throw std::invalid_argument("Insert does not select a target");bookmark_fields(edit);break;
    case BookmarkAction::update:
      if(!edit.target||edit.parent||edit.position)throw std::invalid_argument("Update requires only a target and fields");bookmark_fields(edit);break;
    case BookmarkAction::remove:
      if(!edit.target||edit.parent||edit.position||!empty_fields(edit.fields))throw std::invalid_argument("Remove requires only a target");break;
    case BookmarkAction::move:
      if(!edit.target||!empty_fields(edit.fields))throw std::invalid_argument("Move requires a target and destination only");break;
    default:throw std::invalid_argument("Unknown bookmark edit");
  }
}
std::string serialize_bookmark_edit(const std::string &source,const std::string &revision,const BookmarkEdit &edit) {
  validate_bookmark_edit(edit);bookmark_revision(revision);
  const auto before=parse_bookmarks(source);
  if(before.revision!=revision)throw std::invalid_argument("Bookmark source changed; reread before editing");
  auto document=detail::parse_xml_document(source,"SwordBookmarks");auto *root=xmlDocGetRootElement(document.get());
  const auto nodes=bookmark_nodes(root);
  if(nodes.size()!=before.items.size())throw std::runtime_error("Bookmark address mapping differs from owned snapshot");
  if(edit.target&&*edit.target>=nodes.size())throw std::invalid_argument("Bookmark target is unavailable");
  if(edit.parent&&(*edit.parent>=nodes.size()||before.items[*edit.parent].kind!=BookmarkKind::folder))
    throw std::invalid_argument("Bookmark destination is not a folder");
  auto *target=edit.target?nodes[*edit.target]:nullptr;
  auto *parent=edit.parent?nodes[*edit.parent]:root;
  if(edit.action==BookmarkAction::update) {
    if(before.items[*edit.target].kind!=edit.kind)throw std::invalid_argument("Cannot change bookmark node kind");
    apply_bookmark_fields(target,edit);
  } else if(edit.action==BookmarkAction::remove) {xmlUnlinkNode(target);xmlFreeNode(target);}
  else {
    if(edit.action==BookmarkAction::move)for(auto *ancestor=parent;ancestor;ancestor=ancestor->parent)
      if(ancestor==target)throw std::invalid_argument("Cannot move a folder into its own subtree");
    std::vector<xmlNode *> siblings;
    for(auto *node=parent->children;node;node=node->next)
      if(node!=target&&node->type==XML_ELEMENT_NODE&&(!xmlStrcmp(node->name,BAD_CAST "Folder")||!xmlStrcmp(node->name,BAD_CAST "Bookmark")))siblings.push_back(node);
    if(edit.position>siblings.size())throw std::invalid_argument("Bookmark destination position is unavailable");
    if(edit.action==BookmarkAction::insert)target=xmlNewNode(nullptr,BAD_CAST(edit.kind==BookmarkKind::folder?"Folder":"Bookmark"));
    else xmlUnlinkNode(target);
    std::unique_ptr<xmlNode,decltype(&xmlFreeNode)> detached(target,xmlFreeNode);
    if(!target)throw std::bad_alloc();
    if(edit.action==BookmarkAction::insert)apply_bookmark_fields(target,edit);
    if(!(edit.position<siblings.size()?xmlAddPrevSibling(siblings[edit.position],target):xmlAddChild(parent,target)))
      throw std::bad_alloc();
    (void)detached.release();
  }
  xmlChar *buffer=nullptr;int length=0;xmlDocDumpFormatMemoryEnc(document.get(),&buffer,&length,"UTF-8",0);
  const std::unique_ptr<xmlChar,decltype(xmlFree)> owned(buffer,xmlFree);
  if(!buffer||length<=0)throw std::runtime_error("Bookmark serialization failed");
  std::string result(reinterpret_cast<const char *>(buffer),static_cast<std::size_t>(length));
  (void)parse_bookmarks(result);return result;
}
void validate_bookmark_edit_request(const std::optional<std::string> &revision,const BookmarkEdit &edit) {
  validate_bookmark_edit(edit);if(revision)bookmark_revision(*revision);
  else if(edit.action!=BookmarkAction::insert)throw std::invalid_argument("Missing bookmark files support insert only");
}
BookmarkSnapshot edit_bookmarks(const std::string &path,const std::optional<std::string> &revision,const BookmarkEdit &edit) {
  validate_bookmark_edit_request(revision,edit);
  BookmarkSnapshot prepared;
  detail::replace_preference_file(path,[&](const std::filesystem::path &file,const std::string &original,bool existed){
    if(existed!=revision.has_value())throw std::invalid_argument("Bookmark file existence changed; reread before editing");
    const std::string source=existed?original:"<SwordBookmarks syntaxVersion='1.0'/>";
    const auto serialized=serialize_bookmark_edit(source,revision?*revision:parse_bookmarks(source).revision,edit);
    {std::ofstream output(file,std::ios::binary);output.exceptions(std::ios::badbit|std::ios::failbit);
     output.write(serialized.data(),static_cast<std::streamsize>(serialized.size()));output.close();}
    if(detail::read_preference_file(file,detail::maximum_xml_document_bytes)!=serialized)throw std::runtime_error("Staged bookmark bytes differ");
    prepared=read_bookmarks(file.u8string());
  },detail::maximum_xml_document_bytes);
  return prepared;
}

}
