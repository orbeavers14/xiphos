// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <libxml/HTMLparser.h>
#include <memory>
#include <vector>
#include <string>
#include <stdexcept>
namespace image_test {
inline std::string preview_png(){return std::string("\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52\x00\x00\x00\x08\x00\x00\x00\x08\x08\x06\x00\x00\x00\xc4\x0f\xbe\x8b\x00\x00\x00\x12\x49\x44\x41\x54\x78\x9c\x63\x50\xc9\x58\xf3\x1f\x1f\x66\x18\x19\x0a\x00\xc1\xa0\x8d\xc1\xf4\xd5\x38\xd1\x00\x00\x00\x00\x49\x45\x4e\x44\xae\x42\x60\x82",75);}
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
