// SPDX-License-Identifier: GPL-2.0-or-later
#include "html_document.hpp"
#include "xiphos/core/lexical_style.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace xiphos::core;
using namespace xiphos::core::detail;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 for(const auto &css:{std::string{},lexical_block_style(false,false,false,true),lexical_block_style(true,false,false,false),lexical_block_style(true,false,true,false)}){
  EntryHtmlSource source;source.html="<span class=\"word\">Original café <b>word</b></span>";source.css=css;
  auto output=prepare_export_html(source,8192);
  check(output.issue==HtmlExportIssue::none&&!output.limited&&output.html.find("Original café")!=std::string::npos,"Supported original markup remains a complete Unicode fragment");
  if(!css.empty()){
   std::istringstream rules(output.css);std::string line;
   while(std::getline(rules,line))check(line.rfind(".xiphos-export-style-",0)==0,"Every generated CSS rule is restricted to its matching source wrapper");
  }
 }
 for(const auto &html:{"<img src=\"file:/original.png\">","<link href=\"https://example.invalid/style.css\">","<video src=\"original.mp4\"></video>"}){
  EntryHtmlSource source;source.html=html;auto output=prepare_export_html(source,8192);check(output.issue!=HtmlExportIssue::none&&output.html.empty(),"Resource-dependent source is rejected rather than silently dropped or loaded");
 }
 for(const auto &html:{"<script>throw 1</script>","<span onclick=\"original()\">Original</span>","<a href=\"javascript:original()\">Original</a>","<span style=\"color:red\">Original</span>","<span class=\"xiphos-export-style-2\">Original</span>","<a href=\"j&#97;vascript:original()\">Original</a>"}){
  EntryHtmlSource source;source.html=html;auto output=prepare_export_html(source,8192);check(output.issue!=HtmlExportIssue::none&&output.html.empty(),"Unsupported active/link/style/reserved markup cannot enter an exported document");
 }
 for(const auto &href:{"#original","https://example.invalid/original","HTTPS://example.invalid/Original","sword://Text/John1:1","passagestudy.jsp?action=showNote&amp;type=n&amp;value=1"}){
  EntryHtmlSource source;source.html=std::string("<a href=\"")+href+"\">Original</a>";
  check(prepare_export_html(source,8192).issue==HtmlExportIssue::none,"Passive original semantic links remain in source");
 }
 EntryHtmlSource sdk_link;sdk_link.html="<a href=\"passagestudy.jsp?action=showStrongs&type=Greek&value=3056\">Original</a>";
 for(const auto &html:{"<body dir=\"rtl\">Original</body>","<html lang=\"he\"><body>Original</body></html>","<body class=\"word\">Original</body>"}){
  EntryHtmlSource wrapped;wrapped.html=html;const auto result=prepare_export_html(wrapped,8192);
  check(result.issue==HtmlExportIssue::markup&&result.html.empty(),"Document-wrapper context cannot silently disappear during fragment extraction");
 }
 EntryHtmlSource directed;directed.html="<div dir=\"rtl\" lang=\"he\">Original</div>";
 const auto directed_output=prepare_export_html(directed,8192);
 check(directed_output.issue==HtmlExportIssue::none&&directed_output.html.find("dir=\"rtl\"")!=std::string::npos&&directed_output.html.find("lang=\"he\"")!=std::string::npos,"Fragment-local language and direction remain attached to their content");
 const auto canonical_link=prepare_export_html(sdk_link,8192);
 check(canonical_link.issue==HtmlExportIssue::none&&canonical_link.html.find("&amp;type=Greek&amp;value=3056")!=std::string::npos,"Real SDK bare ampersands become canonical HTML without changing semantic URI values");
 EntryHtmlSource source;source.html="Original &amp; café";auto full=prepare_export_html(source,8192);
 check(prepare_export_html(source,full.html.size()).html==full.html,"Exact serialized source cap accepted");
 check(prepare_export_html(source,1).limited,"Source byte clipping is not successful HTML");
 source.css="body {color:red}";check(prepare_export_html(source,8192).issue==HtmlExportIssue::styles,"Arbitrary module/host CSS is not mistaken for core snapshot CSS");
 // Count the parser's html/body wrappers as well as source nodes. These
 // boundaries protect CPU/stack work separately from the final byte budget.
 auto nested=[](unsigned count){std::string html;for(unsigned i=0;i<count;++i)html+="<div>";html+="Original";for(unsigned i=0;i<count;++i)html+="</div>";return html;};
 EntryHtmlSource deep;deep.html=nested(126);
 check(prepare_export_html(deep,8192).issue==HtmlExportIssue::none,"Depth boundary includes parser wrappers and final text node");
 deep.html=nested(127);const auto too_deep=prepare_export_html(deep,8192);
 check(too_deep.issue==HtmlExportIssue::markup&&too_deep.html.empty(),"One node beyond traversal depth returns no fragment");
 EntryHtmlSource wide;wide.html.reserve(1300000);for(unsigned i=0;i<99998;++i)wide.html+="<span></span>";
 check(prepare_export_html(wide,maximum_html_source_bytes).issue==HtmlExportIssue::none,"Exact aggregate node boundary is accepted within byte budget");
 wide.html+="<span></span>";const auto too_wide=prepare_export_html(wide,maximum_html_source_bytes);
 check(too_wide.issue==HtmlExportIssue::markup&&too_wide.html.empty(),"One node beyond traversal count returns no fragment");
 std::cout<<"Passive HTML subset, scoped core CSS, Unicode, unsupported assets and exact byte guards passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
