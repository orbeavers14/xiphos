// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include "reader_document.hpp"
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
 check(model.follow("Personal","sword://D/anchor-key!before%23after"),"Follow dictionary URI click");settle(model);
 check(model.dictionary().module=="Dictionary"&&model.dictionary().key=="ANCHOR-KEY!BEFORE"&&model.dictionary().anchor=="after"&&model.dictionary_history_count()==1&&model.passage().reference==before.reference&&model.can_back()==back&&model.search_results().generation.id==generation,"Dictionary URI click consumes typed raw key and shared history without Bible/search visit");
 const auto dictionary_revision=model.dictionary().anchor_revision;
 check(model.follow("Personal","sword://D/anchor-key!before%23after"),"Repeat dictionary URI anchor");settle(model);check(model.dictionary().anchor_revision>dictionary_revision,"Repeated dictionary anchor creates a new presentation event");
 check(model.follow("Personal","sword://D/alpha#"),"Explicit dictionary top");settle(model);check(model.dictionary().anchor&&model.dictionary().anchor->empty(),"Empty dictionary anchor requests top");
 check(model.follow("Personal","sword://D/alpha"),"Second dictionary URI click");settle(model);check(model.dictionary_can_back(),"Dictionary URI visits enable shared Back");
 check(model.navigate_dictionary_history(-1),"Navigate dictionary URI history");settle(model);check(model.dictionary().key=="ANCHOR-KEY!BEFORE"&&!model.dictionary().anchor&&model.dictionary_can_forward(),"Dictionary URI visits reuse existing history navigation");
 check(model.follow("Personal","sword://T/John3:16#main"),"Follow Bible anchored URI");settle(model);
 check(model.passage().anchor_module=="Text"&&model.passage().anchor=="main","Shared snapshot targets Bible anchor pane");
 check(model.follow("Personal","sword://N/John3:16#commentary"),"Follow commentary anchored URI");settle(model);
 const auto anchor_revision=model.passage().anchor_revision;
 check(model.passage().anchor_module=="Notes"&&model.passage().anchor=="commentary","Shared snapshot targets commentary pane only");
 check(model.follow("Notes","sword://N/John3:16#commentary"),"Repeat same URI anchor");settle(model);check(model.passage().anchor_revision>anchor_revision,"Repeated same-content anchor has a new presentation event");
 check(model.follow("Notes","sword://Missing/John3:16#fallback"),"Unknown anchored fallback");settle(model);check(model.passage().anchor_module=="Notes"&&model.passage().anchor=="fallback","Unknown fallback applies anchor to visible commentary first");
 check(model.follow("Notes","sword://T/John3:16#"),"Follow explicit top anchor");settle(model);check(model.passage().anchor&&model.passage().anchor->empty()&&model.passage().anchor_module=="Text","Explicit empty anchor differs from absent");
 check(model.visit("Text","Personal","John3:17"),"Ordinary visit after URI anchor");settle(model);check(!model.passage().anchor&&model.passage().anchor_module.empty(),"Ordinary refresh does not inherit anchor");
 const auto script=anchor_scroll_script("café 文字\"\\'</script>");
 check(script.find("café 文字\\\"\\\\'</script>")!=std::string::npos&&script.find("\\303")==std::string::npos,"Host anchor script preserves UTF-8 and escapes quote/backslash without octal byte corruption");
 check(anchor_scroll_script("")=="window.scrollTo(0, 0); true;"&&anchor_scroll_script(std::string("a\n\0b",4)).find("a\\u000a\\u0000b")!=std::string::npos,"Host top/control literal encoding");
 std::cout<<"Portable normalized URI navigation, dictionary hover/click and one-use pane anchors passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
