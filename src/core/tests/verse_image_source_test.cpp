// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/session.hpp"
#include "xiphos/core/render.hpp"
#include <rawtext.h>
#include <rawcom.h>
#include <markupfiltmgr.h>
#include <libxml/HTMLparser.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::vector<std::string> images(const std::string &html){
 std::unique_ptr<xmlDoc,decltype(&xmlFreeDoc)> doc(htmlReadMemory(html.data(),static_cast<int>(html.size()),nullptr,"UTF-8",HTML_PARSE_NONET|HTML_PARSE_NODEFDTD|HTML_PARSE_NOERROR|HTML_PARSE_NOWARNING),xmlFreeDoc);
 check(bool(doc),"Parse generated original-prose SDK image source");
 std::vector<std::string> result;std::vector<xmlNode *> pending{xmlDocGetRootElement(doc.get())};
 while(!pending.empty()){
  auto *n=pending.back();pending.pop_back();if(!n)continue;
  if(n->next)pending.push_back(n->next);if(n->children)pending.push_back(n->children);
  if(n->type==XML_ELEMENT_NODE&&xmlStrcasecmp(n->name,BAD_CAST "img")==0){
   std::unique_ptr<xmlChar,decltype(xmlFree)> src(xmlGetProp(n,BAD_CAST "src"),xmlFree);
   check(bool(src),"SDK image keeps a source");result.emplace_back(reinterpret_cast<const char *>(src.get()));
  }
 }
 return result;
}
int main(int argc,char **argv){try{
 check(argc==2,"Private original image-source fixture root required");const fs::path root(argv[1]);
 fs::create_directories(root/"mods.d");
 const std::vector<std::string> inputs{"original.png","caf%C3%A9.png","a+b.png","literal%2520.png","missing.png","../outside.png","https://example.invalid/original.png"};
 const std::vector<std::string> ids{"ThMLText","ThMLNotes","OSISText","OSISNotes"};
 for(const auto &id:ids){
  const auto data=root/"data"/id;fs::create_directories(data);
  const bool commentary=id.find("Notes")!=std::string::npos,osis=id.find("OSIS")==0;
  if(commentary){check(sword::RawCom::createModule(data.string().c_str())==0,"Create original commentary image module");}
  else check(sword::RawText::createModule(data.string().c_str())==0,"Create original Bible image module");
  std::unique_ptr<sword::SWModule> writer;
  if(commentary)writer=std::make_unique<sword::RawCom>(data.string().c_str());else writer=std::make_unique<sword::RawText>(data.string().c_str());
  unsigned verse=1;
  for(const auto &source:inputs){
   writer->setKey(("John 1:"+std::to_string(verse++)).c_str());
   const auto html=std::string("Original image-source prose ")+(osis?"<figure src=\"":"<img src=\"")+source+(osis?"\"/>":"\" />");writer->setEntry(html.c_str());
  }
  writer.reset();
  std::ofstream config(root/"mods.d"/(id+".conf"));
  config<<'['<<id<<"]\nDataPath=./data/"<<id<<"/\nModDrv="<<(commentary?"RawCom":"RawText")<<"\nSourceType="<<(osis?"OSIS":"ThML")<<"\nEncoding=UTF-8\n";
 }
 fs::remove(root/"mods.d/modules-conf.cache");
 {
  EngineManager manager(root.string().c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);
  for(const auto &id:ids){auto *module=manager.getModule(id.c_str());check(module,"Original image module installed");
   const auto *directory=module->getConfigEntry("AbsoluteDataPath");check(directory&&fs::equivalent(fs::u8path(directory),root/"data"/id),"Actual SDK resource context resolves to this row module's directory");
   for(unsigned verse=1;verse<=7;++verse){const auto rendered=render_entry(*module,("John 1:"+std::to_string(verse)).c_str());const auto sources=images(rendered);
    check(sources.size()==1,"Each SDK row yields exactly one image");
    const auto expected=id.find("OSIS")==0?std::string("file:")+directory+inputs[verse-1]:inputs[verse-1];
    check(sources[0]==expected,"ThML preserves image URLs; OSIS prefixes even missing, parent and network-looking sources with the exact SDK data path");
   }
  }
 }
 {
  Reader reader(root.string());StudySession session(reader);session.enable_module_preferences();
  for(const auto &id:ids){
   session.visit(id,"John1:1");const auto history=session.history().entries().size();
   const auto before=session.module_presentation_options(id);
   const auto snapshot=session.capture_entry_html(id,"John1:1");
   check(images(snapshot.html).size()==1&&session.module_presentation_options(id).image_content==before.image_content,"Configured source keeps the image without publishing learned preferences");
   session.set_image_content(id,false);const auto off=session.capture_entry_html(id,"John1:1");
   check(images(off.html).empty()&&off.html.find("Original image-source prose")!=std::string::npos,"Explicit image-off policy keeps original prose with no image requirement");
   check(session.history().entries().size()==history&&!session.book_selection(),"Image-source reads do not create visits or book selections");
  }
 }
 std::cout<<"Original Bible/commentary image source characterization passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
