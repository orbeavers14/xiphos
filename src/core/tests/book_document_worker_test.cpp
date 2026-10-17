// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected document worker failure");}
xc_status wait(xc_request *request){const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<deadline,"Document worker deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
void done(xc_request *request,xc_status expected=XC_OK){check(wait(request)==expected,"Expected document worker status");xc_request_release(request);}
int main(int argc,char **argv){try{
 check(argc==2,"Original document fixture required");const std::string root=argv[1];EngineOptions on,off;on.words_in_red=true;on.footnotes=true;BookDocument marked,plain;
 {
  Reader reader(root);marked=reader.read_book_document("Document5",4,on);plain=reader.read_book_document("Document5",4,off);
  check(marked.html!=plain.html && marked.sections.size()==5,"Real document filter output and descendants");
  StudySession a(reader),b(reader);a.set_engine_options(on);b.set_engine_options(off);
  (void)a.select_book({"Document5",std::uint64_t(8)});
  check(a.read_book_document("Document5",4).html==marked.html && b.read_book_document("Document5",4).html==plain.html,"Session document uses isolated tab options");
  a.set_module_engine_options("Document5",off);check(a.read_book_document("Document5",4).html==plain.html,"Module preference overrides tab document options");a.clear_module_engine_options("Document5");
  check(a.read_book_document("Document5",0).tree.node.offset==0 && a.book_selection()->tree.node.offset==8 && !b.book_selection(),"Raw root document is read-only and does not replace committed selection");
  rejects<std::out_of_range>([&]{a.read_book_document("Document5",40000);});check(a.book_selection()->tree.node.offset==8,"Failed document query preserves source");
  check(a.history().entries().empty() && a.dictionary_history().entries().empty(),"Read-only documents create no history visits");
 }
 std::future<BookDocument> drained;
 {
  StudyWorker worker(root);const auto a=worker.create_tab().get(),b=worker.create_tab().get();
  auto settings=worker.set_engine_options(a,on);auto red=worker.read_book_document(a,"Document5",4);auto replacement=worker.set_engine_options(a,off);auto black=worker.read_book_document(a,"Document5",4);
  check(red.get().html==marked.html && black.get().html==plain.html,"Queued document captures FIFO option policy");settings.get();replacement.get();
  check(!worker.book_selection(a).get() && !worker.book_selection(b).get(),"Worker document never publishes a selection");
  worker.close_tab(a).get();rejects<std::invalid_argument>([&]{(void)worker.read_book_document(a,"Document5",4).get();});
  drained=worker.read_book_document(b,"Document5",8);
 }
 check(drained.get().sections.size()==2,"Worker close drains a copied descendant document");
 xc_worker *worker=nullptr;xc_request *request=nullptr;
 check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open C document worker");
 check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Create C tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_set_engine_options(worker,tab,XC_ENGINE_FOOTNOTES|XC_ENGINE_WORDS_IN_RED,&request)==XC_OK,"Queue C document options");done(request);
 char id[]="Document5";
 check(xc_worker_read_book_document(worker,tab,id,4,&request)==XC_OK && !xc_request_take_book_document(request),"Pending document cannot be taken");id[0]='X';
 check(wait(request)==XC_OK && !xc_request_take_book_entry(request) && xc_request_book_selection_present(request)==-1,"Owned document is a distinct non-selection result");auto *document=xc_request_take_book_document(request);check(document && !xc_request_take_book_document(request),"C document take-once");xc_request_release(request);
 check(xc_book_document_display_level(document)==3 && xc_book_document_count(document)==marked.sections.size() && std::string(xc_book_document_html(document))==marked.html,"C document owns depth and combined configured HTML");
 xc_book_node node{};check(xc_book_tree_get_node(xc_book_document_tree(document),XC_BOOK_CURRENT,&node)==XC_OK && node.offset==4,"Borrowed selected source");
 for(std::size_t i=0;i<marked.sections.size();++i){xc_book_document_section part{};check(xc_book_document_get_section(document,i,&part)==XC_OK && part.depth==marked.sections[i].depth && bool(part.rendered)==marked.sections[i].rendered,"Borrowed section metadata");check(xc_book_tree_get_node(xc_book_entry_tree(part.entry),XC_BOOK_CURRENT,&node)==XC_OK && node.path==marked.sections[i].entry.tree.node.path && std::string(xc_book_entry_html(part.entry))==marked.sections[i].entry.html,"C document section retains source and configured HTML");}
 xc_book_document_section invalid{reinterpret_cast<const xc_book_entry *>(1),99,1};check(xc_book_document_get_section(document,999,&invalid)==XC_INVALID_ARGUMENT && !invalid.entry && !invalid.depth && !invalid.rendered,"Out-of-range section clears output");
 invalid={reinterpret_cast<const xc_book_entry *>(1),99,1};check(xc_book_document_get_section(nullptr,0,&invalid)==XC_INVALID_ARGUMENT && !invalid.entry && !invalid.depth && !invalid.rendered,"NULL document clears section");
 check(xc_book_document_get_section(document,0,nullptr)==XC_INVALID_ARGUMENT && !xc_book_document_tree(nullptr) && !xc_book_document_html(nullptr) && !xc_book_document_count(nullptr) && !xc_book_document_display_level(nullptr),"NULL getter contracts");xc_book_document_release(nullptr);
 request=reinterpret_cast<xc_request *>(1);check(xc_worker_read_book_document(worker,tab,nullptr,4,&request)==XC_INVALID_ARGUMENT && !request,"NULL module resets output");
 check(xc_worker_read_book_document(worker,tab,"",4,&request)==XC_INVALID_ARGUMENT && !request,"Empty module rejects immediately");
 for(const auto *module:{"Missing","VerseBook"}){check(xc_worker_read_book_document(worker,tab,module,4,&request)==XC_OK,"Queue bad document module");done(request,std::string(module)=="Missing"?XC_MODULE_NOT_FOUND:XC_INVALID_ARGUMENT);}
 check(xc_worker_read_book_document(worker,tab,"Document5",6,&request)==XC_OK,"Queue misaligned document source");done(request,XC_INVALID_ARGUMENT);
 check(xc_worker_read_book_document(worker,tab,"Document5",40000,&request)==XC_OK,"Queue missing document source");done(request,XC_MODULE_NOT_FOUND);
 check(xc_worker_book_selection(worker,tab,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==0,"Failed/successful document reads remain selection-neutral");xc_request_release(request);
 check(xc_worker_read_book_document(worker,tab,"Document5",8,&request)==XC_OK,"Queue document before close");xc_worker_close(worker);
 check(wait(request)==XC_OK,"C close drains accepted document");auto *last=xc_request_take_book_document(request);xc_request_release(request);
 check(last && xc_book_document_count(last)==2 && std::string(xc_book_document_html(document))==marked.html,"Owned documents outlive later work and worker close");
 xc_book_document_release(last);xc_book_document_release(document);
 std::cout<<"Configured document sessions, FIFO options, read-only state and owned C transport passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
