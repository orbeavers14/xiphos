// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected profile-restore error");}
void write(const fs::path &path,const std::string &text){std::ofstream out(path,std::ios::binary);out.write(text.data(),static_cast<std::streamsize>(text.size()));check(bool(out),"Write original private fixture");}
std::string bytes(const fs::path &path){std::ifstream in(path,std::ios::binary);check(bool(in),"Read original profile bytes");return std::string(std::istreambuf_iterator<char>(in),{});}
xc_status wait(xc_request *request){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Profile restore deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){
 try{
  check(argc==2,"Original book-entry fixture required");const fs::path root(argv[1]);const auto file=root/fs::u8path("book-restore-café.xml");
  std::string encoded="\xff\xfe";
  for(char16_t c:std::u16string(u"<?xml version='1.0' encoding='UTF-16'?><Xiphos><modules><book>OSISBook</book></modules><keys><book>Part One</book><offset>20</offset></keys><locale><special>unapplied-original-locale</special></locale><unrelated>Original preserved profile data</unrelated></Xiphos>")){encoded.push_back(static_cast<char>(c&255));encoded.push_back(static_cast<char>(c>>8));}
  write(file,encoded);const auto permissions=fs::status(file).permissions();
  const auto absent=root/"book-absent.xml",missing=root/"book-missing-target.xml",bad=root/"book-malformed.xml",dtd=root/"book-dtd.xml",overflow=root/"book-overflow.xml";
  write(absent,"<Xiphos><locale><special>unapplied</special></locale></Xiphos>");
  write(missing,"<Xiphos><modules><book>OSISBook</book></modules><keys><book>/Missing/path</book></keys></Xiphos>");
  write(bad,"<Xiphos>");write(dtd,"<!DOCTYPE Xiphos SYSTEM 'https://example.test/profile'><Xiphos/>");
  write(overflow,"<Xiphos><modules><book>OSISBook</book></modules><keys><offset>999999999999999999999999999999999999</offset></keys></Xiphos>");
  EngineOptions off,on;on.footnotes=true;on.words_in_red=true;std::string expected;
  {Reader reader(root.u8string());expected=reader.read_book("OSISBook",4,on).html;check(expected!=reader.read_book("OSISBook",4,off).html,"Real configured output differs");}
  std::future<std::optional<BookEntry>> drained;
  {
   StudyWorker worker(root.u8string());const auto a=worker.create_tab().get(),b=worker.create_tab().get();worker.set_engine_options(a,on).get();
   auto imported=worker.restore_book_profile(a,file.u8string(),BookRestoreMode::display_key).get();
   check(imported && imported->tree.node.offset==4 && imported->html==expected,"Explicit UTF-16/Unicode-path import publishes configured book");
   check(worker.module_engine_options(a,"OSISBook").get()->words_in_red,"File restore retains existing engine preferences");
   check(!worker.book_selection(b).get(),"Other tab is unaffected by profile restore");
   rejects<std::invalid_argument>([&]{(void)worker.restore_book_profile(a,bad.u8string(),BookRestoreMode::display_key).get();});
   rejects<std::invalid_argument>([&]{(void)worker.restore_book_profile(a,dtd.u8string(),BookRestoreMode::display_key).get();});
   rejects<std::out_of_range>([&]{(void)worker.restore_book_profile(a,missing.u8string(),BookRestoreMode::display_key).get();});
   rejects<std::invalid_argument>([&]{(void)worker.restore_book_profile(a,overflow.u8string(),BookRestoreMode::initial_window).get();});
   for(const auto &path:{root.u8string(),(root/"not-present.xml").u8string(),std::string(),std::string("bad\0path",8)})rejects<std::invalid_argument>([&]{(void)worker.restore_book_profile(a,path,BookRestoreMode::display_key).get();});
   rejects<std::invalid_argument>([&]{(void)worker.restore_book_profile(a,file.u8string(),static_cast<BookRestoreMode>(99)).get();});
   check(worker.book_selection(a).get()->node.offset==4,"Read/parse/target failures retain previous committed source");
   check(!worker.restore_book_profile(a,absent.u8string(),BookRestoreMode::display_key).get() && worker.book_selection(a).get()->node.offset==4,"Absent module import is successful no-op");
   check(worker.history(a).get().entries.empty() && worker.dictionary_history(a).get().entries.empty(),"File import does not alter Bible/dictionary history");
   worker.close_tab(b).get();rejects<std::invalid_argument>([&]{(void)worker.restore_book_profile(b,file.u8string(),BookRestoreMode::display_key).get();});
   drained=worker.restore_book_profile(a,file.u8string(),BookRestoreMode::initial_window);
  }
  check(drained.get()->tree.node.offset==20,"Worker close drains initial-window profile restore");
  xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.u8string().c_str(),&worker)==XC_OK,"Open C file-restore worker");
  check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Create C file-restore tab");const auto tab=xc_request_tab(request);xc_request_release(request);
  check(xc_worker_set_engine_options(worker,tab,XC_ENGINE_FOOTNOTES|XC_ENGINE_WORDS_IN_RED,&request)==XC_OK && wait(request)==XC_OK,"Configure C file restore");xc_request_release(request);
  request=reinterpret_cast<xc_request *>(1);check(xc_worker_restore_book_profile(worker,tab,nullptr,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_INVALID_ARGUMENT && !request,"NULL profile path resets output");
  check(xc_worker_restore_book_profile(worker,tab,"",XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_INVALID_ARGUMENT && !request,"Empty explicit path rejects");
  check(xc_worker_restore_book_profile(worker,tab,file.u8string().c_str(),99,&request)==XC_INVALID_ARGUMENT && !request,"Unknown mode rejects before queueing");
  const auto path=file.u8string();std::vector<char> copied(path.begin(),path.end());copied.push_back(0);
  check(xc_worker_restore_book_profile(worker,tab,copied.data(),XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK,"Queue copied profile path");copied.assign(copied.size(),'x');
  check(xc_request_book_selection_present(request)==-1 && wait(request)==XC_OK && xc_request_book_selection_present(request)==1 && !xc_request_take_book_tree(request),"Profile result is optional entry kind");
  auto *entry=xc_request_take_book_entry(request);check(entry && !xc_request_take_book_entry(request),"Profile entry take once");xc_request_release(request);
  check(std::string(xc_book_entry_html(entry))==expected,"C file restore uses existing configured HTML");
  check(xc_worker_restore_book_profile(worker,tab,bad.u8string().c_str(),XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK && wait(request)==XC_INVALID_ARGUMENT && xc_request_book_selection_present(request)==-1,"C malformed input fails without result");xc_request_release(request);
  check(xc_worker_restore_book_profile(worker,tab,missing.u8string().c_str(),XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK && wait(request)==XC_MODULE_NOT_FOUND,"C missing target status");xc_request_release(request);
  check(xc_worker_restore_book_profile(worker,tab,absent.u8string().c_str(),XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==0,"C absent book is successful no-op");xc_request_release(request);
  check(xc_worker_book_selection(worker,tab,&request)==XC_OK && wait(request)==XC_OK,"Read retained state after failed/absent file import");auto *retained=xc_request_take_book_tree(request);xc_request_release(request);xc_book_node node{};check(xc_book_tree_get_node(retained,XC_BOOK_CURRENT,&node)==XC_OK && node.offset==4,"File errors retain committed C source");
  check(xc_worker_restore_book_profile(worker,tab,file.u8string().c_str(),XC_BOOK_RESTORE_INITIAL_WINDOW,&request)==XC_OK,"Queue C file restore before close");xc_worker_close(worker);
  check(wait(request)==XC_OK && xc_request_book_selection_present(request)==1,"C close drains profile read/publication");auto *initial=xc_request_take_book_entry(request);xc_request_release(request);
  check(xc_book_tree_get_node(xc_book_entry_tree(initial),XC_BOOK_CURRENT,&node)==XC_OK && node.offset==20 && std::string(xc_book_entry_html(entry))==expected,"File-import snapshots outlive subsequent restore and worker close");xc_book_entry_release(initial);xc_book_entry_release(entry);xc_book_tree_release(retained);
  check(bytes(file)==encoded && fs::status(file).permissions()==permissions && bytes(absent)=="<Xiphos><locale><special>unapplied</special></locale></Xiphos>" && bytes(bad)=="<Xiphos>","Successful and failed imports preserve source file bytes/permissions");
  std::cout<<"Queued explicit book profile restore, failure retention and read-only source checks passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
