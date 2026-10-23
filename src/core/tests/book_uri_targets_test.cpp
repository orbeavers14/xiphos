// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <rawgenbook.h>
#include <rawtext.h>
#include <treekeyidx.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <stdexcept>
using namespace xiphos::core;using namespace sword;namespace fs=std::filesystem;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class E,class F>void rejects(F f){try{f();}catch(const E &){return;}throw std::runtime_error("Expected rejected book URI target");}
xc_status wait(xc_request *r){xc_status s;const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Book URI deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return s;}
void fixture(const fs::path &root){fs::remove_all(root);fs::create_directories(root/"mods.d");fs::create_directories(root/"data");
 for(const auto *id:{"Book","Prayer"}){
  const auto prefix=(root/"data"/id).string();check(RawGenBook::createModule(prefix.c_str())==0,"Create original book URI fixture");
  {RawGenBook writer(prefix.c_str());for(const auto *path:{"/Part One","/Part One/Leaf -;, café","/123 Numeric Title","/Question? Branch","/Bang!before","/Literal%20Path"}){writer.setKey(path);writer.setEntry(("<a name=\"uri-book-target\"></a><p>Original "+std::string(id)+" URI prose for "+path+".</p>").c_str());}}
  std::ofstream config(root/"mods.d"/(std::string(id)+".conf"));config<<'['<<id<<"]\nDataPath=./data/"<<id<<"\nModDrv=RawGenBook\nSourceType=ThML\nEncoding=UTF-8\nDescription=Original book URI fixture\nAbbreviation="<<(std::string(id)=="Book"?"Manual":"Pray")<<"\n";if(std::string(id)=="Prayer")config<<"GSType=PrayerList\n";
 }
 {std::ofstream config(root/"mods.d/verse-key.conf");config<<"[UnsafeBook]\nDataPath=./data/Book\nModDrv=RawGenBook\nKeyType=VerseKey\nAbbreviation=Unsafe\n";}
 fs::create_directories(root/"data/Text");check(RawText::createModule((root/"data/Text").string().c_str())==0,"Create original Bible URI fixture");
 {RawText writer((root/"data/Text").string().c_str());writer.setKey("John3:16");writer.setEntry("Original main Bible URI prose.");writer.setKey("John3:17");writer.setEntry("Original next Bible URI prose.");}
 {std::ofstream config(root/"mods.d/text.conf");config<<"[Text]\nDataPath=./data/Text/\nModDrv=RawText\nSourceType=Plain\nEncoding=UTF-8\nLang=en\nAbbreviation=T\n";}
}
int main(int argc,char **argv){try{
 check(argc==2,"Disposable book URI root required");const std::string root=argv[1];fixture(fs::u8path(root));std::uint64_t part=0;
 {RawGenBook source((fs::u8path(root)/"data/Book").string().c_str());source.setKey("/Part One");(void)source.getKey()->getText();part=dynamic_cast<TreeKeyIdx *>(source.getKey())->getOffset();check(part>=4,"Original part source has indexed offset");}
 std::future<ContextualStudyLink> accepted;BookUriTarget held;
 {StudyWorker worker(root);const auto tab=worker.create_tab().get(),other=worker.create_tab().get();(void)worker.visit_passage(tab,"Text","","John3:17").get();const auto history=worker.history(tab).get();const auto generation=worker.search_generation(tab).get().id;
  for(const auto &row:std::vector<std::pair<std::string,std::string>>{{"%2FPart+One","/Part One"},{"Part+One","/Part One"},{"%2FPart+One%2FLeaf+-;,+caf%C3%A9","/Part One/Leaf -;, café"},{"%2F123+Numeric+Title","/123 Numeric Title"},{"%2FQuestion?+Branch","/Question? Branch"},{"%2FBang!before%23after","/Bang!before"},{"%2FLiteral%2520Path","/Literal%20Path"},{std::to_string(part)+"trailing","/Part One"},{"0","/Part One"},{"3","/Part One"},{"%23root",""}}){
   held=std::get<BookUriTarget>(worker.follow_pane_link_at(tab,"Text","Text","John3:17","sword://Manual/"+row.first).get());
   check(held.tree.module=="Book"&&held.tree.node.path==row.second,"Book URI returns actual alias-resolved SDK path across numeric-prefix/path/Unicode/punctuation rules");
   if(row.first=="%2FBang!before%23after")check(held.key=="/Bang!before"&&held.anchor=="after","Book key uses hash-over-bang precedence");
   if(row.first=="%23root")check(held.tree.node.offset==0&&held.key.empty()&&held.anchor=="root","Explicit path root stays distinct from numeric clamp");
  }
  const auto prayer=std::get<BookUriTarget>(worker.resolve_study_link_at(tab,"Text","John3:17","bible://Pray/%2FPart+One#named").get());check(prayer.tree.module=="Prayer"&&prayer.anchor=="named","Prayer-list GenericBook target shares validated tree policy");
  const auto main=std::get<BookUriTarget>(worker.follow_pane_link_at(tab,"Book","Text","John3:17","sword:///%2FPart+One").get());check(main.tree.module=="Book","Omitted URI token uses explicit main context rather than source");
  const auto verse=std::get<PassageLink>(worker.follow_pane_link_at(tab,"Text","Book","John3:17","sword:///John3:16").get());check(verse.module=="Text","Book source does not replace omitted URI main Bible");
  check(std::holds_alternative<std::monostate>(worker.hover_study_link_at(tab,"Text","John3:17","sword://Manual/%2FPart+One").get()),"Book hover remains no information preview");
  rejects<std::out_of_range>([&]{(void)worker.resolve_study_link_at(tab,"Text","John3:17","sword://Manual/%2FMissing").get();});
  rejects<std::invalid_argument>([&]{(void)worker.resolve_study_link_at(tab,"Text","John3:17","sword://Unsafe/%2FPart+One").get();});
  for(const auto &bad:std::vector<std::string>{"sword://Manual/2147483648","sword://Manual/a%GG",std::string("sword://Manual/")+std::string(4097,'a')})rejects<std::invalid_argument>([&]{(void)worker.follow_study_link_at(tab,"Text","John3:17",bad).get();});
  check(!worker.book_selection(tab).get()&&!worker.book_selection(other).get()&&worker.history(tab).get().entries.size()==history.entries.size()&&worker.dictionary_history(tab).get().entries.empty()&&worker.search_generation(tab).get().id==generation,"Book target resolution does not select/render/persist or visit Bible/dictionary/search/sibling state");
  std::string uri="sword://Manual/%2FPart+One#copied";accepted=worker.follow_pane_link_at(tab,"Text","Text","John3:17",uri);uri.clear();worker.close_tab(tab).get();rejects<std::invalid_argument>([&]{(void)worker.follow_study_link_at(tab,"Text","John3:17","sword://Manual/0").get();});
 }
 held=std::get<BookUriTarget>(accepted.get());check(held.tree.module=="Book"&&held.anchor=="copied","Book target owns source/key/anchor through drain and shutdown");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C book URI owner");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_follow_pane_link_at(worker,tab,"Text","Text","John3:17","sword://Manual/%2FPart+One#",&request)==XC_OK,"C book target submission");xc_worker_close(worker);check(wait(request)==XC_OK,"C book target drains");auto *target=xc_request_take_book_uri_target(request);check(target&&!xc_request_take_book_uri_target(request)&&!xc_request_take_link(request)&&!xc_request_take_book_document(request),"C distinct book target kind/take once");xc_request_release(request);xc_book_node node{};check(xc_book_tree_get_node(xc_book_uri_target_tree(target),XC_BOOK_CURRENT,&node)==XC_OK&&std::string(node.path)=="/Part One"&&std::string(xc_book_tree_module(xc_book_uri_target_tree(target)))=="Book"&&std::string(xc_book_uri_target_key(target))=="/Part One"&&xc_book_uri_target_anchor(target)&&!*xc_book_uri_target_anchor(target),"Borrowed tree/key/explicit-empty anchor survive request and worker");xc_book_uri_target_release(target);
 check(!xc_book_uri_target_tree(nullptr)&&!xc_book_uri_target_key(nullptr)&&!xc_book_uri_target_anchor(nullptr),"Null book target getters");xc_book_uri_target_release(nullptr);
 std::cout<<"Resolved general-book/prayer URI targets: actual aliases, shared display keys, SDK paths, owned anchors and selection-neutral C lifetime passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
