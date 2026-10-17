// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <rawgenbook.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <iterator>
#include <chrono>
#include <thread>
#include <stdexcept>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
xc_status wait(xc_request *request){const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<deadline,"Presentation deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
void done(xc_request *request){check(wait(request)==XC_OK,"Complete presentation setting");xc_request_release(request);}
std::map<std::string,std::string> bytes(const fs::path &path){std::map<std::string,std::string> result;for(const auto &file:fs::recursive_directory_iterator(path))if(file.is_regular_file()){std::ifstream in(file.path(),std::ios::binary);result.emplace(fs::relative(file.path(),path).generic_string(),std::string(std::istreambuf_iterator<char>(in),{}));}return result;}
int main(int argc,char **argv){try{
 check(argc==2,"Original presentation fixture required");const fs::path root(argv[1]);fs::create_directories(root/"mods.d");fs::create_directories(root/"data");
 const auto prefix=(root/"data/book").string();check(sword::RawGenBook::createModule(prefix.c_str())==0,"Create original styled book");
 const std::string annotation="<sync type=\"Strongs\" value=\"G3056\"/>";
 const std::string styled="<font face=\"Original Book Font\">Original</font> styled word "+annotation+" <img src=\"original.png\" />";
 {sword::RawGenBook writer(prefix.c_str());writer.setKey("/Styled");writer.setEntry(styled.c_str());writer.setKey("/Styled/Child");writer.setEntry(("Original café descendant "+annotation).c_str());writer.setKey("/Fallback");writer.setEntry(styled.c_str());writer.setKey("/Fallback/Child");writer.setEntry("Original unsupported <small>ordinary note</small>");writer.setKey("/Malformed");writer.setEntry("Original malformed text <font face=\"unfinished>");}
 {std::ofstream config(root/"mods.d/book.conf");config<<"[StyledBook]\nDataPath=./data/book\nModDrv=RawGenBook\nEncoding=UTF-8\nSourceType=ThML\nDisplayLevel=2\nGlobalOptionFilter=ThMLStrongs\n";}
 fs::remove(root/"mods.d/modules-conf.cache");const auto before=bytes(root/"data");EngineOptions on;on.strongs=true;BookDocument held;
 {
  Reader reader(root.string());const auto target=reader.book_tree_path("StyledBook","/Styled");const auto raw=reader.read_book_document("StyledBook",target.node.offset,on);
  check(!raw.presentation && raw.sections.size()==2 && raw.html.find("<img ")!=std::string::npos && raw.html.find("Original Book Font")!=std::string::npos,"Raw configured source remains available without presentation");
  StudySession first(reader),other(reader);first.set_engine_options(on);other.set_engine_options(on);first.set_lexical_layout_enabled(true);
  check(!first.refresh_book_document(),"No committed document refresh is a no-op");
  auto discovered=first.select_book_document({"StyledBook",std::string("/Styled")});
  check(discovered.presentation && discovered.presentation->image_content==ContentPreference::on && discovered.presentation->respect_font_faces==ContentPreference::on && first.module_presentation_options("StyledBook").image_content==ContentPreference::on,"Selection atomically publishes matching discovery state");
  check(discovered.presentation->inline_images && !discovered.presentation->inline_images->embedded && discovered.presentation->inline_images->issues.size()==1 && discovered.presentation->inline_images->html==discovered.presentation->html,"Unavailable image retains exact visible HTML and reports issue without failing selection");
  check(other.module_presentation_options("StyledBook").image_content==ContentPreference::unknown,"Discovery is tab-local");
  first.set_image_content("StyledBook",false);first.set_respect_font_faces("StyledBook",false);first.set_double_space("StyledBook",true);held=*first.refresh_book_document();
  check(held.html==raw.html && held.sections[0].entry.html==raw.sections[0].entry.html && held.tree.node.path=="/Styled","Presentation preserves raw combined/section source and selected path");
  check(held.presentation->lexical.requested && held.presentation->lexical.applied && held.presentation->lexical.css.find("line-height: 2.7em")!=std::string::npos && held.presentation->html.find("class=\"word\"")!=std::string::npos && held.presentation->html.find("<img ")==std::string::npos && held.presentation->html.find("Original Book Font")==std::string::npos && held.presentation->html.find("café")!=std::string::npos,"Combined annotation layout precedes shared image/font cleanup with matching CSS");
  check(held.presentation->image_content==ContentPreference::off && held.presentation->respect_font_faces==ContentPreference::off && held.presentation->double_space,"Snapshot owns matching applied content preferences");
  auto fallback=first.select_book_document({"StyledBook",std::string("/Fallback")});
  check(fallback.presentation->lexical.requested && !fallback.presentation->lexical.applied && fallback.presentation->lexical.css.find("line-height: 2em")!=std::string::npos && fallback.presentation->html.find("class=\"word\"")==std::string::npos && fallback.presentation->html.find("ordinary note")!=std::string::npos,"Unsupported combined annotation grammar retains whole-document fallback and spacing-only CSS");
  auto malformed=first.select_book_document({"StyledBook",std::string("/Malformed")});
  check(malformed.presentation->cleanup_diagnostics.size()==1 && malformed.presentation->cleanup_diagnostics[0].kind==ContentCleanupKind::font_face && malformed.presentation->cleanup_diagnostics[0].suffix.find("unfinished")!=std::string::npos,"Owned document retains malformed cleanup diagnostic");
  check(held.presentation->inline_images && !held.presentation->inline_images->embedded && held.presentation->inline_images->issues.empty(),"Image-off cleanup precedes resource loading");
  auto plain=other.select_book_document({"StyledBook",std::string("/Styled")});check(plain.presentation->html==raw.html && !plain.presentation->lexical.requested && plain.presentation->lexical.css.empty(),"Unopted consumer preserves raw rendering and has no annotation CSS");
  (void)first.select_book_document({"StyledBook",std::string("/")});check(first.refresh_book_document()->tree.node.offset==0 && std::get<std::string>(first.book_selection()->requested.address)=="/","Refresh preserves explicit root-path intent instead of clamping it");
  check(first.history().entries().empty() && first.dictionary_history().entries().empty() && reader.read_book_document("StyledBook",target.node.offset,on).html==raw.html,"Presentation/refresh are history-neutral and Reader raw output stays unchanged");
 }
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.string().c_str(),&worker)==XC_OK,"Open presentation worker");
 check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Create presentation tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_set_engine_options(worker,tab,XC_ENGINE_STRONGS,&request)==XC_OK,"Set C annotation options");done(request);
 check(xc_worker_set_lexical_layout_enabled(worker,tab,1,&request)==XC_OK,"Enable C matching CSS consumer");done(request);
 check(xc_worker_set_image_content(worker,tab,"StyledBook",0,&request)==XC_OK,"Hide C images");done(request);check(xc_worker_set_respect_font_faces(worker,tab,"StyledBook",0,&request)==XC_OK,"Hide C font faces");done(request);
 check(xc_worker_set_double_space(worker,tab,"StyledBook",1,&request)==XC_OK,"Enable C spacing");done(request);
 check(xc_worker_select_book_document(worker,tab,"StyledBook",4,&request)==XC_OK && wait(request)==XC_OK,"Select owned presented C document");auto *document=xc_request_take_book_document(request);xc_request_release(request);
 xc_lexical_presentation layout{};xc_book_document_display_options options{};
 check(document && xc_book_document_presentation_html(document) && xc_book_document_lexical_layout(document,&layout)==XC_OK && layout.requested && layout.applied && std::string(layout.css)==held.presentation->lexical.css && xc_book_document_get_display_options(document,&options)==XC_OK && options.images==0 && options.font_faces==0 && options.double_space==1,"C document owns matching HTML/CSS and display options");
 check(std::string(xc_book_document_html(document))==held.html && std::string(xc_book_document_presentation_html(document))==held.presentation->html,"C raw and presented HTML remain distinct owned values");
 check(xc_worker_read_book_document(worker,tab,"StyledBook",4,&request)==XC_OK && wait(request)==XC_OK,"Read raw C document");auto *raw=xc_request_take_book_document(request);xc_request_release(request);options={9,9,9};layout={reinterpret_cast<const char *>(1),1,1};
 check(!xc_book_document_presentation_html(raw) && xc_book_document_get_display_options(raw,&options)==XC_INVALID_ARGUMENT && options.images==-1 && options.font_faces==-1 && options.double_space==0 && xc_book_document_lexical_layout(raw,&layout)==XC_INVALID_ARGUMENT && !layout.css && !layout.requested && !layout.applied && !xc_book_document_cleanup_count(raw) && !xc_book_document_inline_html(raw) && !xc_book_document_inline_issue_count(raw),"Raw document exposes no presentation and clears optional getters");
 check(xc_book_document_get_display_options(nullptr,&options)==XC_INVALID_ARGUMENT && xc_book_document_get_display_options(document,nullptr)==XC_INVALID_ARGUMENT && xc_book_document_cleanup_kind(document,999)==-1 && !xc_book_document_cleanup_suffix(document,999),"Invalid presentation getter contracts");
 xc_saved_book_selection malformed={"StyledBook","/Malformed",nullptr};check(xc_worker_restore_book_document_selection(worker,tab,&malformed,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK && wait(request)==XC_OK,"Queue malformed presentation diagnostic");auto *diagnostic=xc_request_take_book_document(request);xc_request_release(request);check(xc_book_document_cleanup_count(diagnostic)==1 && xc_book_document_cleanup_kind(diagnostic,0)==1 && std::string(xc_book_document_cleanup_suffix(diagnostic,0)).find("unfinished")!=std::string::npos,"C copies cleanup diagnostic ownership");
 xc_saved_book_selection root_saved={"StyledBook","/",nullptr};check(xc_worker_restore_book_document_selection(worker,tab,&root_saved,XC_BOOK_RESTORE_DISPLAY_KEY,&request)==XC_OK,"Restore explicit root");done(request);
 check(xc_worker_refresh_book_document(worker,tab,&request)==XC_OK,"Queue root refresh before close");xc_worker_close(worker);check(wait(request)==XC_OK,"Close drains presented root refresh");auto *last=xc_request_take_book_document(request);xc_request_release(request);xc_book_node node{};check(last && xc_book_tree_get_node(xc_book_document_tree(last),XC_BOOK_CURRENT,&node)==XC_OK && node.offset==0 && std::string(xc_book_document_presentation_html(document))==held.presentation->html,"Copied presentation survives worker close and root refresh preserves identity");
 xc_book_document_release(document);xc_book_document_release(raw);xc_book_document_release(diagnostic);xc_book_document_release(last);
 check(bytes(root/"data")==before,"Presentation leaves all module data bytes unchanged");
 std::cout<<"Owned combined book presentation, shared cleanup/layout, scoped discovery and root-preserving refresh passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
