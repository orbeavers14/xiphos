// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected book session error");}
xc_status wait(xc_request *request){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Book publication deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
void done(xc_request *request,xc_status status=XC_OK){check(wait(request)==status,"Expected C publication status");xc_request_release(request);}
int main(int argc,char **argv){
 try{
  check(argc==2,"Generated book-entry fixture required");const std::string root=argv[1];EngineOptions off,on;on.footnotes=true;on.words_in_red=true;std::string plain,marked;BookSelectionState retained;
  {
   Reader reader(root);plain=reader.read_book("OSISBook",4,off).html;marked=reader.read_book("OSISBook",4,on).html;check(plain!=marked,"Real book option outputs differ");
   StudySession a(reader),b(reader);check(!a.book_selection() && !b.book_selection(),"New sessions have no selected book");a.set_engine_options(on);b.set_engine_options(off);
   auto entry=a.select_book({"OSISBook",std::string("Part One")});
   check(entry.html==marked && a.book_selection()->tree.node.offset==4 && std::get<std::string>(a.book_selection()->requested.address)=="Part One","Selection publishes configured source and retains typed request");
   b.select_book({"ThMLBook",std::uint64_t(20)});check(a.book_selection()->tree.module=="OSISBook" && b.book_selection()->tree.module=="ThMLBook","Shared Reader does not couple committed tab selections");
   a.read_book("OSISBook",8);a.book_note("OSISBook",20,"1");reader.resolve_book_target({"OSISBook",std::string("Linked")});
   check(a.book_selection()->tree.node.offset==4,"Read-only entry/note/resolution calls do not publish selection");
   rejects<std::out_of_range>([&]{a.select_book({"OSISBook",std::string("Missing/path")});});
   rejects<std::invalid_argument>([&]{a.select_book({"OSISBook",std::uint64_t(6)});});
   rejects<std::invalid_argument>([&]{a.select_book({"VerseBook",std::uint64_t(4)});});
   rejects<std::out_of_range>([&]{a.select_book({"Missing",std::uint64_t(4)});});
   rejects<std::invalid_argument>([&]{a.restore_book_selection({"OSISBook","4","-4"},BookRestoreMode::initial_window);});
   check(a.book_selection()->tree.node.offset==4 && a.book_selection()->tree.module=="OSISBook","Rejected selection/restore retains prior committed source");
   check(!a.restore_book_selection({},BookRestoreMode::display_key) && a.book_selection()->tree.node.offset==4,"No saved module is a successful no-op, not a clear");
   a.set_module_engine_options("OSISBook",off);check(a.select_book({"OSISBook",std::uint64_t(4)}).html==plain,"Publication honors module-over-tab options");a.clear_module_engine_options("OSISBook");
   check(a.restore_book_selection({"OSISBook","Part One","20"},BookRestoreMode::initial_window)->tree.node.path=="/Linked","Initial-window restore publishes offset source");
   check(a.restore_book_selection({"OSISBook","Part One","20"},BookRestoreMode::display_key)->tree.node.offset==4,"Display-key restore publishes path source");
   check(a.history().entries().empty() && a.dictionary_history().entries().empty(),"Book selection does not invent Bible/dictionary visits");
   retained=*a.book_selection();a.select_book({"OSISBook",std::uint64_t(8)});
  }
  check(retained.tree.node.path=="/Part One" && retained.tree.first_child->offset==8,"Copied committed state outlives later selection and Reader");
  std::future<std::optional<BookTree>> drained;
  {
   StudyWorker worker(root);const auto a=worker.create_tab().get(),b=worker.create_tab().get();check(!worker.book_selection(a).get(),"Worker new tab has no book");
   auto settings=worker.set_engine_options(a,on);auto selection=worker.select_book(a,{"OSISBook",std::uint64_t(4)});auto snapshot=worker.book_selection(a);
   check(selection.get().html==marked && snapshot.get()->node.offset==4,"FIFO publishes source after configured selection");settings.get();
   check(!worker.book_selection(b).get(),"Other tab remains unselected");
   auto bad=worker.select_book(a,{"OSISBook",std::string("Missing")});auto retained=worker.book_selection(a);rejects<std::out_of_range>([&]{(void)bad.get();});check(retained.get()->node.offset==4,"Queued failure cannot replace committed source");
   (void)worker.read_book(a,"OSISBook",8).get();check(worker.book_selection(a).get()->node.offset==4,"Queued read remains state-neutral");
   check(!worker.restore_book_selection(a,{},BookRestoreMode::display_key).get() && worker.book_selection(a).get()->node.offset==4,"Worker absent restore no-op");
   worker.close_tab(a).get();rejects<std::invalid_argument>([&]{(void)worker.book_selection(a).get();});rejects<std::invalid_argument>([&]{(void)worker.select_book(a,{"OSISBook",std::uint64_t(4)}).get();});
   (void)worker.restore_book_selection(b,{"OSISBook","Part One","20"},BookRestoreMode::initial_window).get();drained=worker.book_selection(b);
  }
  check(drained.get()->node.offset==20,"Worker close drains copied committed snapshot");
  xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open C publication worker");
  check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Create C book tab");const auto a=xc_request_tab(request);xc_request_release(request);
  check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Create other C tab");const auto b=xc_request_tab(request);xc_request_release(request);
  check(xc_worker_book_selection(worker,a,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==0 && !xc_request_take_book_tree(request),"C committed source initially absent");xc_request_release(request);
  check(xc_worker_set_engine_options(worker,a,XC_ENGINE_FOOTNOTES|XC_ENGINE_WORDS_IN_RED,&request)==XC_OK,"Queue C tab options");done(request);
  check(xc_worker_select_book(worker,a,"OSISBook",4,&request)==XC_OK && wait(request)==XC_OK,"Queue explicit C selection");auto *entry=xc_request_take_book_entry(request);check(entry && std::string(xc_book_entry_html(entry))==marked,"C published entry uses tab options");xc_request_release(request);
  check(xc_worker_book_selection(worker,b,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==0,"C other tab remains unselected");xc_request_release(request);
  check(xc_worker_select_book(worker,a,"OSISBook",40000,&request)==XC_OK,"Queue rejected C selection");done(request,XC_MODULE_NOT_FOUND);
  xc_saved_book_selection saved={nullptr,nullptr,nullptr};check(xc_worker_restore_book_selection(worker,a,&saved,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==0 && !xc_request_take_book_entry(request),"C absent restore is a successful no-op");xc_request_release(request);
  check(xc_worker_book_selection(worker,a,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==1,"Failed/absent C restore retains committed source");auto *old=xc_request_take_book_tree(request);xc_request_release(request);xc_book_node node{};check(xc_book_tree_get_node(old,XC_BOOK_CURRENT,&node)==XC_OK && node.offset==4,"Retained old committed source");
  request=reinterpret_cast<xc_request *>(1);check(xc_worker_restore_book_selection(worker,a,nullptr,0,&request)==XC_INVALID_ARGUMENT && !request,"NULL restore input resets output");
  check(xc_worker_restore_book_selection(worker,a,&saved,99,&request)==XC_INVALID_ARGUMENT && !request,"Unknown restore mode rejects immediately");
  check(xc_worker_select_book(worker,a,nullptr,4,&request)==XC_INVALID_ARGUMENT && !request,"NULL selection module rejects");
  char id[]="OSISBook",key[]="Part One",offset[]="20";saved={id,key,offset};
  check(xc_worker_restore_book_selection(worker,b,&saved,XC_BOOK_RESTORE_INITIAL_WINDOW,&request)==XC_OK,"Queue copied restore for other tab");id[0]='X';key[0]='X';offset[0]='9';saved={nullptr,nullptr,nullptr};
  check(wait(request)==XC_OK && xc_request_book_selection_present(request)==1 && !xc_request_take_book_tree(request),"Restore returns optional entry, not source-only kind");auto *restored=xc_request_take_book_entry(request);check(restored && !xc_request_take_book_entry(request) && xc_request_book_selection_present(request)==1,"Restore entry take-once/stable presence");xc_request_release(request);
  check(xc_book_tree_get_node(xc_book_entry_tree(restored),XC_BOOK_CURRENT,&node)==XC_OK && node.offset==20,"Copied initial context publishes alias node");xc_book_entry_release(restored);
  check(xc_worker_select_book(worker,a,"OSISBook",8,&request)==XC_OK,"Queue selection then drop observation");xc_request_release(request);
  check(xc_worker_book_selection(worker,a,&request)==XC_OK && wait(request)==XC_OK,"Observer release does not cancel accepted publication");auto *current=xc_request_take_book_tree(request);xc_request_release(request);check(xc_book_tree_get_node(current,XC_BOOK_CURRENT,&node)==XC_OK && node.offset==8,"Dropped observer still publishes FIFO selection");xc_book_tree_release(current);
  check(xc_worker_close_tab(worker,a,&request)==XC_OK,"Close selected C tab");done(request);
  check(xc_worker_book_selection(worker,a,&request)==XC_OK,"Queue closed tab snapshot");done(request,XC_INVALID_ARGUMENT);
  check(xc_worker_select_book(worker,a,"OSISBook",4,&request)==XC_OK,"Queue closed tab select");done(request,XC_INVALID_ARGUMENT);
  check(xc_worker_book_selection(worker,b,&request)==XC_OK,"Queue snapshot before C worker close");xc_worker_close(worker);
  check(wait(request)==XC_OK && xc_request_book_selection_present(request)==1,"Close drains committed book source");current=xc_request_take_book_tree(request);xc_request_release(request);
  check(xc_book_tree_get_node(current,XC_BOOK_CURRENT,&node)==XC_OK && node.offset==20 && std::string(xc_book_entry_html(entry))==marked && xc_book_tree_get_node(old,XC_BOOK_CURRENT,&node)==XC_OK && node.offset==4,"Owned entry and source snapshots outlive changes/tab/worker close");
  xc_book_tree_release(current);xc_book_tree_release(old);xc_book_entry_release(entry);
  std::cout<<"Per-tab book publication, option policy, failed-restore retention and queued C ownership passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
