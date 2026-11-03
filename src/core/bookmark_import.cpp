// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/andbible.hpp"
#include "xml_document.hpp"
#include "bookmark_import_xml.hpp"
#include "preference_file.hpp"
#include <glib.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <algorithm>
#include <functional>
#include <stdexcept>
namespace xiphos::core {
namespace {
std::string prop(xmlNode *node,const char *name){std::unique_ptr<xmlChar,decltype(xmlFree)> value(xmlGetNoNsProp(node,BAD_CAST name),xmlFree);return value?reinterpret_cast<const char *>(value.get()):"";}
bool named(xmlNode *node,const char *name){return node->type==XML_ELEMENT_NODE&&!xmlStrcmp(node->name,BAD_CAST name);}
std::vector<xmlNode *> leaves(xmlNode *root){std::vector<xmlNode *> out;std::function<void(xmlNode *)> visit=[&](xmlNode *parent){for(auto *node=parent->children;node;node=node->next)if(named(node,"Bookmark"))out.push_back(node);else if(named(node,"Folder"))visit(node);};visit(root);return out;}
std::string digest(const std::string &value){std::unique_ptr<gchar,decltype(&g_free)> hash(g_compute_checksum_for_data(G_CHECKSUM_SHA256,reinterpret_cast<const guchar *>(value.data()),value.size()),g_free);if(!hash)throw std::bad_alloc();return hash.get();}
struct Documents {
 detail::XmlDocument target,source;
 std::string original;
 BookmarkImportReview review;
 std::map<std::string,std::vector<xmlNode *>> existing;
 std::vector<xmlNode *> incoming;
 std::map<xmlNode *,std::string> contexts;
 std::size_t work=0,output=0;
 void add(std::string &to,const std::string &value){if(value.size()>4*maximum_bookmark_snapshot_bytes-work)throw std::length_error("Bookmark comparison exceeds metadata budget");work+=value.size();to+=std::to_string(value.size())+":"+value;}
 std::string representation(xmlNode *node,bool container,bool wrapper,unsigned depth=0){
  if(depth>64)throw std::invalid_argument("Bookmark comparison metadata exceeds depth limit");
  std::string text;add(text,std::to_string(node->type));
  add(text,wrapper?"import-root":node->name?reinterpret_cast<const char *>(node->name):"");
  if(node->ns&&node->ns->href)add(text,reinterpret_cast<const char *>(node->ns->href));
  if(node->type!=XML_ELEMENT_NODE){if(node->content)add(text,reinterpret_cast<const char *>(node->content));return text;}
  std::vector<std::pair<std::string,std::string>> attrs;
  for(auto *attribute=node->properties;attribute;attribute=attribute->next){std::string key=reinterpret_cast<const char *>(attribute->name);if(wrapper&&!attribute->ns&&(key=="andbible-source"||key=="caption"))continue;
   if(wrapper&&attribute->ns&&attribute->ns->href&&!xmlStrcmp(attribute->ns->href,BAD_CAST detail::bookmark_import_xml_namespace)&&(key=="root"||key=="caption"))continue;
   if(attribute->ns&&attribute->ns->href)key=std::string(reinterpret_cast<const char *>(attribute->ns->href))+":"+key;
   std::unique_ptr<xmlChar,decltype(xmlFree)> value(xmlNodeListGetString(node->doc,attribute->children,1),xmlFree);attrs.emplace_back(key,value?reinterpret_cast<const char *>(value.get()):"");}
  if(wrapper){const auto caption=named(node,"SwordBookmarks")?xmlHasNsProp(node,BAD_CAST "caption",nullptr)?std::make_optional(prop(node,"caption")):std::nullopt:detail::import_root_caption(node);if(caption)attrs.emplace_back("caption",*caption);}
  std::sort(attrs.begin(),attrs.end());for(const auto &attribute:attrs){add(text,attribute.first);add(text,attribute.second);}
  for(auto *child=node->children;child;child=child->next){if(container&&(named(child,"Folder")||named(child,"Bookmark")||xmlIsBlankNode(child)))continue;add(text,representation(child,false,false,depth+1));}
  return text;
 }
 std::string context(xmlNode *node){auto found=contexts.find(node);if(found!=contexts.end())return found->second;
  const bool wrapper=named(node,"SwordBookmarks")||detail::imported_bookmark_root(node)||xmlHasNsProp(node,BAD_CAST "andbible-source",nullptr);
  auto value=digest(representation(node,true,wrapper));
  if(!wrapper&&node->parent&&node->parent->type==XML_ELEMENT_NODE){std::size_t position=0;for(auto *previous=node->prev;previous;previous=previous->prev)if(named(previous,"Folder")||named(previous,"Bookmark"))++position;value=digest(context(node->parent)+":"+std::to_string(position)+":"+value);}
  contexts.emplace(node,value);return value;
 }
 std::string fingerprint(xmlNode *node){return digest(context(node->parent)+":"+digest(representation(node,false,false)));}
 Documents(std::string bytes,const std::optional<std::string> &revision,const std::string &xml):target(nullptr,xmlFreeDoc),source(nullptr,xmlFreeDoc),original(std::move(bytes)){
  const auto current=parse_bookmarks(original),imported=parse_bookmarks(xml);if(revision&&*revision!=current.revision)throw std::invalid_argument("Bookmark destination changed; reload before reviewing");
  review.destination_revision=revision;review.source_revision=imported.revision;
  target=detail::parse_xml_document(original,"SwordBookmarks");source=detail::parse_xml_document(xml,"SwordBookmarks");
  for(auto *node:leaves(xmlDocGetRootElement(target.get()))){auto id=prop(node,"andbible-entry");if(!id.empty())existing[id].push_back(node);}
  incoming=leaves(xmlDocGetRootElement(source.get()));std::set<std::string> seen;
  for(auto *node:incoming){const auto id=prop(node,"andbible-entry");if(id.empty()){++review.new_items;++review.unidentified;continue;}
   if(!seen.insert(id).second)throw std::invalid_argument("Incoming bookmark identities repeat; explicit source correction is required");
   const auto found=existing.find(id);if(found==existing.end()){++review.new_items;continue;}
   const auto kind=found->second.size()!=1?BookmarkImportCollisionKind::ambiguous:fingerprint(node)==fingerprint(found->second[0])?BookmarkImportCollisionKind::identical:BookmarkImportCollisionKind::conflicting;
   if(kind==BookmarkImportCollisionKind::identical)++review.identical;else if(kind==BookmarkImportCollisionKind::conflicting)++review.conflicting;else ++review.ambiguous;
   BookmarkImportCollision collision{id,prop(node,"key"),prop(found->second[0],"description"),prop(node,"description"),kind};
   for(const auto *value:{&collision.identity,&collision.key,&collision.existing_description,&collision.incoming_description}){if(value->size()>maximum_bookmark_snapshot_bytes-output)throw std::length_error("Bookmark review exceeds byte limit");output+=value->size();}
   review.collisions.push_back(std::move(collision));
  }
 }
 std::string apply(const std::string &folder,BookmarkImportPolicy policy){
  if(policy!=BookmarkImportPolicy::reject_collisions&&policy!=BookmarkImportPolicy::skip_identical&&policy!=BookmarkImportPolicy::keep_existing)throw std::invalid_argument("Unknown bookmark merge policy");
  if(review.ambiguous||(policy==BookmarkImportPolicy::reject_collisions&&!review.collisions.empty())||(policy==BookmarkImportPolicy::skip_identical&&review.conflicting))throw std::invalid_argument("Bookmark collisions require a different explicit policy or source correction");
  if(!review.new_items&&!incoming.empty())return original; // Byte-identical repeat, without another import folder.
  for(auto *node:incoming)if(existing.count(prop(node,"andbible-entry"))){xmlUnlinkNode(node);xmlFreeNode(node);}
  auto copy=detail::copy_bookmark_import_root(source.get(),target.get(),folder);if(!xmlAddChild(xmlDocGetRootElement(target.get()),copy.get()))throw std::bad_alloc();copy.release();
  xmlChar *raw=nullptr;int size=0;xmlDocDumpFormatMemoryEnc(target.get(),&raw,&size,"UTF-8",0);std::unique_ptr<xmlChar,decltype(xmlFree)> encoded(raw,xmlFree);if(!raw||size<=0)throw std::runtime_error("Bookmark merge serialization failed");return {reinterpret_cast<const char *>(raw),static_cast<std::size_t>(size)};
 }
};
std::string baseline(const std::string &path,const std::optional<std::string> &revision){
 detail::validate_xml_text(path,4096);if(path.empty())throw std::invalid_argument("Bookmark destination path required");
 const auto file=std::filesystem::u8path(path);const bool exists=std::filesystem::exists(file);if(exists!=revision.has_value())throw std::invalid_argument("Bookmark destination existence changed");
 return exists?detail::read_preference_file(file,detail::maximum_xml_document_bytes):"<SwordBookmarks syntaxVersion='1.0'/>";
}
}
BookmarkImportReview review_bookmark_import(const std::string &path,const std::optional<std::string> &revision,const std::string &xml){return Documents(baseline(path,revision),revision,xml).review;}
BookmarkSnapshot merge_bookmark_document(const std::string &path,const std::optional<std::string> &revision,const std::string &xml,const std::string &folder,BookmarkImportPolicy policy){
 detail::validate_xml_text(folder,4096);if(folder.empty())throw std::invalid_argument("Import folder required");(void)parse_bookmarks(xml);
 BookmarkSnapshot prepared;detail::replace_preference_file(path,[&](const std::filesystem::path &file,const std::string &old,bool exists){
  if(exists!=revision.has_value())throw std::invalid_argument("Bookmark destination existence changed");Documents documents(exists?old:"<SwordBookmarks syntaxVersion='1.0'/>",revision,xml);
  const auto bytes=documents.apply(folder,policy);prepared=parse_bookmarks(bytes);{std::ofstream out(file,std::ios::binary);out.exceptions(std::ios::badbit|std::ios::failbit);out.write(bytes.data(),bytes.size());out.close();}
  if(detail::read_preference_file(file,detail::maximum_xml_document_bytes)!=bytes)throw std::runtime_error("Bookmark merge verification failed");
 },detail::maximum_xml_document_bytes);return prepared;
}
}
