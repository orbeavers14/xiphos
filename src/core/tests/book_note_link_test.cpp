// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F call){try{call();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected source-bound URI rejection");}
std::string replace(std::string value,const std::string &from,const std::string &to){const auto at=value.find(from);check(at!=std::string::npos,"Mutation anchor");value.replace(at,from.size(),to);return value;}
std::string note_uri(const BookEntry &entry){const auto at=entry.html.find("href=\"");check(at!=std::string::npos,"SDK book note marker href");const auto end=entry.html.find('"',at+6);check(end!=std::string::npos,"Complete SDK note href");return entry.html.substr(at+6,end-at-6);}
xc_status wait(xc_request *request){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Book-note URI deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){
 try{
  check(argc==2,"Original book fixture required");const std::string root=argv[1];
  const std::string short_uri="passagestudy.jsp?action=showNote&type=n&value=1&module=Book&passage=%2FA";
  check(book_note_link(short_uri,"Book","/A").reference=="/A","Short book title has no verse fallback");
  check(book_note_link(replace(short_uri,"module=Book&",""),"Book","/A").module=="Book","Omitted module defaults only to captured book");
  check(book_note_link(replace(short_uri,"%2FA",""),"Book","").reference.empty(),"Explicit empty passage can address actual root");
  const auto special=replace(short_uri,"%2FA","%2Fcaf%C3%A9+%CE%B1%2F123%23%3F%21%2D%3B%2C");
  check(book_note_link(special,"Book","/café α/123#?!-;,").reference=="/café α/123#?!-;,","Decoded Unicode/reserved title characters are compared as one exact book path");
  for(const auto &uri:{short_uri,special})rejects([&]{study_note_link(uri,"Book");});
  for(const auto &bad:{"https://example.test/"+short_uri,"javascript:"+short_uri,short_uri+"#fragment",short_uri+"&extra=1",short_uri+"&passage=%2FA",short_uri+"&",replace(short_uri,"value=1","value="),replace(short_uri,"%2FA","%00"),replace(short_uri,"%2FA","%2"),replace(short_uri,"%2FA","%GG"),replace(short_uri,"showNote","showRef"),replace(short_uri,"type=n","type=z"),replace(short_uri,"module=Book","module=Other"),replace(short_uri,"%2FA","%2FB"),replace(short_uri,"&passage=%2FA","")})
    rejects([&]{book_note_link(bad,"Book","/A");});
  rejects([&]{book_note_link(short_uri,"","/A");});
  rejects([&]{book_note_link(short_uri,"Book",std::string("/A\0bad",6));});
  rejects([&]{book_note_link(short_uri,"Book","A");});
  rejects([&]{book_note_link(short_uri,std::string("Book\0bad",8),"/A");});
  check(book_note_link("xiphos-preview://study/"+short_uri,"Book","/A").identifier=="1","Fixed native origin reuses common parser allowlist");
  EngineOptions options;options.footnotes=true;std::string uri,linked,expected;
  {
    Reader reader(root);uri=note_uri(reader.read_book("OSISBook",4,options));linked=note_uri(reader.read_book("OSISBook",20,options));
    expected=reader.book_note("OSISBook",4,"1",options).note.html;
    StudySession session(reader);session.set_engine_options(options);
    reader.read_book("OSISBook",8,options);
    auto result=session.book_note_link_at("OSISBook",4,uri);
    check(result.tree.node.offset==4 && result.note.html==expected,"Real SDK marker resolves from captured source after later navigation");
    const auto alias=session.book_note_link_at("OSISBook",20,linked,false);
    check(alias.tree.node.path=="/Linked" && alias.note.reference=="/Linked" && alias.note.html.empty(),"Linked metadata-only URI keeps selected alias");
    const auto thml=note_uri(reader.read_book("ThMLBook",4,options));
    check(session.book_note_link_at("ThMLBook",4,thml).note.html.find("Original book note.")!=std::string::npos,"Real ThML marker uses common source/renderer");
    rejects([&]{session.book_note_link_at("OSISBook",8,uri);});
    rejects([&]{session.book_note_link_at("OSISBook",20,uri);});
    rejects([&]{session.book_note_link_at("ThMLBook",4,uri);});
    rejects([&]{session.book_note_link_at("OSISBook",4,replace(uri,"type=n","type=x"));});
    rejects([&]{session.book_note_link_at("OSISBook",4,replace(uri,"value=1","value=99"));});
    check(session.history().entries().empty() && session.dictionary_history().entries().empty(),"Source-bound note commands add no history");
  }
  std::future<BookNote> drained;
  {
    StudyWorker worker(root);const auto tab=worker.create_tab().get();worker.set_engine_options(tab,options).get();
    auto bad=worker.book_note_link_at(tab,"OSISBook",8,uri);auto good=worker.book_note_link_at(tab,"OSISBook",4,uri);
    rejects([&]{(void)bad.get();});check(good.get().note.html==expected,"Worker continues after mismatched note URI");
    drained=worker.book_note_link_at(tab,"OSISBook",20,linked);
  }
  check(drained.get().tree.node.offset==20,"Worker close drains source-bound URI lookup");
  xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open C URI worker");
  check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Create URI tab");const auto tab=xc_request_tab(request);xc_request_release(request);
  check(xc_worker_set_engine_options(worker,tab,XC_ENGINE_FOOTNOTES,&request)==XC_OK && wait(request)==XC_OK,"Configure exact C URI options");xc_request_release(request);
  request=reinterpret_cast<xc_request *>(1);check(xc_worker_book_note_link_at(worker,tab,"OSISBook",4,nullptr,1,&request)==XC_INVALID_ARGUMENT && !request,"NULL URI resets request");
  check(xc_worker_book_note_link_at(worker,tab,"OSISBook",4,"",1,&request)==XC_INVALID_ARGUMENT && !request,"Empty URI rejects");
  check(xc_worker_book_note_link_at(worker,tab,"OSISBook",4,uri.c_str(),2,&request)==XC_INVALID_ARGUMENT && !request,"Strict URI body selector");
  check(xc_worker_book_note_link_at(worker,tab,"OSISBook",8,uri.c_str(),1,&request)==XC_OK && wait(request)==XC_INVALID_ARGUMENT && !xc_request_take_book_note(request),"C URI cannot drift to another tree source");xc_request_release(request);
  char copied[1024];check(uri.size()<sizeof(copied),"Fixture URI copy bound");std::strcpy(copied,uri.c_str());
  check(xc_worker_book_note_link_at(worker,tab,"OSISBook",4,copied,1,&request)==XC_OK,"Queue copied real SDK URI");std::memset(copied,'x',uri.size());
  check(wait(request)==XC_OK && !xc_request_take_note(request),"Source-bound URI returns typed book-note kind");auto *note=xc_request_take_book_note(request);check(note && !xc_request_take_book_note(request),"URI note take once");xc_request_release(request);
  check(std::string(xc_note_html(xc_book_note_content(note)))==expected,"C URI renders exact common note body");
  check(xc_worker_book_note_link_at(worker,tab,"OSISBook",20,linked.c_str(),0,&request)==XC_OK,"Queue linked metadata URI before close");xc_worker_close(worker);
  check(wait(request)==XC_OK,"C close drains URI lookup");auto *metadata=xc_request_take_book_note(request);xc_request_release(request);xc_book_node node{};
  check(xc_book_tree_get_node(xc_book_note_tree(metadata),XC_BOOK_CURRENT,&node)==XC_OK && node.offset==20 && !*xc_note_html(xc_book_note_content(metadata)),"C linked URI retains typed identity and metadata-only behavior");
  check(std::string(xc_note_html(xc_book_note_content(note)))==expected,"URI result survives worker shutdown");xc_book_note_release(metadata);xc_book_note_release(note);
  std::cout<<"Source-bound SDK book-note URI parsing, mutation rejection and queued C checks passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
