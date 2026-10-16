// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/engine_manager.hpp"
#include "book_tree_source.hpp"
#include <markupfiltmgr.h>
#include <swfilter.h>
#include <treekeyidx.h>
#include <utf8html.h>
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
template<class Error,class F> void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected typed book-note error");}
std::map<std::string,std::string> bytes(const fs::path &root){
  std::map<std::string,std::string> result;
  for(const auto &item:fs::recursive_directory_iterator(root))if(item.is_regular_file()){
    std::ifstream input(item.path(),std::ios::binary);check(bool(input),"Read module bytes");
    result.emplace(fs::relative(item.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(input),{}));
  }return result;
}
StudyNote frozen(sword::SWModule &module,unsigned long offset,bool body){
  dynamic_cast<sword::TreeKeyIdx &>(*module.getKey()).setOffset(offset);
  module.setProcessEntryAttributes(true);module.renderText();
  // Frozen pre-extraction source -> copied attributes -> UTF8HTML -> SDK body.
  const auto values=module.getEntryAttributes().at("Footnote").at("1");
  const auto value=[&](const char *key){const auto found=values.find(key);return found==values.end()?std::string():std::string(found->second.c_str());};
  StudyNote result{module.getName(),module.getKeyText(),"1",value("type"),value("n"),"",value("refList")};
  if(body){sword::SWBuf text(value("body").c_str());sword::UTF8HTML encoding;encoding.processText(text);result.html=module.renderText(text.c_str()).c_str();}
  return result;
}
void same(const StudyNote &actual,const StudyNote &expected){
  check(actual.module==expected.module && actual.reference==expected.reference && actual.identifier==expected.identifier &&
        actual.type==expected.type && actual.label==expected.label && actual.html==expected.html && actual.reference_list==expected.reference_list,"Exact frozen note source/metadata/body");
}
struct Fault:sword::SWFilter{
  std::string needle;explicit Fault(std::string value):needle(std::move(value)){}
  char processText(sword::SWBuf &text,const sword::SWKey *,const sword::SWModule *)override{
    if(std::string(text.c_str()).find(needle)!=std::string::npos)throw std::runtime_error("injected book-note failure");return 0;
  }
};
struct MoveKey:sword::SWFilter{
  char processText(sword::SWBuf &,const sword::SWKey *,const sword::SWModule *module)override{dynamic_cast<sword::TreeKeyIdx &>(*module->getKey()).setOffset(8);return 0;}
};
struct Attached{
  sword::SWModule &module;sword::SWFilter &filter;
  Attached(sword::SWModule &m,sword::SWFilter &f):module(m),filter(f){module.addRenderFilter(&filter);}
  ~Attached(){module.removeRenderFilter(&filter);}
};
xc_status wait(xc_request *request){
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;
  while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Book-note request deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;
}
int main(int argc,char **argv){
 try{
  check(argc==2,"Original-prose book fixture required");const std::string root=argv[1];const auto before=bytes(fs::path(root)/"data");
  BookNote retained;std::string expected_html,expected_red_html,unconfigured_html;
  {
   Reader reader(root);EngineManager legacy(root.c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);
   unconfigured_html=reader.book_note("OSISBook",4,"1").note.html;
   for(const auto *id:{"OSISBook","ThMLBook"}){
    auto &module=*legacy.getModule(id);auto &key=dynamic_cast<sword::TreeKeyIdx &>(*module.getKey());
    for(bool notes:{false,true})for(bool red:{false,true})for(unsigned long offset:{4ul,20ul})for(bool body:{false,true}){
      EngineOptions options;options.footnotes=notes;options.words_in_red=red;apply_engine_options(legacy,options);
      const auto expected=frozen(module,offset,body);
      reader.read_book(id,8,options); // stale previous source must never select this note
      const auto result=reader.book_note(id,offset,"1",options,body);
      same(result.note,expected);
      check(result.tree.node.offset==offset && result.tree.node.path==expected.reference,"Note stays paired with exact tree source, including linked alias");
      if(!body)check(result.note.html.empty(),"Metadata-only note avoids body output");
      if(std::string(id)=="OSISBook" && offset==4 && !notes && body){
        if(red)expected_red_html=result.note.html;else expected_html=result.note.html;
      }
    }
    EngineOptions options;options.footnotes=true;apply_engine_options(legacy,options);
    const auto expected=frozen(module,4,true);const auto *identity=module.getKey();
    for(bool processing:{false,true}){
      key.setOffset(8);module.setProcessEntryAttributes(processing);
      auto result=detail::capture_book_note(module,4,"1",true);same(result.note,expected);
      check(module.isProcessEntryAttributes()==processing && module.getKey()==identity && key.getOffset()==4,"Fresh note capture temporarily enables and restores both initial processing states");
      rejects<std::invalid_argument>([&]{detail::capture_book_note(module,8,"1",true);});
      check(module.isProcessEntryAttributes()==processing,"No-note source cannot reuse earlier attributes with capture disabled");
      {
        Fault fault("Original book note.");Attached attached(module,fault);
        check(detail::capture_book_note(module,4,"1",false).note.html.empty(),"Metadata-only avoids deliberately failing note body");
        rejects<std::runtime_error>([&]{detail::capture_book_note(module,4,"1",true);});
        check(module.isProcessEntryAttributes()==processing,"Note-body exception restores caller processing flag");
      }
      {
        Fault fault("Original café");Attached attached(module,fault);
        rejects<std::runtime_error>([&]{detail::capture_book_note(module,4,"1",false);});
        check(module.isProcessEntryAttributes()==processing,"Source-render exception restores caller processing flag");
      }
      check(detail::capture_book_note(module,4,"1",true).note.html==expected.html,"Later book note recovers after source/body errors");
      rejects<std::out_of_range>([&]{detail::capture_book_note(module,40000,"1",true);});
      check(key.getOffset()==4,"Missing note source rejects before changing live key");
      rejects<std::invalid_argument>([&]{detail::capture_book_note(module,8,"",true);});
      check(key.getOffset()==4,"Invalid note ID rejects before selecting another node");
    }
    {MoveKey move;Attached attached(module,move);rejects<std::runtime_error>([&]{detail::capture_book_note(module,4,"1",true);});}
    check(reader.book_note(id,4,"1").note.html.find("Original book note.")!=std::string::npos,"Unconfigured note overload reuses Reader baseline");
   }
   check(expected_html!=expected_red_html,"Real book-note red-letter options differ");
   EngineOptions options;options.footnotes=true;
   rejects<std::invalid_argument>([&]{reader.book_note("OSISBook",4,"missing",options);});
   rejects<std::invalid_argument>([&]{reader.book_note("OSISBook",4,std::string("1\0bad",5),options);});
   rejects<std::invalid_argument>([&]{reader.book_note("OSISBook",12,"1",options);});
   rejects<std::invalid_argument>([&]{reader.book_note("OSISBook",0,"1",options);});
   rejects<std::invalid_argument>([&]{reader.book_note("Dictionary",0,"1",options);});
   rejects<std::invalid_argument>([&]{reader.book_note("VerseBook",0,"1",options);});
   rejects<std::out_of_range>([&]{reader.book_note("Missing",4,"1",options);});
   rejects<std::out_of_range>([&]{reader.book_note("OSISBook",40000,"1",options);});
   rejects<std::invalid_argument>([&]{reader.book_note(std::string("OSISBook\0bad",12),4,"1",options);});
   retained=reader.book_note("OSISBook",20,"1",options);reader.read_book("OSISBook",8,options);
  }
  check(retained.tree.node.path=="/Linked" && retained.note.reference=="/Linked" && retained.note.html.find("Original book note.")!=std::string::npos,"Owned note/source survives later reads and Reader destruction");
  std::future<BookNote> drained;
  {
   StudyWorker worker(root);const auto a=worker.create_tab().get(),b=worker.create_tab().get();
   EngineOptions options;options.footnotes=true;options.words_in_red=true;worker.set_engine_options(a,options).get();
   auto missing=worker.book_note(a,"OSISBook",8,"1");auto good=worker.book_note(a,"OSISBook",20,"1");
   rejects<std::invalid_argument>([&]{(void)missing.get();});const auto queued=good.get();check(queued.tree.node.offset==20 && queued.note.html==expected_red_html,"Queued note errors do not stop later source lookup or lose tab options");
   worker.set_module_engine_options(a,"OSISBook",{}).get();
   check(worker.book_note(a,"OSISBook",4,"1").get().note.html==expected_html,"Book note uses existing module-over-tab precedence");
   worker.clear_module_engine_options(a,"OSISBook").get();
   check(worker.book_note(a,"OSISBook",4,"1").get().note.html==expected_red_html,"Clearing override restores tab book-note options");
   check(worker.book_note(b,"ThMLBook",4,"1",false).get().note.html.empty(),"Other tab metadata-only baseline");
   check(worker.history(a).get().entries.empty() && worker.dictionary_history(a).get().entries.empty(),"Book note queries add no history");
   worker.close_tab(a).get();rejects<std::invalid_argument>([&]{(void)worker.book_note(a,"OSISBook",4,"1").get();});
   drained=worker.book_note(b,"OSISBook",4,"1");
  }
  check(drained.get().note.html==unconfigured_html,"Worker close drains owned book note");
  xc_worker *worker=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open C note worker");xc_request *request=nullptr;
  check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Create C note tab");const auto tab=xc_request_tab(request);xc_request_release(request);
  request=reinterpret_cast<xc_request *>(1);check(xc_worker_book_note(worker,tab,"OSISBook",4,"1",2,&request)==XC_INVALID_ARGUMENT && !request,"Strict body selector resets output");
  check(xc_worker_book_note(worker,tab,"OSISBook",4,nullptr,1,&request)==XC_INVALID_ARGUMENT && !request,"NULL identifier rejects");
  check(xc_worker_book_note(worker,tab,"OSISBook",4,"",1,&request)==XC_INVALID_ARGUMENT && !request,"Empty identifier rejects");
  char id[]="OSISBook",number[]="1";check(xc_worker_book_note(worker,tab,id,4,number,1,&request)==XC_OK,"Queue copied note arguments");id[0]='X';number[0]='9';
  check(!xc_request_take_book_note(request) && wait(request)==XC_OK && !xc_request_take_note(request) && !xc_request_take_book_entry(request),"Book note take/poll and wrong-kind ownership");
  auto *note=xc_request_take_book_note(request);check(note && !xc_request_take_book_note(request),"Take book note once");xc_request_release(request);
  const auto *content=xc_book_note_content(note);const auto *tree=xc_book_note_tree(note);xc_book_node node{};
  check(std::string(xc_note_html(content))==unconfigured_html && std::string(xc_note_reference(content))=="/Part One" && std::string(xc_note_identifier(content))=="1" && xc_book_tree_get_node(tree,XC_BOOK_CURRENT,&node)==XC_OK && node.offset==4,"Borrowed C note and typed source agree");
  check(xc_worker_set_engine_options(worker,tab,0,&request)==XC_OK && wait(request)==XC_OK,"Queue explicit all-Off C book-note options");xc_request_release(request);
  check(xc_worker_book_note(worker,tab,"OSISBook",4,"1",1,&request)==XC_OK && wait(request)==XC_OK,"Read explicit all-Off C note body");
  auto *plain_note=xc_request_take_book_note(request);xc_request_release(request);
  check(std::string(xc_note_html(xc_book_note_content(plain_note)))==expected_html,"C note body honors explicit Off rather than default SDK formatting");xc_book_note_release(plain_note);
  check(xc_worker_set_engine_options(worker,tab,XC_ENGINE_WORDS_IN_RED,&request)==XC_OK && wait(request)==XC_OK,"Queue C red-letter book-note option");xc_request_release(request);
  check(xc_worker_book_note(worker,tab,"OSISBook",4,"1",1,&request)==XC_OK && wait(request)==XC_OK,"Read C book note with actual changed body option");
  auto *red_note=xc_request_take_book_note(request);xc_request_release(request);
  check(std::string(xc_note_html(xc_book_note_content(red_note)))==expected_red_html,"C note body uses configured tab options");xc_book_note_release(red_note);
  check(xc_worker_book_note(worker,tab,"OSISBook",20,"1",0,&request)==XC_OK && wait(request)==XC_OK,"Queue linked metadata-only note");
  auto *metadata=xc_request_take_book_note(request);xc_request_release(request);
  check(!*xc_note_html(xc_book_note_content(metadata)) && std::string(xc_note_reference(xc_book_note_content(metadata)))=="/Linked","Metadata-only C note retains alias context");xc_book_note_release(metadata);
  check(xc_worker_book_note(worker,tab,"OSISBook",8,"1",1,&request)==XC_OK && wait(request)==XC_INVALID_ARGUMENT && !xc_request_take_book_note(request),"No-note source cannot return stale C result");xc_request_release(request);
  check(xc_worker_book_note(worker,tab,"OSISBook",40000,"1",1,&request)==XC_OK && wait(request)==XC_MODULE_NOT_FOUND,"Missing typed source error");xc_request_release(request);
  check(xc_worker_book_note(worker,tab,"OSISBook",20,"1",1,&request)==XC_OK,"Queue C note before close");xc_worker_close(worker);
  check(wait(request)==XC_OK,"C close drains accepted note request");metadata=xc_request_take_book_note(request);xc_request_release(request);
  check(std::string(xc_note_html(content))==unconfigured_html && xc_book_tree_get_node(xc_book_note_tree(metadata),XC_BOOK_CURRENT,&node)==XC_OK && node.offset==20,"Owned note and borrowed views survive worker close");
  xc_book_note_release(metadata);xc_book_note_release(note);xc_book_note_release(nullptr);
  check(bytes(fs::path(root)/"data")==before,"Note metadata/body/errors preserve all module data bytes and filenames");
  std::cout<<"Exact book-note sources, shared renderer, stale-state recovery and queued C lifetime passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
