// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected document publication failure");}
xc_status wait(xc_request *request){const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<deadline,"Document publication deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
void done(xc_request *request,xc_status expected=XC_OK){check(wait(request)==expected,"Expected document publication status");xc_request_release(request);}
uint64_t offset(const xc_book_tree *tree){xc_book_node node{};check(xc_book_tree_get_node(tree,XC_BOOK_CURRENT,&node)==XC_OK,"Copied source node");return node.offset;}
std::string bytes(const std::filesystem::path &path){std::ifstream file(path,std::ios::binary);return std::string(std::istreambuf_iterator<char>(file),{});}
int main(int argc,char **argv){try{
 check(argc==3,"Original document fixture and private XML path required");const std::string root=argv[1],file=argv[2];
 const std::string profile="<Xiphos><modules><book>Document5</book></modules><keys><book>/Branch</book><offset>8</offset></keys><unrelated>Original retained text</unrelated></Xiphos>";
 {std::ofstream out(std::filesystem::u8path(file),std::ios::binary);out<<profile;check(bool(out),"Create original profile fixture");}
 const auto permissions=std::filesystem::status(std::filesystem::u8path(file)).permissions();
 const std::string uri="passagestudy.jsp?action=showNote&type=n&value=1&module=Document5&passage=%2FBranch%2FLeaf";
 EngineOptions on;on.footnotes=true;on.words_in_red=true;std::future<std::optional<BookDocument>> drained;
 {
  StudyWorker worker(root);const auto a=worker.create_tab().get(),b=worker.create_tab().get();check(!worker.book_document_selection(a).get(),"Worker initially has no document");
  worker.set_engine_options(a,on).get();auto selected=worker.select_book_document(a,{"Document5",std::uint64_t(4)});auto next=worker.navigate_book_document(a,BookNavigation::first_child);auto snapshot=worker.book_document_selection(a);
  check(selected.get().sections.size()==5 && next.get()->tree.node.offset==8 && snapshot.get()->tree.node.offset==8,"FIFO document commands publish complete source before next navigation");
  check(!worker.book_document_selection(b).get(),"Other tab has no document");
  (void)worker.navigate_book_document(a,BookNavigation::parent).get();auto note=worker.book_document_note_link(a,uri).get();check(note.tree.node.offset==8 && worker.book_document_selection(a).get()->tree.node.offset==4,"Queued descendant note keeps committed root");
  auto bad=worker.select_book_document(a,{"TooDeep",std::uint64_t(4)});auto retained=worker.book_document_selection(a);rejects<std::runtime_error>([&]{(void)bad.get();});check(retained.get()->tree.node.offset==4,"Traversal failure retains previous complete document");
  check(!worker.restore_book_document_selection(a,{},BookRestoreMode::display_key).get() && worker.book_document_selection(a).get()->tree.node.offset==4,"Absent queued restore is a no-op");
  check(worker.restore_book_document_profile(a,file,BookRestoreMode::initial_window).get()->tree.node.offset==8 && worker.restore_book_document_profile(a,file,BookRestoreMode::display_key).get()->tree.node.offset==4,"Explicit document file restore preserves distinct contexts");
  worker.close_tab(a).get();rejects<std::invalid_argument>([&]{(void)worker.restore_book_document_profile(a,file+".missing",BookRestoreMode::display_key).get();});
  (void)worker.select_book_document(b,{"Document5",std::uint64_t(4)}).get();drained=worker.book_document_selection(b);
 }
 check(drained.get()->sections.size()==5,"Worker close drains owned committed document snapshot");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open C publication worker");
 check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Create C tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_book_document_selection(worker,tab,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==0,"C initial optional document absence");xc_request_release(request);
 check(xc_worker_select_book_document(worker,tab,"Document5",0,&request)==XC_OK && wait(request)==XC_OK,"Select C document through shared numeric reading rule");auto *first=xc_request_take_book_document(request);xc_request_release(request);check(first && offset(xc_book_document_tree(first))==4 && xc_book_document_count(first)==5,"C complete document selection");
 check(xc_worker_select_book_document(worker,tab,"TooDeep",4,&request)==XC_OK,"Queue bounded failing selection");done(request,XC_ENGINE_ERROR);
 check(xc_worker_book_document_selection(worker,tab,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==1,"Failure retains C committed document");auto *snapshot=xc_request_take_book_document(request);check(snapshot && !xc_request_take_book_document(request) && xc_request_book_selection_present(request)==1,"Optional document take-once/stable presence");xc_request_release(request);
 check(xc_worker_book_document_note_link(worker,tab,uri.c_str(),1,&request)==XC_OK && wait(request)==XC_OK,"Queue exact descendant note");auto *note=xc_request_take_book_note(request);xc_request_release(request);check(note && offset(xc_book_note_tree(note))==8 && std::string(xc_note_html(xc_book_note_content(note))).find("Original descendant note")!=std::string::npos,"C note owns descendant source/body");
 check(xc_worker_follow_book_link(worker,tab,uri.c_str(),&request)==XC_OK&&wait(request)==XC_OK&&!xc_request_take_book_image_data(request),"Core click dispatch retains exact document note route");auto *followed_note=xc_request_take_book_note(request);xc_request_release(request);check(followed_note&&offset(xc_book_note_tree(followed_note))==8,"Dispatched note retains descendant source");xc_book_note_release(followed_note);
 const std::string outside="passagestudy.jsp?action=showNote&type=n&value=1&module=Document5&passage=%2FLinked";
 check(xc_worker_book_document_note_link(worker,tab,outside.c_str(),1,&request)==XC_OK,"Queue outside-document note");done(request,XC_INVALID_ARGUMENT);
 request=reinterpret_cast<xc_request *>(1);check(xc_worker_navigate_book_document(worker,tab,99,&request)==XC_INVALID_ARGUMENT && !request,"Unknown document direction rejects and clears output");
 check(xc_worker_book_document_note_link(worker,tab,uri.c_str(),2,&request)==XC_INVALID_ARGUMENT && !request,"Strict note body flag");
 check(xc_worker_restore_book_document_selection(worker,tab,nullptr,0,&request)==XC_INVALID_ARGUMENT && !request,"NULL saved document rejects");
 xc_saved_book_selection absent={nullptr,nullptr,nullptr};check(xc_worker_restore_book_document_selection(worker,tab,&absent,99,&request)==XC_INVALID_ARGUMENT && !request,"Unknown restore mode rejects");
 check(xc_worker_restore_book_document_selection(worker,tab,&absent,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==0 && !xc_request_take_book_document(request),"No saved document is successful absence");xc_request_release(request);
 std::string copied_path=file;check(xc_worker_restore_book_document_profile(worker,tab,copied_path.c_str(),XC_BOOK_RESTORE_INITIAL_WINDOW,&request)==XC_OK,"Queue copied explicit profile path");copied_path.assign("unrelated");check(wait(request)==XC_OK,"Copied path resolves after caller mutation");auto *imported=xc_request_take_book_document(request);xc_request_release(request);check(imported && offset(xc_book_document_tree(imported))==8,"Initial profile restore publishes saved offset document");xc_book_document_release(imported);
 check(xc_worker_restore_book_document_profile(worker,tab,"",0,&request)==XC_INVALID_ARGUMENT && !request,"Empty file path rejects");
 char module[]="Document5",key[]="/Branch",saved_offset[]="8";xc_saved_book_selection saved={module,key,saved_offset};
 check(xc_worker_restore_book_document_selection(worker,tab,&saved,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK,"Queue copied saved values");module[0]='X';key[0]='X';saved_offset[0]='0';check(wait(request)==XC_OK,"Saved values copied before returning");imported=xc_request_take_book_document(request);xc_request_release(request);check(imported && offset(xc_book_document_tree(imported))==4,"Display-key context publishes branch document");xc_book_document_release(imported);
 check(xc_worker_navigate_book_document(worker,tab,XC_BOOK_PREVIOUS_SIBLING_DIRECTION,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==0,"Missing document relation is a no-op");xc_request_release(request);
 check(xc_worker_select_book(worker,tab,"Document5",4,&request)==XC_OK,"Legacy single-entry selection remains available");done(request);
 check(xc_worker_book_document_selection(worker,tab,&request)==XC_OK && wait(request)==XC_OK && xc_request_book_selection_present(request)==0,"Single-entry selection clears old document context");xc_request_release(request);
 check(xc_worker_book_document_note_link(worker,tab,uri.c_str(),1,&request)==XC_OK,"Queue note without document");done(request,XC_INVALID_ARGUMENT);
 check(xc_worker_select_book_document(worker,tab,"Document5",4,&request)==XC_OK,"Queue document and release observer");xc_request_release(request);
 check(xc_worker_book_document_selection(worker,tab,&request)==XC_OK,"Queue final owned snapshot");xc_worker_close(worker);check(wait(request)==XC_OK,"Close drains accepted document publication");auto *last=xc_request_take_book_document(request);xc_request_release(request);
 check(last && xc_book_document_count(last)==5 && xc_book_document_count(first)==5 && offset(xc_book_document_tree(snapshot))==4 && offset(xc_book_note_tree(note))==8,"All copied roots/sections/notes survive selection changes and shutdown");
 check(bytes(std::filesystem::u8path(file))==profile && std::filesystem::status(std::filesystem::u8path(file)).permissions()==permissions,"Document import leaves profile bytes and permissions unchanged");
 xc_book_document_release(first);xc_book_document_release(snapshot);xc_book_document_release(last);xc_book_note_release(note);
 std::cout<<"Queued atomic document publication, profile restore, rendered-section notes and owned C snapshots passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
