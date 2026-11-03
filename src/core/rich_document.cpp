// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/rich_document.hpp"
#include "xiphos/core/link.hpp"
#include "xml_document.hpp"
#include <libxml/HTMLparser.h>
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <stdexcept>
#include <tuple>
namespace xiphos::core {
namespace {
constexpr std::size_t max_runs=16384;
void invalid(){throw std::invalid_argument("Unsupported or invalid rich document; preserve/edit its original source");}
bool equal(const RichStyle &a,const RichStyle &b){return std::tie(a.flags,a.block,a.foreground,a.background,a.link)==std::tie(b.flags,b.block,b.foreground,b.background,b.link);}
std::string lower(std::string s){for(auto &c:s)if(c>='A'&&c<='Z')c+=32;return s;}
std::string trim(std::string s){const auto first=s.find_first_not_of(" \t\r\n");return first==s.npos?"":s.substr(first,s.find_last_not_of(" \t\r\n")-first+1);}
std::string color(std::string s){s=lower(trim(s));if(s.empty())return s;static const std::map<std::string,std::string> names{{"black","#000000"},{"white","#ffffff"},{"red","#ff0000"},{"green","#008000"},{"blue","#0000ff"},{"yellow","#ffff00"},{"gray","#808080"},{"grey","#808080"},{"purple","#800080"},{"orange","#ffa500"}};auto n=names.find(s);if(n!=names.end())return n->second;if(s.size()==4&&s[0]=='#'){std::string full="#";for(std::size_t i=1;i<4;++i){full+=s[i];full+=s[i];}s=std::move(full);}if(s.size()!=7||s[0]!='#'||!std::all_of(s.begin()+1,s.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');}))invalid();return s;}
bool uri(const std::string &s){if(s.empty())return true;if(s.size()>4096||!editable_document_text(s)||std::any_of(s.begin(),s.end(),[](unsigned char c){return c<=32||c==127;}))return false;auto v=lower(s);for(auto p:{"sword://","bible://","https://","http://","mailto:"})if(v.rfind(p,0)==0)return true;
 if(v.rfind("passagestudy.jsp?",0)==0||v.rfind("xiphos.url?",0)==0){
   // Validate command syntax with the same core router used by reading panes.
   // Context is supplied at activation, not embedded into the saved document.
   try{(void)contextual_study_link(s,"editor-context");return true;}catch(const std::invalid_argument &){return false;}
 }
 return false;}
std::string escape(const std::string &s){std::string out;for(char c:s){switch(c){case '&':out+="&amp;";break;case '<':out+="&lt;";break;case '>':out+="&gt;";break;case '"':out+="&quot;";break;default:out+=c;}if(out.size()>maximum_editable_document_bytes)invalid();}return out;}

void append(RichDocument &d,std::string text,const RichStyle &style){if(text.empty())return;if(!d.runs.empty()&&equal(d.runs.back().style,style))d.runs.back().text+=text;else{if(d.runs.size()>=max_runs)invalid();d.runs.push_back({std::move(text),style});}}
std::string prop(xmlNode *n,const char *key){auto *p=xmlGetProp(n,BAD_CAST key);if(!p)return {};std::unique_ptr<xmlChar,decltype(xmlFree)> owned(p,xmlFree);return reinterpret_cast<const char *>(p);}
struct Parser {
 RichDocument document;std::size_t bytes=0;bool synthetic=false;
 void add(const std::string &text,const RichStyle &style,bool generated=false){auto cost=text.size();if(document.runs.empty()||!equal(document.runs.back().style,style))cost+=style.link.size()+style.foreground.size()+style.background.size();if(cost>maximum_editable_document_bytes-bytes)invalid();bytes+=cost;append(document,text,style);if(!text.empty())synthetic=generated;}
 void boundary(const RichStyle &style){if(!document.runs.empty()&&document.runs.back().text.back()!='\n')add("\n",style,true);}
 void walk(xmlNode *nodes,RichStyle style,int list=0,unsigned depth=0){if(depth>64)invalid();for(auto *n=nodes;n;n=n->next){
  if(n->type==XML_TEXT_NODE){if(n->content)add(reinterpret_cast<const char *>(n->content),style);continue;}
  if(n->type!=XML_ELEMENT_NODE)invalid();auto tag=lower(reinterpret_cast<const char *>(n->name));auto next=style;
  for(auto *p=n->properties;p;p=p->next){auto key=lower(reinterpret_cast<const char *>(p->name));if(!((tag=="a"&&key=="href")||(tag=="span"&&key=="style")||(tag=="font"&&key=="color")||(tag=="ol"&&key=="type")))invalid();}
  bool block=false;
  if(tag=="b"||tag=="strong")next.flags|=1;else if(tag=="i"||tag=="em")next.flags|=2;else if(tag=="u")next.flags|=4;else if(tag=="s"||tag=="strike"||tag=="del")next.flags|=8;
  else if(tag=="br"){add("\n",style);continue;}
  else if(tag=="a"){next.link=prop(n,"href");if(!uri(next.link))invalid();}
  else if(tag=="font"){const auto value=trim(prop(n,"color"));if(!value.empty())next.foreground=color(value);}
  else if(tag=="span"){auto css=prop(n,"style");std::size_t at=0;while(at<css.size()){auto end=css.find(';',at);auto part=trim(css.substr(at,end==css.npos?css.npos:end-at));if(!part.empty()){auto colon=part.find(':');if(colon==part.npos)invalid();auto key=lower(trim(part.substr(0,colon)));auto value=part.substr(colon+1);if(key=="color")next.foreground=color(value);else if(key=="background-color")next.background=color(value);else if(key=="text-decoration"){auto decoration=lower(trim(value));unsigned flags=0;if(decoration!="none"){std::size_t pos=0;while(pos<decoration.size()){auto end=decoration.find_first_of(" \t\r\n",pos);auto token=decoration.substr(pos,end==decoration.npos?decoration.npos:end-pos);unsigned bit=token=="underline"?4u:token=="line-through"?8u:0u;if(!bit||(flags&bit))invalid();flags|=bit;if(end==decoration.npos)break;pos=decoration.find_first_not_of(" \t\r\n",end);if(pos==decoration.npos)break;}if(!flags)invalid();}next.flags=(next.flags&~12u)|(style.flags&12u)|flags;}else if(key=="font-weight"){const auto weight=lower(trim(value));if(weight=="bold"||weight=="700")next.flags|=1u;else if(weight=="normal"||weight=="400"){if(static_cast<int>(next.block)>=1&&static_cast<int>(next.block)<=6)invalid();next.flags&=~1u;}else invalid();}else if(key=="font-style"){const auto font=lower(trim(value));if(font=="italic")next.flags|=2u;else if(font=="normal"){if(next.block==RichBlock::address)invalid();next.flags&=~2u;}else invalid();}else invalid();}if(end==css.npos)break;at=end+1;}}
  else if(tag=="ul"||tag=="ol"){if(list)invalid();int kind=9;if(tag=="ol"){const auto type=lower(prop(n,"type"));if(!type.empty()&&type!="1"&&type!="a"&&type!="i")invalid();kind=type=="a"?12:type=="i"?11:10;}boundary(style);walk(n->children,style,kind,depth+1);continue;}
  else if(tag=="li"){if(!list)invalid();next.block=static_cast<RichBlock>(list);block=true;}
  else if(tag.size()==2&&tag[0]=='h'&&tag[1]>='1'&&tag[1]<='6'){next.block=static_cast<RichBlock>(tag[1]-'0');block=true;}
  else if(tag=="address"||tag=="pre"||tag=="p"||tag=="div"){if(list)invalid();next.block=tag=="address"?RichBlock::address:tag=="pre"?RichBlock::pre:RichBlock::normal;block=true;}
  else invalid();
  if(block)boundary(style);walk(n->children,next,list,depth+1);if(block)boundary(next);
 }}
};
}
RichDocument normalize_rich_document(const RichDocument &d){
 RichDocument out;int block=-1;std::size_t bytes=0;
 if(d.runs.size()>max_runs)invalid();
 for(const auto &r:d.runs){if(!editable_document_text(r.text)||r.text.size()+r.style.link.size()+r.style.foreground.size()+r.style.background.size()>maximum_editable_document_bytes-bytes)invalid();bytes+=r.text.size()+r.style.link.size()+r.style.foreground.size()+r.style.background.size();std::size_t at=0;while(at<r.text.size()){if(block<0)block=static_cast<int>(r.style.block);auto end=r.text.find('\n',at);auto count=end==r.text.npos?r.text.size()-at:end-at+1;auto style=r.style;style.block=static_cast<RichBlock>(block);append(out,r.text.substr(at,count),style);at+=count;if(end!=r.text.npos)block=-1;}}
 validate_rich_document(out);return out;
}
void validate_rich_document(const RichDocument &d){
 if(d.runs.size()>max_runs)invalid();std::size_t bytes=0;int line=-1;
 for(const auto &r:d.runs){detail::validate_xml_text(r.text,maximum_editable_document_bytes);if(!editable_document_text(r.text)||r.text.empty()||r.style.flags>15||static_cast<int>(r.style.block)<0||static_cast<int>(r.style.block)>12||!uri(r.style.link))invalid();if(r.text.size()+r.style.link.size()+r.style.foreground.size()+r.style.background.size()>maximum_editable_document_bytes-bytes)invalid();bytes+=r.text.size()+r.style.link.size()+r.style.foreground.size()+r.style.background.size();if(color(r.style.foreground)!=r.style.foreground||color(r.style.background)!=r.style.background)invalid();
  for(char c:r.text){if(line==-1)line=static_cast<int>(r.style.block);if(line!=static_cast<int>(r.style.block))invalid();if(c=='\n')line=-1;}
 }
}
RichDocument parse_rich_document(const std::string &html){
 if(!editable_document_text(html))invalid();detail::validate_xml_text(html,maximum_editable_document_bytes);if(html.empty())return {};auto low=lower(html);if(low.find("<!entity")!=low.npos||low.find("<!doctype")!=low.npos||low.find("<!--")!=low.npos||low.find("<?")!=low.npos)invalid();
 std::unique_ptr<xmlDoc,decltype(&xmlFreeDoc)> doc(htmlReadMemory(html.data(),static_cast<int>(html.size()),nullptr,"UTF-8",HTML_PARSE_NONET|HTML_PARSE_NOERROR|HTML_PARSE_NOWARNING|HTML_PARSE_NODEFDTD),xmlFreeDoc);if(!doc)invalid();
 auto *root=xmlDocGetRootElement(doc.get());if(!root||root->properties)invalid();xmlNode *body=nullptr;for(auto *n=root?root->children:nullptr;n;n=n->next){auto name=lower(reinterpret_cast<const char *>(n->name));if(name=="body")body=n;else if(name=="head")for(auto *h=n->children;h;h=h->next)if(h->type==XML_ELEMENT_NODE)invalid();}
 if(!body||body->properties)invalid();Parser parser;parser.walk(body->children,{});if(parser.synthetic&&!parser.document.runs.empty()){parser.document.runs.back().text.pop_back();if(parser.document.runs.back().text.empty())parser.document.runs.pop_back();}validate_rich_document(parser.document);return parser.document;
}
std::string rich_document_text(const RichDocument &d){validate_rich_document(d);std::string text;for(const auto &r:d.runs)text+=r.text;return text;}
std::string serialize_rich_document(const RichDocument &d){
 validate_rich_document(d);std::string out,line;int block=0;bool have=false;
 int list=-1;
 auto close_list=[&]{if(list>=9){out+=list==9?"</ul>":"</ol>";list=-1;}};
 auto emit=[&](bool newline){
  static const std::array<const char *,9> open{{"","<h1>","<h2>","<h3>","<h4>","<h5>","<h6>","<address>","<pre>"}};
  static const std::array<const char *,9> close{{"","</h1>","</h2>","</h3>","</h4>","</h5>","</h6>","</address>","</pre>"}};
  if(list!=block)close_list();
  if(block>=9){if(list!=block){out+=block==9?"<ul>":block==10?"<ol>":block==11?"<ol type=\"i\">":"<ol type=\"a\">";list=block;}out+="<li>";}else out+=open[block];
  out+=line;if(newline)out+="<br/>";out+=block>=9?"</li>":close[block];if(out.size()>maximum_editable_document_bytes)invalid();line.clear();have=false;
 };
 for(const auto &r:d.runs){std::size_t at=0;while(at<r.text.size()){if(!have){block=static_cast<int>(r.style.block);have=true;}auto end=r.text.find('\n',at);auto part=r.text.substr(at,end==r.text.npos?r.text.npos:end-at);std::string open,close;auto tag=[&](const std::string &a,const std::string &b){open+=a;close=b+close;};if(!r.style.link.empty())tag("<a href=\""+escape(r.style.link)+"\">","</a>");if(!r.style.foreground.empty()||!r.style.background.empty()){std::string css;if(!r.style.foreground.empty())css+="color:"+r.style.foreground+";";if(!r.style.background.empty())css+="background-color:"+r.style.background+";";tag("<span style=\""+css+"\">","</span>");}for(const auto &t:std::array<std::tuple<unsigned,const char *,const char *>,4>{{{1,"<b>","</b>"},{2,"<i>","</i>"},{4,"<u>","</u>"},{8,"<s>","</s>"}}})if(r.style.flags&std::get<0>(t))tag(std::get<1>(t),std::get<2>(t));line+=open+escape(part)+close;if(line.size()>maximum_editable_document_bytes)invalid();if(end==r.text.npos)break;emit(true);at=end+1;} }
 if(have)emit(false);close_list();if(out.size()>maximum_editable_document_bytes)invalid();return out;
}
RichDocument format_rich_document(const RichDocument &d,std::size_t start,std::size_t length,const RichCommand &cmd){
 const auto text=rich_document_text(d);if(start>text.size()||length>text.size()-start)invalid();auto boundary=[&](std::size_t p){return p==text.size()||(static_cast<unsigned char>(text[p])&0xc0)!=0x80;};if(!boundary(start)||!boundary(start+length))invalid();auto end=start+length;const int action=static_cast<int>(cmd.action);if(action<0||action>8)invalid();
 if(!length||cmd.action==RichAction::block||cmd.action==RichAction::clear){auto before=start?text.rfind('\n',start-1):text.npos;start=before==text.npos?0:before+1;auto after=text.find('\n',end>start&&end&&text[end-1]=='\n'?end-1:end);end=after==text.npos?text.size():after+1;}
 RichStyle initial;std::size_t offset=0;for(const auto &r:d.runs){if(start<offset+r.text.size()){initial=r.style;break;}offset+=r.text.size();}
 if(action<=3&&(cmd.value<-1||cmd.value>1))invalid();if(cmd.action==RichAction::block&&(cmd.value<0||cmd.value>12))invalid();auto c=cmd.text;if(cmd.action==RichAction::foreground||cmd.action==RichAction::background)c=color(c);if(cmd.action==RichAction::link&&!uri(c))invalid();
 RichDocument out;offset=0;for(const auto &r:d.runs){const auto finish=offset+r.text.size();const auto a=std::max(start,offset),b=std::min(end,finish);if(a>=b)append(out,r.text,r.style);else{append(out,r.text.substr(0,a-offset),r.style);auto style=r.style;if(action<=3){const auto bit=1u<<action;const bool on=cmd.value==-1?!(initial.flags&bit):cmd.value!=0;if(on)style.flags|=bit;else style.flags&=~bit;}else if(cmd.action==RichAction::block)style.block=static_cast<RichBlock>(cmd.value);else if(cmd.action==RichAction::foreground)style.foreground=c;else if(cmd.action==RichAction::background)style.background=c;else if(cmd.action==RichAction::link)style.link=c;else style={};append(out,r.text.substr(a-offset,b-a),style);append(out,r.text.substr(b-offset),r.style);}offset=finish;}
 validate_rich_document(out);return out;
}
}
