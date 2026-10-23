// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
using namespace xiphos::linux_preview;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void settle(ReaderModel &model){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);do{model.tick();check(std::chrono::steady_clock::now()<end,"URI model deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(model.busy());check(model.error().empty(),model.error().c_str());}
int main(int argc,char **argv){try {
 check(argc==2,"Original navigation fixture required");ReaderModel model(argv[1]);settle(model);
 check(model.visit("Text","Personal","John3:17"),"Seed paired panes");settle(model);
 SearchInput input;input.modules={"Text"};input.expression="Original";check(model.search(input),"Seed search");settle(model);const auto generation=model.search_results().generation.id;
 check(model.follow("Personal","sword://N/John3:16-18"),"Follow commentary range alias");settle(model);
 check(model.passage().bible=="Text"&&model.passage().commentary=="Notes"&&model.passage().reference=="John 3:16"&&model.passage().commentary_html.find("Original Notes")!=std::string::npos&&model.can_back(),"Actual commentary target applies through shared navigation/history");
 check(model.follow("Notes","sword://Missing/Genesis1:1-2"),"Follow unknown URI range");settle(model);
 check(model.passage().bible=="Text"&&model.passage().reference=="Genesis 1:1"&&model.passage().bible_html.find("Original Text")!=std::string::npos,"Unknown range uses main Bible rather than source commentary");
 check(model.follow("Notes","bible://P/John3:16-18"),"Follow personal commentary range");settle(model);
 check(model.passage().commentary=="Personal"&&model.passage().commentary_html.find("Original Personal")!=std::string::npos,"Personal commentary normalized target");
 const auto before=model.passage();const auto back=model.can_back();
 check(model.follow("Personal","sword://T/John3:16-18"),"Preview Bible URI range");settle(model);
 check(model.references().module=="Text"&&model.references().count==3&&model.passage().reference==before.reference&&model.passage().bible_html==before.bible_html&&model.can_back()==back&&model.search_results().generation.id==generation,"Bible URI ranges retain sidebar preview and search/history isolation");
 check(model.hover("sword://D/alpha-beta;gamma,delta/part%23tag!note?query"),"Queue dictionary URI hover");
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(20);
 do{model.tick();check(std::chrono::steady_clock::now()<deadline,"Dictionary hover model deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(model.preview_busy());
 check(model.information().module=="Dictionary"&&model.information().reference=="ALPHA-BETA;GAMMA,DELTA/PART#TAG!NOTE?QUERY"&&model.information().html.find("Original Dictionary")!=std::string::npos&&model.passage().reference==before.reference&&model.can_back()==back&&model.search_results().generation.id==generation,"Typed dictionary information preview retains arbitrary SDK key and reading/search/history");
 std::cout<<"Portable normalized URI navigation and dictionary information preview passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
