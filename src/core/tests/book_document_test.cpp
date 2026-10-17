// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "book_tree_source.hpp"
#include <rawgenbook.h>
#include <markupfiltmgr.h>
#include <swfilter.h>
#include <swoptfilter.h>
#include <treekeyidx.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
using namespace xiphos::core;
using namespace sword;
namespace fs=std::filesystem;
#include "legacy_book_document_actual.inc"
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected document error");}
std::map<std::string,std::string> bytes(const fs::path &path){std::map<std::string,std::string> result;for(const auto &file:fs::recursive_directory_iterator(path))if(file.is_regular_file()){std::ifstream input(file.path(),std::ios::binary);result.emplace(fs::relative(file.path(),path).generic_string(),std::string(std::istreambuf_iterator<char>(input),{}));}return result;}
struct Fault : SWFilter {char processText(SWBuf &,const SWKey *,const SWModule *module)override{if(std::string(module->getKey()->getText())=="/Branch/Leaf")throw std::runtime_error("Injected descendant rendering failure");return 0;}};
int main(int argc,char **argv){try{
 check(argc==2,"Original document fixture root required");const fs::path root(argv[1]);fs::create_directories(root/"mods.d");fs::create_directories(root/"data");
 unsigned fixture_index=0;
 for(const auto &value:std::vector<std::optional<std::string>>{std::nullopt,"","0","1","2","3","  +3trailing","-2","2147483647"}){
  const std::string id="Document"+std::to_string(fixture_index++);const auto prefix=(root/"data"/id).string();check(RawGenBook::createModule(prefix.c_str())==0,"Create original document module");
  {RawGenBook writer(prefix.c_str());writer.setKey("/Branch");writer.setEntry("<p>Original <q who=\"Jesus\">branch</q><note>Original branch note.</note></p>");
   writer.setKey("/Branch/Leaf");writer.setEntry("Original café leaf.");writer.setKey("/Branch/Leaf/Deep");writer.setEntry("Original deep section.");
   writer.setKey("/Branch/Empty/Twig");writer.setEntry("Original empty-branch child.");
   std::unique_ptr<SWKey> linked(writer.createKey());linked->setText("/Branch");(void)linked->getText();writer.setKey("/Linked");writer.linkEntry(linked.get());}
  std::ofstream config(root/"mods.d"/(id+".conf"));config<<'['<<id<<"]\nDataPath=./data/"<<id<<"\nModDrv=RawGenBook\nEncoding=UTF-8\nSourceType=OSIS\nGlobalOptionFilter=OSISFootnotes\nGlobalOptionFilter=OSISRedLetterWords\n";
  if(value)config<<"DisplayLevel="<<*value<<'\n';
 }
 {std::ofstream config(root/"mods.d/verse-book.conf");config<<"[VerseBook]\nDataPath=./data/Document0\nModDrv=RawGenBook\nKeyType=VerseKey\n";}
 fs::remove(root/"mods.d/modules-conf.cache");const auto before=bytes(root/"data");
 check(book_display_level(std::nullopt)==1 && book_display_level("")==1 && book_display_level("none")==1 && book_display_level("-2147483648")==1 && book_display_level("  +02trailing")==2 && book_display_level("2147483647")==2147483647u,"Defined atoi sign/prefix/default cases");
 for(const auto &text:{std::string("2147483648"),std::string("-2147483649"),std::string("2\0ignored",9),std::string(4097,'2')})rejects<std::invalid_argument>([&]{book_display_level(text);});
 BookDocument retained;
 {
  Reader reader(root.string());EngineManager legacy(root.string().c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);
  for(const auto &info:reader.modules()){
   if(!info.indexed_book){rejects<std::invalid_argument>([&]{reader.read_book_document(info.id,0);});continue;}
   auto &module=*legacy.getModule(info.id.c_str());auto &key=dynamic_cast<TreeKeyIdx &>(*module.getKey());
   const auto *configured=module.getConfigEntry("DisplayLevel");const auto depth=book_display_level(configured?std::make_optional(std::string(configured)):std::nullopt);
   for(const std::string path:{"", "/Branch", "/Branch/Empty", "/Linked"}){
    const auto tree=reader.book_tree_path(info.id,path);
    const auto baseline=reader.read_book_document(info.id,tree.node.offset).html;
    for(bool enabled:{false,true}){
     EngineOptions options;options.footnotes=enabled;options.words_in_red=enabled;
     ScopedEngineOptions scoped(legacy,options);key.setOffset(tree.node.offset);SWBuf actual;
     if(depth<=1)actual=module.renderText();else _render_display_level(module,tree.node.offset,static_cast<int>(depth),1,actual);
     const auto result=reader.read_book_document(info.id,tree.node.offset,options);
     check(reader.read_book_document(info.id,tree.node.offset).html==baseline,"Configured document reads restore the manager option baseline");
     check(result.tree.node.path==path && result.display_level==depth && result.html==actual.c_str(),"Document HTML matches the extracted actual legacy traversal");
     check(!result.sections.empty() && result.sections.front().entry.tree.node.offset==tree.node.offset,"Document selected source remains separate from descendants");
     for(const auto &section:result.sections){check(section.entry.tree.module==info.id && section.depth>=1 && section.depth<=depth,"Owned depth/source metadata");if(section.rendered)check(section.entry.html==reader.read_book(info.id,section.entry.tree.node.offset,options).html,"Each section retains exact configured source HTML");}
     if(depth==2 && path=="/Branch")check(result.sections.size()==3 && result.html.find("Original café")!=std::string::npos && result.html.find("Original deep")==std::string::npos && !result.sections.back().rendered,"Depth two visits entryless branches without deeper children");
     if(depth>=3 && path=="/Branch")check(result.sections.size()==5 && result.html.find("Original deep")!=std::string::npos && result.html.find("empty-branch child")!=std::string::npos,"Depth-first document includes descendants of entryless nodes");
     if(depth>1 && path=="/Branch"){
      retained=result;const auto *identity=module.getKey();key.setOffset(tree.node.offset);
      for(bool processing:{false,true}){
       module.setProcessEntryAttributes(processing);const auto snapshot=detail::capture_book_document(module,tree.node.offset,{});
       check(module.getKey()==identity && key.getOffset()==tree.node.offset && module.isProcessEntryAttributes()==processing,"Document restores selected cursor and entry-attribute flag");
       check(detail::capture_book_document(module,tree.node.offset,{64,4096,snapshot.html.size()}).html==snapshot.html,"Exact combined HTML byte boundary succeeds");
       rejects<std::runtime_error>([&]{detail::capture_book_document(module,tree.node.offset,{64,4096,snapshot.html.size()-1});});
       for(const BookDocumentLimits limits:std::vector<BookDocumentLimits>{{1,4096,8388608},{64,1,8388608},{64,4096,1}}){rejects<std::runtime_error>([&]{detail::capture_book_document(module,tree.node.offset,limits);});check(key.getOffset()==tree.node.offset && module.getKey()==identity && module.isProcessEntryAttributes()==processing,"Bounded failure restores selected source and processing flag");}
       Fault fault;module.addRenderFilter(&fault);rejects<std::runtime_error>([&]{detail::capture_book_document(module,tree.node.offset,{});});module.removeRenderFilter(&fault);
       check(key.getOffset()==tree.node.offset && module.getKey()==identity && module.isProcessEntryAttributes()==processing,"Render failure restores selected cursor/flags");
       check(detail::capture_book_document(module,tree.node.offset,{}).html==snapshot.html,"Document recovers after rendering failure");
      }
     }
    }
   }
   rejects<std::invalid_argument>([&]{reader.read_book_document(info.id,0,BookDocumentLimits{0,1,1});});
   rejects<std::out_of_range>([&]{reader.read_book_document(info.id,40000);});
  }
  rejects<std::out_of_range>([&]{reader.read_book_document("Missing",0);});
  rejects<std::invalid_argument>([&]{reader.read_book_document(std::string("Document0\0x",11),0);});
 }
 check(retained.sections.size()>=3 && retained.html.find("Original")!=std::string::npos,"Copied document outlives Reader and SDK queries");
 check(bytes(root/"data")==before,"Document queries leave all module data bytes/names unchanged");
 std::cout<<"Configured DisplayLevel documents match actual legacy depth-first rendering and preserve bounded source ownership\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
