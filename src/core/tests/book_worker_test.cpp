// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
template<class Error,class F> void rejects(F call) {
  try { call(); } catch (const Error &) { return; }
  throw std::runtime_error("Expected queued book error");
}
xc_status wait(xc_request *request) {
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);
  xc_status status;
  while ((status=xc_request_poll(request))==XC_PENDING) {
    check(std::chrono::steady_clock::now()<end,"Book request deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return status;
}
void done(xc_request *request,xc_status expected=XC_OK) {
  check(wait(request)==expected,"Expected queued C book status"); xc_request_release(request);
}
std::map<std::string,std::string> bytes(const fs::path &root) {
  std::map<std::string,std::string> result;
  for (const auto &item:fs::recursive_directory_iterator(root)) if (item.is_regular_file()) {
    std::ifstream input(item.path(),std::ios::binary); check(bool(input),"Read module bytes");
    result.emplace(fs::relative(item.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(input),{}));
  }
  return result;
}
int main(int argc,char **argv) {
  try {
    check(argc==2,"Generated book-entry fixture required"); const std::string root=argv[1];
    const auto before=bytes(fs::path(root)/"data");
    EngineOptions off,on; on.footnotes=true; on.words_in_red=true;
    std::string plain,marked,unconfigured;
    {
      Reader reader(root);
      unconfigured=reader.read_book("OSISBook",4).html;
      plain=reader.read_book("OSISBook",4,off).html; marked=reader.read_book("OSISBook",4,on).html;
      check(plain!=marked,"Real option fixtures differ");
    }
    std::future<BookTree> drained_tree; std::future<BookEntry> drained_entry;
    {
      StudyWorker worker(root); const auto a=worker.create_tab().get(), b=worker.create_tab().get();
      check(worker.read_book(a,"OSISBook",4).get().html==unconfigured,"Unconfigured tab keeps Reader baseline");
      auto settings=worker.set_engine_options(a,on);
      auto first=worker.read_book(a,"OSISBook",4);
      auto other_settings=worker.set_engine_options(b,off);
      auto other=worker.read_book(b,"OSISBook",4);
      check(first.get().html==marked && other.get().html==plain,"FIFO copied settings and tab isolation");
      settings.get(); other_settings.get();
      worker.set_module_engine_options(a,"OSISBook",off).get();
      check(worker.read_book(a,"OSISBook",4).get().html==plain,"Existing module override wins");
      worker.clear_module_engine_options(a,"OSISBook").get();
      check(worker.read_book(a,"OSISBook",4).get().html==marked,"Clear module override reveals tab options");
      worker.clear_engine_options(a).get();
      check(worker.read_book(a,"OSISBook",4).get().html==unconfigured,"Clear tab options restores unconfigured read");
      worker.enable_module_preferences(a).get();
      check(worker.module_engine_options(a,"OSISBook").get().has_value(),"Existing legacy default policy applies to book module");
      check(worker.read_book(a,"OSISBook",8).get().html=="Original leaf prose.","Enabled legacy defaults can read books");
      auto bad=worker.book_tree_path("OSISBook","/Missing");
      auto good=worker.book_tree_path("OSISBook","/Linked");
      rejects<std::out_of_range>([&]{bad.get();}); check(good.get().node.offset==20,"Failed source query does not stop FIFO");
      check(worker.history(a).get().entries.empty() && worker.dictionary_history(a).get().entries.empty(),"Book queries/read have no Bible/dictionary history side effects");
      worker.close_tab(a).get();
      rejects<std::invalid_argument>([&]{worker.read_book(a,"OSISBook",4).get();});
      check(worker.book_tree("OSISBook",0).get().node.title=="OSISBook","Source queries do not require an open tab");
      drained_tree=worker.book_tree("OSISBook",4); drained_entry=worker.read_book(b,"OSISBook",20);
    }
    check(drained_tree.get().first_child->offset==8 && drained_entry.get().tree.node.path=="/Linked","Worker close drains and copied results outlive engine");
    {
      StudyWorker failed((fs::path(root)/"missing-root").string());
      rejects<std::invalid_argument>([&]{failed.book_tree("OSISBook",0).get();});
      rejects<std::invalid_argument>([&]{failed.read_book(1,"OSISBook",4).get();});
    }
    xc_worker *worker=nullptr; check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open C worker");
    xc_request *request=nullptr; check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Create C book tab");
    const auto tab=xc_request_tab(request); xc_request_release(request);
    xc_request *invalid=reinterpret_cast<xc_request *>(1);
    check(xc_worker_book_tree(worker,nullptr,0,&invalid)==XC_INVALID_ARGUMENT && !invalid,"NULL module resets output");
    invalid=reinterpret_cast<xc_request *>(1);
    check(xc_worker_book_tree_path(worker,"OSISBook",nullptr,&invalid)==XC_INVALID_ARGUMENT && !invalid,"NULL path resets output");
    check(xc_worker_book_tree(worker,"OSISBook",0,nullptr)==XC_INVALID_ARGUMENT,"NULL request output rejected");
    check(xc_worker_read_book(worker,tab,"",4,&invalid)==XC_INVALID_ARGUMENT && !invalid,"Empty read module rejects");
    char module[]="OSISBook", path[]="/Part One";
    check(xc_worker_book_tree_path(worker,module,path,&request)==XC_OK,"Queue copied book source strings");
    std::memset(module,'x',sizeof(module)-1); std::memset(path,'x',sizeof(path)-1);
    check(!xc_request_take_book_tree(request) && !xc_request_take_book_entry(request),"Take before poll is empty");
    check(wait(request)==XC_OK && !xc_request_take_book_entry(request),"Wrong result kind cannot consume tree");
    auto *tree=xc_request_take_book_tree(request); check(tree && !xc_request_take_book_tree(request),"Take owned tree exactly once");
    xc_request_release(request);
    xc_book_node node{};
    check(std::string(xc_book_tree_module(tree))=="OSISBook" && xc_book_tree_get_node(tree,XC_BOOK_CURRENT,&node)==XC_OK && node.present && node.offset==4 && std::string(node.path)=="/Part One" && node.has_children,"C copied tree source");
    check(xc_book_tree_get_node(tree,XC_BOOK_PARENT,&node)==XC_OK && node.present && !node.offset && std::string(node.title)=="OSISBook","Present root differs from absent relationship");
    check(xc_book_tree_get_node(tree,XC_BOOK_PREVIOUS_SIBLING,&node)==XC_OK && !node.present && !node.path && !node.title && !node.offset && !node.has_children,"Absent relationship clears all fields");
    check(xc_book_tree_get_node(tree,XC_BOOK_FIRST_CHILD,&node)==XC_OK && node.present && node.offset==8,"C first child");
    check(xc_book_tree_get_node(tree,XC_BOOK_NEXT_SIBLING,&node)==XC_OK && node.present && node.offset==12,"C next sibling");
    check(xc_book_tree_get_node(tree,-123,&node)==XC_INVALID_ARGUMENT && !node.present && !node.path,"Unknown node kind clears stale output");
    check(xc_book_tree_get_node(tree,XC_BOOK_CURRENT,nullptr)==XC_INVALID_ARGUMENT,"NULL node output rejected");
    check(xc_worker_set_engine_options(worker,tab,XC_ENGINE_FOOTNOTES|XC_ENGINE_WORDS_IN_RED,&request)==XC_OK,"Queue C settings"); done(request);
    check(xc_worker_read_book(worker,tab,"OSISBook",4,&request)==XC_OK,"Queue configured C book read");
    check(wait(request)==XC_OK && !xc_request_take_book_tree(request),"Entry result has distinct ownership kind");
    auto *entry=xc_request_take_book_entry(request); check(entry && !xc_request_take_book_entry(request),"Take entry once"); xc_request_release(request);
    check(xc_book_entry_has_entry(entry) && std::string(xc_book_entry_html(entry))==marked,"C entry exact configured HTML");
    const auto *borrowed=xc_book_entry_tree(entry);
    check(xc_book_tree_get_node(borrowed,XC_BOOK_CURRENT,&node)==XC_OK && node.present && node.offset==4,"Entry-owned borrowed tree");
    check(xc_worker_book_tree_path(worker,"OSISBook","/Missing",&request)==XC_OK,"Queue missing path"); done(request,XC_MODULE_NOT_FOUND);
    check(xc_worker_book_tree_path(worker,"OSISBook","relative",&request)==XC_OK,"Queue noncanonical path"); done(request,XC_INVALID_ARGUMENT);
    check(xc_worker_read_book(worker,tab,"OSISBook",40000,&request)==XC_OK,"Queue missing node"); done(request,XC_MODULE_NOT_FOUND);
    check(xc_worker_read_book(worker,tab,"OSISBook",1,&request)==XC_OK,"Queue invalid offset"); done(request,XC_INVALID_ARGUMENT);
    check(xc_worker_read_book(worker,tab,"Dictionary",0,&request)==XC_OK,"Queue wrong module type"); done(request,XC_INVALID_ARGUMENT);
    check(xc_worker_read_book(worker,tab,"Missing",0,&request)==XC_OK,"Queue missing module"); done(request,XC_MODULE_NOT_FOUND);
    check(xc_worker_book_tree_path(worker,"OSISBook","",&request)==XC_OK,"Queue actual root path");
    check(wait(request)==XC_OK,"Root path query succeeds"); auto *root_tree=xc_request_take_book_tree(request); xc_request_release(request);
    check(xc_book_tree_get_node(root_tree,XC_BOOK_CURRENT,&node)==XC_OK && node.present && !node.offset && !*node.path,"Empty path is actual root"); xc_book_tree_release(root_tree);
    check(xc_worker_close_tab(worker,tab,&request)==XC_OK,"Queue close before read"); xc_request_release(request);
    check(xc_worker_read_book(worker,tab,"OSISBook",4,&request)==XC_OK,"Queue closed tab read"); done(request,XC_INVALID_ARGUMENT);
    check(xc_worker_book_tree(worker,"OSISBook",8,&request)==XC_OK,"Queue source before worker close");
    xc_worker_close(worker);
    check(wait(request)==XC_OK,"Close drains pending C book work"); root_tree=xc_request_take_book_tree(request); xc_request_release(request);
    check(std::string(xc_book_tree_module(tree))=="OSISBook" && std::string(xc_book_entry_html(entry))==marked && xc_book_tree_get_node(borrowed,XC_BOOK_FIRST_CHILD,&node)==XC_OK && node.offset==8,"Owned and borrowed book snapshots outlive request and worker");
    xc_book_tree_release(tree); xc_book_tree_release(root_tree); xc_book_entry_release(entry);
    xc_book_tree_release(nullptr); xc_book_entry_release(nullptr);
    check(bytes(fs::path(root)/"data")==before,"Queued successes/failures preserve all module data bytes and filenames");
    std::cout << "Book FIFO, tab policy, C snapshot ownership/error and shutdown checks passed\n";
    return 0;
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
