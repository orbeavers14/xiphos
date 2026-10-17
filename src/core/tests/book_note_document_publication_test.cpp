// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <glib.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <thread>
#include <chrono>
using namespace xiphos::core;
struct GLOBAL_OPS { int image_content=0,respect_font_faces=0;bool xrefnotenumbers=true; };
struct FrozenBackend {std::string label;char *get_entry_attribute(const char *,const char *,const char *,bool){return label.empty()?nullptr:g_strdup(label.c_str());}} probe;
FrozenBackend *backend=&probe;
int footnote=0,xref=0;
int main_save_module_options(const char *,const char *,int){return 1;}
#define XI_message(args) ((void)0)
namespace frozen {
#include "legacy_html_cleanup.inc"
#include "legacy_content_cleanup.inc"
}
#undef XI_message
#undef CLEAR_FONT_NAME
#undef CLEAR_FONT_NAME_LENGTH
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
xc_status wait(xc_request *request){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Book numbering deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
void done(xc_request *r){check(wait(r)==XC_OK,"Complete book numbering request");xc_request_release(r);}
int main(int argc,char **argv){try{
 check(argc==2,"Original note document fixture required");const std::string root=argv[1];
 EngineOptions options;options.footnotes=true;options.scripturerefs=true;BookDocument held;
 {
  Reader reader(root);StudySession session(reader),plain(reader);session.set_engine_options(options);plain.set_engine_options(options);session.set_note_numbering_enabled(true);
  for(const auto *id:{"TailLabel","FirstLabel","EmptyTail","NoNotes","AllEmpty","SingleLabel","SingleEmpty","CrossRefs","ThMLTail"}){
   const auto target=BookSelectionTarget{id,reader.book_tree_path(id,"/Branch").node.offset};
   check(!plain.select_book_document(target).presentation->note_numbers,"Unopted session keeps unavailable numbering capability");
   for(bool enabled:{true,false}){
    session.set_note_numbers(id,enabled);session.set_image_content(id,false);session.set_respect_font_faces(id,false);
    const auto raw=reader.read_book_document(id,std::get<std::uint64_t>(target.address),options,enabled);
    probe.label=raw.note_source->reported_label;footnote=xref=0;GLOBAL_OPS ops;ops.xrefnotenumbers=enabled;
    auto *expected=frozen::CleanupContent(g_string_new(raw.html.c_str()),&ops,id);
    held=session.select_book_document(target);
    check(held.presentation && held.presentation->note_numbers==std::make_optional(enabled)&&held.presentation->html==expected->str&&held.html==raw.html&&held.note_source->reported_label==raw.note_source->reported_label,"Selected combined document matches complete frozen cleanup/numbering in both modes while retaining raw SDK source");g_string_free(expected,true);
    check(session.refresh_book_document()->presentation->html==held.presentation->html,"Refresh resets combined counters and retains source/number mode");
    if(std::string(id)=="CrossRefs"&&enabled)check(footnote==2&&xref==2&&held.presentation->html.find("*n2")!=std::string::npos&&held.presentation->html.find("*x2")!=std::string::npos,"Footnote and cross-reference counters continue independently across sections");
   }
  }
  const auto profile=(std::filesystem::path(root)/"book-note-settings.conf").string();
  {std::ofstream out(profile);out<<"[Unrelated]\nCustom=Original retained setting\n";}
  session.save_module_preferences(profile);StudySession imported(reader);imported.set_note_numbering_enabled(true);imported.import_module_preferences(profile);
  const auto restored=imported.select_book_document({"FirstLabel",std::uint64_t(4)});
  check(restored.presentation->note_numbers==std::make_optional(false)&&restored.presentation->html.find("*n2")==std::string::npos,"Shared explicit profile save/import restores book number mode without a separate persistence engine");
  check(session.history().entries().empty()&&session.dictionary_history().entries().empty(),"Book numbering does not visit other histories");
 }
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open note document worker");
 check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create numbering tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_set_engine_options(worker,tab,XC_ENGINE_FOOTNOTES|XC_ENGINE_CROSS_REFERENCES,&request)==XC_OK,"Set queued engine options");done(request);
 check(xc_worker_set_note_numbering_enabled(worker,tab,1,&request)==XC_OK,"Enable owned numbering");done(request);
 check(xc_worker_select_book_document(worker,tab,"FirstLabel",4,&request)==XC_OK&&wait(request)==XC_OK,"Select queued numbered document");auto *document=xc_request_take_book_document(request);xc_request_release(request);uint64_t last=999;
 check(document&&xc_book_document_note_source_label(document)&&!*xc_book_document_note_source_label(document)&&xc_book_document_note_source_offset(document,&last)==XC_OK&&last!=4&&xc_book_document_note_numbers(document)==1&&std::string(xc_book_document_presentation_html(document)).find("*n2")!=std::string::npos,"C snapshot owns terminal source and numbered HTML");
 check(xc_worker_set_note_numbers(worker,tab,"FirstLabel",0,&request)==XC_OK,"Disable queued note numbers");done(request);
 check(xc_worker_refresh_book_document(worker,tab,&request)==XC_OK&&wait(request)==XC_OK,"Refresh disabled numbers");auto *off=xc_request_take_book_document(request);xc_request_release(request);check(off&&xc_book_document_note_numbers(off)==0&&std::string(xc_book_document_presentation_html(off)).find("*n2")==std::string::npos,"Copied applied off value retains optional presence");xc_book_document_release(off);
 check(xc_worker_select_book_document(worker,tab,"AllEmpty",4,&request)==XC_OK&&wait(request)==XC_OK,"Select captured all-empty book");auto *empty=xc_request_take_book_document(request);xc_request_release(request);last=999;check(empty&&xc_book_document_note_source_label(empty)&&!*xc_book_document_note_source_label(empty)&&xc_book_document_note_source_offset(empty,&last)==XC_INVALID_ARGUMENT&&!last,"All-skipped source is captured without a terminal offset");xc_book_document_release(empty);
 last=999;check(!xc_book_document_note_source_label(nullptr)&&xc_book_document_note_source_offset(nullptr,&last)==XC_INVALID_ARGUMENT&&!last&&xc_book_document_note_source_offset(document,nullptr)==XC_INVALID_ARGUMENT&&xc_book_document_note_numbers(nullptr)==-1,"Invalid note source getter contracts");
 check(xc_worker_select_book_document(worker,tab,"TailLabel",4,&request)==XC_OK,"Queue terminal labelled book before close");xc_worker_close(worker);check(wait(request)==XC_OK,"Drain numbered document before shutdown");auto *tail=xc_request_take_book_document(request);xc_request_release(request);check(tail&&std::string(xc_book_document_note_source_label(tail))=="tail-label"&&std::string(xc_book_document_presentation_html(document)).find("*n2")!=std::string::npos,"Owned label and numbered snapshots survive worker close");xc_book_document_release(tail);xc_book_document_release(document);
 std::cout<<"Combined book numbering matches frozen cleanup, scopes/counters and owned C publication passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
