// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::linux_preview;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
void settle(ReaderModel &model,bool success=true){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);do{model.tick();check(std::chrono::steady_clock::now()<end,"Book URI model deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(model.busy());if(success)check(model.error().empty(),model.error().c_str());}
int main(int argc,char **argv){try{
 check(argc==2,"Original book URI fixture required");xc_worker *owner=nullptr;check(xc_worker_open(argv[1],&owner)==XC_OK,"Open one shared book URI owner");
 {ReaderModel model(*owner),other(*owner);settle(model);settle(other);check(model.visit("Text","","John3:17"),"Seed main verse");settle(model);const auto before=model.passage();const auto back=model.can_back(),forward=model.can_forward();const auto generation=model.search_results().generation.id;
  check(model.follow("Text","sword://Manual/%2FPart+One#uri-book-target"),"Follow main book URI");settle(model);
  check(model.book().module=="Book"&&model.book().nodes[0].path=="/Part One"&&model.book().html.find("Original Book")!=std::string::npos&&model.book().html.find("uri-book-target")!=std::string::npos&&model.book().anchor=="uri-book-target"&&model.passage().reference==before.reference&&model.can_back()==back&&model.can_forward()==forward&&other.book().module.empty(),"Book target consumes actual shared document and anchor without main/history/window visit");
  check(model.follow_book("bible://Pray/%2F123+Numeric+Title#"),"Follow book-origin prayer URI");settle(model);check(model.book().module=="Prayer"&&model.book().nodes[0].path=="/123 Numeric Title"&&model.book().anchor&&model.book().anchor->empty(),"Prayer indexed-book source and explicit top anchor");
  check(model.follow_book("sword://Manual/0"),"Follow numeric startup URI");settle(model);check(model.book().module=="Book"&&model.book().nodes[0].path=="/Part One"&&!model.book().anchor,"Numeric URI uses minimum four and ordinary document clears anchor");
  check(model.follow_book("sword:///John3:16"),"Book-origin main Bible URI");settle(model);check(model.passage().bible=="Text"&&model.passage().reference=="John 3:16"&&model.book().module=="Book","Book-origin omitted module uses main reading context");
  const auto reading=model.passage();const auto history=model.can_back();
  check(model.follow_book("sword://T/16-17"),"Book-origin relative Bible preview");settle(model);check(model.references().count==2&&model.references().html.find("John 3:16")!=std::string::npos&&model.passage().reference==reading.reference&&model.can_back()==history,"Relative preview uses main reference, not tree path, without navigation");
  check(model.read_book_key(""),"Read explicit tree root");settle(model);check(model.book().nodes[0].offset==0&&model.book().nodes[0].path.empty()&&!model.book().anchor,"Explicit empty path remains root and clears anchor");
  const auto kept=model.book().html;check(model.follow_book("sword://Manual/%2FMissing"),"Queue missing book URI");settle(model,false);check(!model.error().empty()&&model.book().html==kept&&model.book().nodes[0].offset==0&&other.book().module.empty()&&model.search_results().generation.id==generation,"Failed book URI retains committed document/search/sibling state");
 }
 xc_worker_close(owner);std::cout<<"Portable actual book/prayer URI selection, book-origin reading context, transient anchors and state isolation passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
