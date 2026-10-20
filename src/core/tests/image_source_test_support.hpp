// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <libxml/HTMLparser.h>
#include <memory>
#include <vector>
#include <string>
#include <stdexcept>
namespace image_test {
inline std::string bytes(){return std::string("Original\0image",14)+char(0xff);}
inline std::vector<std::string> attributes(const std::string &html,const char *tag,const char *name){
 std::unique_ptr<xmlDoc,decltype(&xmlFreeDoc)> doc(htmlReadMemory(html.data(),static_cast<int>(html.size()),nullptr,"UTF-8",HTML_PARSE_NONET|HTML_PARSE_NODEFDTD|HTML_PARSE_NOERROR|HTML_PARSE_NOWARNING),xmlFreeDoc);
 if(!doc)throw std::runtime_error("Parse original-prose SDK image source");
 std::vector<std::string> result;std::vector<xmlNode *> pending{xmlDocGetRootElement(doc.get())};
 while(!pending.empty()){
  auto *n=pending.back();pending.pop_back();if(!n)continue;
  if(n->next)pending.push_back(n->next);if(n->children)pending.push_back(n->children);
  if(n->type==XML_ELEMENT_NODE&&xmlStrcasecmp(n->name,BAD_CAST tag)==0){
   std::unique_ptr<xmlChar,decltype(xmlFree)> src(xmlGetProp(n,BAD_CAST name),xmlFree);
   if(!src)throw std::runtime_error("SDK image keeps a source");result.emplace_back(reinterpret_cast<const char *>(src.get()));
  }
 }
 return result;
}
inline std::vector<std::string> images(const std::string &html){return attributes(html,"img","src");}
}
