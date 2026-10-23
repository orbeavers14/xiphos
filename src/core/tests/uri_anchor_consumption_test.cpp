// SPDX-License-Identifier: GPL-2.0-or-later
#include "passage_uri_test_host.hpp"
#include <cassert>
struct FakeHtml { std::string pane, bytes; };
using GtkWidget=FakeHtml;using XiphosHtml=FakeHtml;struct MOD_FONT {};
static void open_html(FakeHtml *html,const char *kind){html->bytes.clear();call("stream-open",html->pane.c_str(),kind);}
static void write_html(FakeHtml *html,const char *text,std::size_t size){html->bytes.append(text,size);}
static void close_html(FakeHtml *html){call("stream-html",html->pane.c_str(),html->bytes.c_str());}
static void jump_html(FakeHtml *html,const char *anchor){call("jump",html->pane.c_str(),anchor);}
#define XIPHOS_HTML(value) (value)
#define XIPHOS_HTML_OPEN_STREAM(value,kind) open_html(value,kind)
#define XIPHOS_HTML_WRITE(value,text,size) write_html(value,text,size)
#define XIPHOS_HTML_CLOSE(value) close_html(value)
#define XIPHOS_HTML_JUMP_TO_ANCHOR(value,anchor) jump_html(value,anchor)
namespace frozen_output {
#include "legacy_uri_html_output.inc"
}
namespace actual_output {
#include "legacy_uri_html_output_actual.inc"
}
using Output=void (*)(char *,GtkWidget *,MOD_FONT *,char *);
template<class Handler>Trace through_output(Handler handler,Output output,const std::string &uri,bool commentary,int notebook){
 display_output=[&](const char *pane){FakeHtml html{pane,""};std::string text="<html><head></head><body>Original retained output fixture.</body></html>";output(&text[0],&html,nullptr,nullptr);};
 const auto trace=run(handler,uri,true,"Text",commentary,notebook,false);display_output={};return trace;
}
std::vector<std::string> jumps(const Trace &trace){std::vector<std::string> out;for(const auto &entry:trace.calls)if(entry.compare(0,5,"jump:")==0)out.push_back(entry);return out;}
int main(){try{
 std::size_t comparisons=0;
 for(const auto *module:{"Text","T","Commentary","Percom","Dictionary","D","Book","Prayer","Missing"})
 for(const auto *key:{"John3:16","John3:16-18"})for(const auto *anchor:{"#tag","!note","#","!before#after","#café 文字","#quote\"\\'</script>"})
 for(bool visible:{false,true})for(int page:{0,1}){
  const std::string uri=std::string("sword://")+module+"/"+key+anchor;
  const auto expected=through_output(frozen::sword_uri,frozen_output::HtmlOutput,uri,visible,page);
  const auto actual=through_output(actual::sword_uri,actual_output::HtmlOutput,uri,visible,page);
  check(actual==expected,"Complete URI plus complete HtmlOutput trace differs");++comparisons;
 }
 for(bool visible:{false,true}){
  const auto unknown=through_output(actual::sword_uri,actual_output::HtmlOutput,"sword://Missing/John3:16#tag",visible,0);
  check(jumps(unknown)==std::vector<std::string>{visible?"jump:commentary:tag":"jump:bible:tag"}&&!unknown.anchor,"Unknown fallback consumes once in first actually rendered pane");
 }
 for(const auto *module:{"Text","Commentary","Percom","Dictionary","Book","Prayer"}){
  const auto actual=through_output(actual::sword_uri,actual_output::HtmlOutput,std::string("sword://")+module+"/John3:16#",true,1);
  const auto pane=std::string(module)=="Text"?"bible":std::string(module)=="Dictionary"?"dictionary":std::string(module)=="Book"||std::string(module)=="Prayer"?"book":"commentary";
  check(jumps(actual)==std::vector<std::string>{"jump:"+std::string(pane)+":"}&&!actual.anchor,"Known family consumes explicit empty anchor once despite notebook switch");
 }
 const auto sidebar=through_output(actual::sword_uri,actual_output::HtmlOutput,"sword://T/John3:16-18#tag",true,1);check(jumps(sidebar).empty()&&sidebar.anchor&&sidebar.buffers==1,"Bible sidebar returns before HtmlOutput and retains characterized split leak");
 std::cout<<"Complete URI/HtmlOutput anchor consumption: "<<comparisons<<" frozen/current traces, first visible fallback pane, dictionary/book and empty-anchor one-use agree\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
