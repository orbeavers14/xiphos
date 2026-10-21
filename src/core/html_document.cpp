// SPDX-License-Identifier: GPL-2.0-or-later
#include "html_document.hpp"
#include "xiphos/core/lexical_style.hpp"
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <glib.h>
#include <array>
#include <algorithm>
#include <memory>
#include <set>
#include <stdexcept>
namespace xiphos::core::detail {
namespace {
std::string lower(std::string value){for(auto &c:value)if(c>='A'&&c<='Z')c=static_cast<char>(c+'a'-'A');return value;}
std::string trim(std::string value){const auto first=value.find_first_not_of(" \t\r\n");if(first==std::string::npos)return {};const auto last=value.find_last_not_of(" \t\r\n");return value.substr(first,last-first+1);}
bool passive_link(const std::string &value){
  for(unsigned char c:value)if(c<32||c==127)return false;
  const auto uri=lower(trim(value));
  return uri.empty()||uri.front()=='#'||uri.rfind("https://",0)==0||uri.rfind("http://",0)==0||uri.rfind("mailto:",0)==0||
    uri.rfind("sword://",0)==0||uri.rfind("passagestudy.jsp?",0)==0;
}
std::string scope_css(const std::string &css,unsigned style){
  const auto root=".xiphos-export-style-"+std::to_string(style);std::string result;std::size_t position=0;
  while(position<css.size()){
    const auto open=css.find('{',position);if(open==std::string::npos){if(trim(css.substr(position)).empty())break;throw std::invalid_argument("Unknown core stylesheet grammar");}
    const auto close=css.find('}',open+1);if(close==std::string::npos)throw std::invalid_argument("Unknown core stylesheet grammar");
    const auto selector=trim(css.substr(position,open-position)),body=css.substr(open+1,close-open-1);
    if(selector.empty()||selector.find_first_of(",@{}")!=std::string::npos||body.find('{')!=std::string::npos)throw std::invalid_argument("Unsupported core stylesheet rule");
    result+=(selector=="*"?root+", "+root+" *":root+" "+selector)+" {"+body+"}\n";position=close+1;
  }
  return result;
}
}
PreparedExportHtml prepare_export_html(const EntryHtmlSource &source,std::size_t max_bytes,const ExportImageResolver &images){
  if(!max_bytes||max_bytes>maximum_html_source_bytes)throw std::invalid_argument("Invalid HTML preparation budget");
  PreparedExportHtml out;
  if(source.html.size()>max_bytes){out.limited=true;return out;}
  if(source.html.find('\0')!=std::string::npos||!g_utf8_validate(source.html.data(),static_cast<gssize>(source.html.size()),nullptr)){
    out.issue=HtmlExportIssue::markup;return out;
  }
  static const std::array<std::string,4> styles{{{},lexical_block_style(false,false,false,true),lexical_block_style(true,false,false,false),lexical_block_style(true,false,true,false)}};
  const auto found=std::find(styles.begin(),styles.end(),source.css);
  if(found==styles.end()){out.issue=HtmlExportIssue::styles;return out;}
  out.style=static_cast<unsigned>(found-styles.begin());
  try{out.css=scope_css(source.css,out.style);}catch(const std::invalid_argument &){out.issue=HtmlExportIssue::styles;return out;}
  if(source.html.empty())return out;
  std::unique_ptr<htmlParserCtxt,decltype(&htmlFreeParserCtxt)> context(htmlNewParserCtxt(),htmlFreeParserCtxt);
  if(!context)throw std::bad_alloc();
  std::unique_ptr<xmlDoc,decltype(&xmlFreeDoc)> document(htmlCtxtReadMemory(context.get(),source.html.data(),static_cast<int>(source.html.size()),
    nullptr,"UTF-8",HTML_PARSE_NONET|HTML_PARSE_NODEFDTD|HTML_PARSE_NOERROR|HTML_PARSE_NOWARNING),xmlFreeDoc);
  if(!document||document->intSubset||document->extSubset){out.issue=HtmlExportIssue::markup;return out;}
  static const std::set<std::string> tags{"html","head","body","div","span","a","sup","sub","b","i","em","strong","font","p","br","hr","table","tbody","thead","tfoot","tr","th","td","ul","ol","li","blockquote","center","h1","h2","h3","h4","h5","h6","small","big","s","strike","u","tt","pre","code","dl","dt","dd","q","cite","abbr","bdo"};
  static const std::set<std::string> attributes{"id","class","title","dir","lang","xml:lang","align","valign","width","height","border","cellpadding","cellspacing","colspan","rowspan","color","bgcolor","face","size","name","href"};
  std::vector<std::pair<xmlNode *,unsigned>> pending{{xmlDocGetRootElement(document.get()),0}};xmlNode *body=nullptr;std::size_t nodes=0;
  std::vector<std::pair<xmlNode *,std::string>> resources;
  while(!pending.empty()){
    const auto item=pending.back();pending.pop_back();auto *node=item.first;if(!node)continue;
    if(++nodes>100000||item.second>128){out.issue=HtmlExportIssue::markup;return out;}
    if(node->next)pending.emplace_back(node->next,item.second);
    if(node->children)pending.emplace_back(node->children,item.second+1);
    if(node->type==XML_COMMENT_NODE||node->type==XML_TEXT_NODE)continue;
    if(node->type!=XML_ELEMENT_NODE){out.issue=HtmlExportIssue::markup;return out;}
    const std::string tag=reinterpret_cast<const char *>(node->name);
    if((tag=="img"&&!images)||tag=="link"||tag=="source"||tag=="video"||tag=="audio"){out.issue=HtmlExportIssue::resources;return out;}
    if(tag!="img"&&!tags.count(tag)){out.issue=HtmlExportIssue::markup;return out;}
    // Only body children enter the document. Reject wrapper context that would
    // otherwise disappear (for example inherited right-to-left direction).
    if((tag=="html"||tag=="head"||tag=="body")&&node->properties){out.issue=HtmlExportIssue::markup;return out;}
    if(tag=="body")body=node;
    for(auto *attribute=node->properties;attribute;attribute=attribute->next){
      const std::string name=reinterpret_cast<const char *>(attribute->name);
      if(name=="style"){out.issue=HtmlExportIssue::styles;return out;}
      if((name=="src"&&tag!="img")||name=="srcset"||name=="background"){out.issue=HtmlExportIssue::resources;return out;}
      if(!(tag=="img"&&(name=="src"||name=="alt"))&&!attributes.count(name)){out.issue=HtmlExportIssue::markup;return out;}
      std::unique_ptr<xmlChar,decltype(xmlFree)> value(xmlNodeListGetString(document.get(),attribute->children,1),xmlFree);
      const std::string text=value?reinterpret_cast<const char *>(value.get()):"";
      if((name=="id"||name=="class")&&text.find("xiphos-export-")!=std::string::npos){out.issue=HtmlExportIssue::styles;return out;}
      if(name=="href"&&(tag!="a"||!passive_link(text))){out.issue=HtmlExportIssue::links;return out;}
    }
    if(tag=="img"){
      std::unique_ptr<xmlChar,decltype(xmlFree)> src(xmlGetProp(node,BAD_CAST "src"),xmlFree);
      if(!src||!*src){out.issue=HtmlExportIssue::resources;return out;}
      if(resources.size()==maximum_search_export_images){out.limited=true;return out;}
      resources.emplace_back(node,reinterpret_cast<const char *>(src.get()));
    }
  }
  if(!body){out.issue=HtmlExportIssue::markup;return out;}
  // The complete structure passed before a resolver can touch the module library.
  // Keep encoded assets bounded even before the final serialized fragment check.
  std::size_t resource_bytes=0;
  for(const auto &resource:resources){
    const auto image=images(resource.second);
    if(image.status==ExportPngStatus::cancelled){out.cancelled=true;return out;}
    if(image.status==ExportPngStatus::output_limit){out.limited=true;return out;}
    if(image.status!=ExportPngStatus::ok||image.data_url.rfind("data:image/png;base64,",0)!=0){out.issue=HtmlExportIssue::resources;return out;}
    if(image.data_url.size()>max_bytes-resource_bytes){out.limited=true;return out;}
    resource_bytes+=image.data_url.size();
    if(!xmlSetProp(resource.first,BAD_CAST "src",BAD_CAST image.data_url.c_str()))throw std::bad_alloc();
  }
  std::unique_ptr<xmlBuffer,decltype(&xmlBufferFree)> output(xmlBufferCreate(),xmlBufferFree);
  if(!output)throw std::bad_alloc();
  for(auto *node=body->children;node;node=node->next){
    if(node->type==XML_COMMENT_NODE)continue;
    if(htmlNodeDump(output.get(),document.get(),node)<0)throw std::runtime_error("Unable to serialize passive HTML source");
    if(static_cast<std::size_t>(xmlBufferLength(output.get()))>max_bytes){out.limited=true;return out;}
  }
  if(xmlBufferLength(output.get()))out.html.assign(reinterpret_cast<const char *>(xmlBufferContent(output.get())),static_cast<std::size_t>(xmlBufferLength(output.get())));
  return out;
}
}
