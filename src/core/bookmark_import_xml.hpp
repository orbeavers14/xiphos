// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xml_document.hpp"
#include <stdexcept>
#include <optional>
namespace xiphos::core::detail {
inline constexpr char bookmark_import_xml_namespace[]="urn:xiphos:bookmarks:import:1";
using ImportXmlNode=std::unique_ptr<xmlNode,decltype(&xmlFreeNode)>;
inline std::optional<std::string> import_root_caption(xmlNode *node){
 std::unique_ptr<xmlChar,decltype(xmlFree)> saved(xmlGetNsProp(node,BAD_CAST "caption",BAD_CAST bookmark_import_xml_namespace),xmlFree);
 if(saved)return std::string(reinterpret_cast<const char *>(saved.get()));return std::nullopt;
}
inline bool imported_bookmark_root(xmlNode *node){
 std::unique_ptr<xmlChar,decltype(xmlFree)> value(xmlGetNsProp(node,BAD_CAST "root",BAD_CAST bookmark_import_xml_namespace),xmlFree);
 return value&&!xmlStrcmp(value.get(),BAD_CAST "SwordBookmarks");
}
// Keep the source root caption separate from the user's destination-folder name.
// Reserved metadata collisions reject instead of overwriting either value.
inline ImportXmlNode copy_bookmark_import_root(xmlDoc *source,xmlDoc *target,const std::string &folder){
 auto *root=xmlDocGetRootElement(source);ImportXmlNode copy(xmlDocCopyNode(root,target,1),xmlFreeNode);if(!copy)throw std::bad_alloc();
 auto *ns=xmlSearchNsByHref(target,copy.get(),BAD_CAST bookmark_import_xml_namespace);
 if(!ns||!ns->prefix){std::string prefix="xi";for(unsigned number=0;xmlSearchNs(target,copy.get(),BAD_CAST prefix.c_str());++number){if(number>=1000)throw std::invalid_argument("Import namespace prefix budget exceeded");prefix="xi"+std::to_string(number+1);}ns=xmlNewNs(copy.get(),BAD_CAST bookmark_import_xml_namespace,BAD_CAST prefix.c_str());if(!ns)throw std::bad_alloc();}
 std::unique_ptr<xmlChar,decltype(xmlFree)> oldRoot(xmlGetNsProp(copy.get(),BAD_CAST "root",BAD_CAST bookmark_import_xml_namespace),xmlFree);
 if(oldRoot&&xmlStrcmp(oldRoot.get(),BAD_CAST "SwordBookmarks"))throw std::invalid_argument("Import root metadata conflicts");
 std::unique_ptr<xmlChar,decltype(xmlFree)> caption(xmlGetNoNsProp(root,BAD_CAST "caption"),xmlFree);
 const auto saved=import_root_caption(copy.get());if(saved&&(!caption||*saved!=reinterpret_cast<const char *>(caption.get())))throw std::invalid_argument("Import caption metadata conflicts");
 if(!xmlSetNsProp(copy.get(),ns,BAD_CAST "root",BAD_CAST "SwordBookmarks"))throw std::bad_alloc();
 if(caption&&!xmlSetNsProp(copy.get(),ns,BAD_CAST "caption",caption.get()))throw std::bad_alloc();
 xmlNodeSetName(copy.get(),BAD_CAST "Folder");if(!xmlSetProp(copy.get(),BAD_CAST "caption",BAD_CAST folder.c_str()))throw std::bad_alloc();return copy;
}
}
