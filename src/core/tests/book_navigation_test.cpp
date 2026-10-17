// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected book navigation failure");}
xc_status wait(xc_request *request){const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<deadline,"Navigation timeout");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Original book-entry fixture required");const std::string root=argv[1];EngineOptions options;options.footnotes=true;options.words_in_red=true;BookEntry owned;
 {
  Reader reader(root);StudySession a(reader),b(reader);a.set_engine_options(options);
  check(!a.navigate_book(BookNavigation::parent) && !a.book_selection(),"No selected source is a successful no-op");
  rejects<std::invalid_argument>([&]{a.navigate_book(static_cast<BookNavigation>(99));});
  owned=a.select_book({"OSISBook",std::uint64_t(4)});
  check(!a.navigate_book(BookNavigation::previous_sibling) && a.book_selection()->tree.node.offset==4,"Missing previous sibling keeps source");
  check(a.navigate_book(BookNavigation::parent)->tree.node.offset==4,"Legacy parent root uses the numeric reading minimum");
  check(a.navigate_book(BookNavigation::first_child)->tree.node.offset==8,"First child command uses SDK relation");
  check(!a.navigate_book(BookNavigation::first_child) && a.book_selection()->tree.node.offset==8,"Leaf child command is a no-op");
  check(a.navigate_book(BookNavigation::parent)->tree.node.offset==4,"Parent command returns branch");
  auto empty=a.navigate_book(BookNavigation::next_sibling);check(empty->tree.node.offset==12 && !empty->has_entry,"Entryless branch remains a valid selection");
  check(a.navigate_book(BookNavigation::first_child)->tree.node.offset==16,"Entryless branch child is reachable");
  check(!a.navigate_book(BookNavigation::next_sibling) && a.book_selection()->tree.node.offset==16,"Missing next sibling keeps source");
  (void)a.navigate_book(BookNavigation::parent);auto linked=a.navigate_book(BookNavigation::next_sibling);
  check(linked->tree.node.offset==20 && linked->html==reader.read_book("OSISBook",20,options).html,"Linked entry keeps alias source and configured rendering");
  check(a.navigate_book(BookNavigation::previous_sibling)->tree.node.offset==12,"Previous sibling includes entryless branch");
  check(a.navigate_book(BookNavigation::previous_sibling)->tree.node.offset==4,"SDK sibling order retained");
  rejects<std::invalid_argument>([&]{a.navigate_book(static_cast<BookNavigation>(-1));});
  check(a.book_selection()->tree.node.offset==4 && !b.book_selection(),"Invalid command and other tab remain neutral");
  check(a.history().entries().empty() && a.dictionary_history().entries().empty(),"Book commands do not create Bible/dictionary visits");
  (void)a.select_book({"OSISBook",std::string("/")});check(!a.navigate_book(BookNavigation::parent) && a.book_selection()->tree.node.offset==0,"Explicit path root has no parent and stays root");
  check(a.navigate_book(BookNavigation::first_child)->tree.node.offset==4,"Root first child reaches first reading node");
 }
 {
  StudyWorker worker(root);const auto a=worker.create_tab().get(),b=worker.create_tab().get();
  worker.set_engine_options(a,options).get();(void)worker.select_book(a,{"OSISBook",std::uint64_t(4)}).get();
  auto first=worker.navigate_book(a,BookNavigation::next_sibling);auto second=worker.navigate_book(a,BookNavigation::next_sibling);auto state=worker.book_selection(a);
  check(first.get()->tree.node.offset==12 && second.get()->tree.node.offset==20 && state.get()->node.offset==20,"FIFO commands navigate from the last committed source");
  check(!worker.navigate_book(b,BookNavigation::next_sibling).get(),"Unselected other worker tab is neutral");
  worker.close_tab(a).get();rejects<std::invalid_argument>([&]{(void)worker.navigate_book(a,BookNavigation::parent).get();});
 }
 xc_worker *worker=nullptr;xc_request *request=nullptr;
 check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open C worker");
 check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Create C tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_navigate_book(worker,tab,XC_BOOK_PARENT_DIRECTION,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==0,"C missing source optional absence");xc_request_release(request);
 request=reinterpret_cast<xc_request *>(1);check(xc_worker_navigate_book(worker,tab,99,&request)==XC_INVALID_ARGUMENT && !request,"C unknown direction resets output immediately");
 check(xc_worker_select_book(worker,tab,"OSISBook",4,&request)==XC_OK && wait(request)==XC_OK,"Select C source");xc_request_release(request);
 check(xc_worker_navigate_book(worker,tab,XC_BOOK_FIRST_CHILD_DIRECTION,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==1,"C navigation optional entry");
 auto *entry=xc_request_take_book_entry(request);check(entry && !xc_request_take_book_entry(request) && xc_request_book_selection_present(request)==1,"Take-once entry and stable presence");xc_request_release(request);
 check(xc_worker_navigate_book(worker,tab,XC_BOOK_PARENT_DIRECTION,&request)==XC_OK,"Queue parent and drop observer");xc_request_release(request);
 check(xc_worker_navigate_book(worker,tab,XC_BOOK_NEXT_SIBLING_DIRECTION,&request)==XC_OK,"Queue next behind dropped parent");xc_worker_close(worker);
 check(wait(request)==XC_OK,"Close drains accepted navigation");auto *drained=xc_request_take_book_entry(request);xc_request_release(request);xc_book_node node{};
 check(drained && xc_book_tree_get_node(xc_book_entry_tree(drained),XC_BOOK_CURRENT,&node)==XC_OK && node.offset==12,"FIFO command uses committed parent despite released observer");
 check(xc_book_tree_get_node(xc_book_entry_tree(entry),XC_BOOK_CURRENT,&node)==XC_OK && node.offset==8 && std::string(xc_book_entry_html(entry)).find("Original leaf")!=std::string::npos && owned.html.find("wordsOfJesus")!=std::string::npos,"Copied results survive subsequent commands and shutdown");
 xc_book_entry_release(drained);xc_book_entry_release(entry);
 std::cout<<"Shared book navigation, boundaries, source identities and FIFO C ownership passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
