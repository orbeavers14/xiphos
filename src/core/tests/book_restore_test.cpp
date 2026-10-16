// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "book_tree_source.hpp"
#include <swmgr.h>
#include <swmodule.h>
#include <treekeyidx.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <map>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected typed restore error");}
std::map<std::string,std::string> bytes(const fs::path &root){std::map<std::string,std::string> result;for(const auto &item:fs::recursive_directory_iterator(root))if(item.is_regular_file()){std::ifstream in(item.path(),std::ios::binary);check(bool(in),"Open module bytes");result.emplace(fs::relative(item.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(in),{}));}return result;}
xc_status wait(xc_request *request){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Restore request deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){
 try{
  check(argc==2,"Generated tree fixture required");const std::string root=argv[1];const auto before=bytes(fs::path(root)/"data");BookTree retained;
  {
   Reader reader(root);sword::SWMgr legacy(root.c_str(),true,nullptr,false,false);auto &module=*legacy.getModule("Book");
   auto &live=dynamic_cast<sword::TreeKeyIdx &>(*module.getKey());live.setOffset(12);const auto *identity=module.getKey();const auto attributes=module.getEntryAttributes();module.setProcessEntryAttributes(false);
   for(const auto &text:{"Part One","/Part One","/Part One/Section α","Part One/Section α","/123 Numeric Title","/",""}){
    SavedBookSelection saved{std::string("Book"),std::string(text),std::string("24")};const auto target=*saved_book_target(saved,BookRestoreMode::display_key);
    const auto result=reader.resolve_book_target(target);
    std::unique_ptr<sword::SWKey> own(module.createKey());auto &key=dynamic_cast<sword::TreeKeyIdx &>(*own);key.setText(text);const std::string path=key.getText();check(!key.popError(),"Frozen valid legacy path snap");
    check(result.node.offset==key.getOffset() && result.node.path==path && result.node.title==(key.getOffset()?key.getLocalName():"Book"),"Relative/root/Unicode source matches frozen SDK snap");
    const auto direct=detail::capture_book_display_path(module,text);check(direct.node.path==path && direct.node.offset==key.getOffset(),"Private source normalization matches SDK");
   }
   for(const auto &text:{"0","1","0012junk","24"}){
    const auto target=*saved_book_target({std::string("Book"),std::string(text),std::nullopt},BookRestoreMode::display_key);
    check(reader.resolve_book_target(target).node.offset==std::get<std::uint64_t>(target.address),"Defined numeric candidates resolve their exact validated node");
   }
   const SavedBookSelection saved{std::string("Book"),std::string("Part One"),std::string("24")};
   check(reader.resolve_book_target(*saved_book_target(saved,BookRestoreMode::display_key)).node.offset==4 && reader.resolve_book_target(*saved_book_target(saved,BookRestoreMode::initial_window)).node.offset==24,"Restore origin remains distinct through SDK resolution");
   rejects<std::out_of_range>([&]{reader.resolve_book_target({"Book",std::string("/Missing/path")});});
   rejects<std::out_of_range>([&]{reader.resolve_book_target({"Book",std::string("Missing/path")});});
   rejects<std::out_of_range>([&]{reader.resolve_book_target({"Book",std::uint64_t(40000)});});
   rejects<std::invalid_argument>([&]{reader.resolve_book_target({"Book",std::uint64_t(6)});});
   rejects<std::invalid_argument>([&]{reader.resolve_book_target({"VerseBook",std::string("/")});});
   rejects<std::invalid_argument>([&]{reader.resolve_book_target({"Dictionary",std::uint64_t(0)});});
   rejects<std::out_of_range>([&]{reader.resolve_book_target({"Missing",std::uint64_t(4)});});
   rejects<std::invalid_argument>([&]{reader.resolve_book_target({std::string("Book\0bad",8),std::uint64_t(4)});});
   rejects<std::invalid_argument>([&]{reader.resolve_book_target({"Book",std::string("Part One\0bad",12)});});
   rejects<std::invalid_argument>([&]{reader.resolve_book_target({"Book",std::string(4097,'x')});});
   rejects<std::invalid_argument>([&]{reader.book_tree_path("Book","Part One");});
   rejects<std::out_of_range>([&]{reader.book_tree_path("Book","/");});
   rejects<std::out_of_range>([&]{detail::capture_book_display_path(module,"/Missing/path");});
   check(module.getKey()==identity && live.getOffset()==12 && module.getEntryAttributes()==attributes && !module.isProcessEntryAttributes(),"Queries/failures leave live key, attributes and processing flag unchanged");
   retained=reader.resolve_book_target({"Book",std::string("Part One/Section α")});
  }
  check(retained.node.offset==8 && retained.node.path=="/Part One/Section α","Resolved sources own data beyond Reader lifetime");
  std::future<std::optional<BookTree>> drained;
  {
   StudyWorker worker(root);SavedBookSelection saved{std::string("Book"),std::string("Part One"),std::string("24")};
   auto first=worker.resolve_saved_book_selection(saved,BookRestoreMode::display_key);saved.key="Missing/path";auto bad=worker.resolve_saved_book_selection(saved,BookRestoreMode::display_key);saved={};
   check(first.get()->node.offset==4,"Worker copies raw selection before later mutation");rejects<std::out_of_range>([&]{(void)bad.get();});
   check(!worker.resolve_saved_book_selection({},BookRestoreMode::display_key).get(),"No saved module is successful absence");
   rejects<std::invalid_argument>([&]{(void)worker.resolve_saved_book_selection({},static_cast<BookRestoreMode>(99)).get();});
   drained=worker.resolve_saved_book_selection({std::string("Book"),std::string("Part One"),std::string("24")},BookRestoreMode::initial_window);
  }
  check(drained.get()->node.offset==24,"Worker close drains owned initial restore result");
  xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open C restore worker");
  xc_saved_book_selection saved={nullptr,nullptr,nullptr};
  request=reinterpret_cast<xc_request *>(1);check(xc_worker_resolve_saved_book_selection(worker,nullptr,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_INVALID_ARGUMENT && !request,"NULL input resets output");
  check(xc_worker_resolve_saved_book_selection(worker,&saved,99,&request)==XC_INVALID_ARGUMENT && !request,"Unknown mode rejects even without module");
  check(xc_worker_resolve_saved_book_selection(worker,&saved,XC_BOOK_RESTORE_DISPLAY_KEY,nullptr)==XC_INVALID_ARGUMENT,"NULL request output rejects");
  check(xc_worker_resolve_saved_book_selection(worker,&saved,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK && xc_request_book_selection_present(request)==-1 && wait(request)==XC_OK,"Queue/poll successful absence");
  check(xc_request_book_selection_present(request)==0 && !xc_request_take_book_tree(request),"No module is distinct from failed request/wrong kind");xc_request_release(request);
  char id[]="Book",key[]="Part One",offset[]="24";saved={id,key,offset};
  check(xc_worker_resolve_saved_book_selection(worker,&saved,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK,"Queue copied C restore strings");id[0]='X';key[0]='X';offset[0]='9';saved={nullptr,nullptr,nullptr};
  check(wait(request)==XC_OK && xc_request_book_selection_present(request)==1 && !xc_request_take_book_entry(request),"Resolved restore is a tree result");
  auto *tree=xc_request_take_book_tree(request);check(tree && !xc_request_take_book_tree(request) && xc_request_book_selection_present(request)==1,"Take once, stable result presence");xc_request_release(request);
  xc_book_node node{};check(xc_book_tree_get_node(tree,XC_BOOK_CURRENT,&node)==XC_OK && node.present && node.offset==4 && std::string(node.path)=="/Part One","C relative restore source");
  saved={"Book","/Missing/path","24"};check(xc_worker_resolve_saved_book_selection(worker,&saved,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK && wait(request)==XC_MODULE_NOT_FOUND && xc_request_book_selection_present(request)==-1,"Missing path cannot publish stale C target");xc_request_release(request);
  saved={"Book","6","24"};check(xc_worker_resolve_saved_book_selection(worker,&saved,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK && wait(request)==XC_INVALID_ARGUMENT,"Misaligned saved offset fails at poll");xc_request_release(request);
  saved={"Missing","0","0"};check(xc_worker_resolve_saved_book_selection(worker,&saved,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK && wait(request)==XC_MODULE_NOT_FOUND,"Missing module status");xc_request_release(request);
  saved={"VerseBook","0","0"};check(xc_worker_resolve_saved_book_selection(worker,&saved,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK && wait(request)==XC_INVALID_ARGUMENT,"Unsupported key type status");xc_request_release(request);
  saved={"Book","Part One","24"};check(xc_worker_resolve_saved_book_selection(worker,&saved,XC_BOOK_RESTORE_INITIAL_WINDOW,&request)==XC_OK,"Queue initial restore before close");xc_worker_close(worker);
  check(wait(request)==XC_OK && xc_request_book_selection_present(request)==1,"C close drains accepted restore");auto *initial=xc_request_take_book_tree(request);xc_request_release(request);
  check(xc_book_tree_get_node(initial,XC_BOOK_CURRENT,&node)==XC_OK && node.offset==24 && std::string(xc_book_tree_module(tree))=="Book","Owned results survive request and worker close");xc_book_tree_release(initial);xc_book_tree_release(tree);
  check(bytes(fs::path(root)/"data")==before,"All restore successes/failures preserve every module data byte and filename");
  std::cout<<"SDK restore normalization, exact origin, read-only state and queued C ownership checks passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
