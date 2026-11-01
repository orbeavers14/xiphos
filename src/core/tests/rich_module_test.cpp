// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
static thread_local bool watching=false;
static thread_local std::size_t allocations=0;
static thread_local int fail_after=-1;
void *operator new(std::size_t n){if(watching){++allocations;if(fail_after==0)throw std::bad_alloc();if(fail_after>0)--fail_after;}if(auto *p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void *operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void *p)noexcept{std::free(p);}void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p,std::size_t)noexcept{std::free(p);}void operator delete[](void *p,std::size_t)noexcept{std::free(p);}
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static xc_status wait(xc_request *r){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(30);xc_status s;while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Tabs deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return s;}
template<class T>static void rejected(std::future<T> value,bool immediate=false){if(immediate)check(value.wait_for(std::chrono::seconds(0))==std::future_status::ready,"Admission rejection before FIFO");try{(void)value.get();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected tab profile rejection");}
#include <rawtext.h>
#include <rawcom.h>
#include <rawld.h>
#include <rawgenbook.h>
using namespace sword;
static std::string bytes(const std::filesystem::path &file){std::ifstream in(file,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
static void fixture(const std::filesystem::path &root){
 check(root.filename()=="rich-module-fixture","Only the named disposable fixture may be replaced");std::filesystem::remove_all(root);std::filesystem::create_directories(root/"mods.d");
 for(const auto *id:{"Text","TextAlt","Notes","Dictionary"}){
  const auto data=root/"data"/id;std::filesystem::create_directories(data);const bool dictionary=std::string(id)=="Dictionary",notes=std::string(id)=="Notes";const auto prefix=dictionary?data/"entries":data;const std::string canon=std::string(id)=="TextAlt"?"KJVA":"KJV";
  check((dictionary?RawLD::createModule(prefix.u8string().c_str()):notes?RawCom::createModule(prefix.u8string().c_str()):RawText::createModule(prefix.u8string().c_str(),canon.c_str()))==0,"Create original capture module");
  std::unique_ptr<SWModule> writer;if(dictionary)writer=std::make_unique<RawLD>(prefix.u8string().c_str());else if(notes)writer=std::make_unique<RawCom>(prefix.u8string().c_str());else writer=std::make_unique<RawText>(prefix.u8string().c_str(),nullptr,nullptr,nullptr,ENC_UTF8,DIRECTION_LTR,FMT_PLAIN,"en",canon.c_str());
  for(const auto *key:dictionary?std::vector<const char *>{"alpha","beta","anchor-key"}:std::vector<const char *>{"Genesis1:1","John3:16","John3:17","John3:18","John4:1"}){writer->setKey(key);writer->setEntry(("Original capture prose "+std::string(id)+" "+key).c_str());}
  std::ofstream out(root/"mods.d"/(std::string(id)+".conf"));out<<'['<<id<<"]\nDataPath=./data/"<<id<<(dictionary?"/entries":"/")<<"\nModDrv="<<(dictionary?"RawLD":notes?"RawCom":"RawText")<<"\nSourceType=Plain\nEncoding=UTF-8\nLang=en\nDescription=Original session capture fixture\nVersification="<<canon<<'\n';if(dictionary)out<<"Feature=GreekDef\n";
 }
 const auto folder=root/"data/CaptureBook";std::filesystem::create_directories(folder);const auto prefix=(folder/"entries").u8string();check(RawGenBook::createModule(prefix.c_str())==0,"Create original capture book");{RawGenBook writer(prefix.c_str());writer.setKey("/Original");writer.setEntry("Original capture book prose.");}
 std::ofstream out(root/"mods.d/CaptureBook.conf");out<<"[CaptureBook]\nDataPath=./data/CaptureBook/entries\nModDrv=RawGenBook\nSourceType=Plain\nEncoding=UTF-8\nDescription=Original capture book\n";
}
#include "xiphos/core/rich_document.hpp"
#include <rawfiles.h>
static void rejects(const std::function<void()> &f){try{f();}catch(const std::exception &){return;}throw std::runtime_error("Expected rich/module edit rejection");}
static void formatting(){
 const auto d=parse_rich_document("<h2><b>Original Ω</b></h2><p>A &amp; B <a href='sword://Text/John3:16'>link</a></p>");
 check(rich_document_text(d)=="Original Ω\nA & B link","Rich parser headings/entities/links");
 auto f=format_rich_document(d,0,8,{RichAction::italic,-1,""});auto html=serialize_rich_document(f);check(html.find("<b><i>Original</i></b>")!=std::string::npos,"Shared format adds balanced inline style");check(rich_document_text(parse_rich_document(html))==rich_document_text(d),"Rich canonical serialization preserves Unicode/text");
 for(int block=0;block<=12;++block){auto input=parse_rich_document("one<br/>two Ω");auto changed=format_rich_document(input,0,3,{RichAction::block,block,""});auto round=parse_rich_document(serialize_rich_document(changed));check(rich_document_text(round)=="one\ntwo Ω","All block/list formats preserve text");}
 auto all=parse_rich_document("a Ω z");all=format_rich_document(all,2,2,{RichAction::foreground,0,"red"});all=format_rich_document(all,2,2,{RichAction::background,0,"#abc"});all=format_rich_document(all,2,2,{RichAction::link,0,"https://example.invalid/?a=1&b=2"});const auto encoded=serialize_rich_document(all);check(encoded.find("#ff0000")!=std::string::npos&&encoded.find("&amp;b=2")!=std::string::npos&&rich_document_text(parse_rich_document(encoded))=="a Ω z","Colors and URI attributes escape safely");
 rejects([&]{format_rich_document(all,3,1,{RichAction::bold,-1,""});});for(const auto *source:{"<script>bad()</script>","<img src='x'>","<span style='font-size:20px'>x</span>","<a href='javascript:bad()'>x</a>","<table><tr><td>x</td></tr></table>"})rejects([&]{parse_rich_document(source);});
 check(serialize_rich_document(parse_rich_document("<b>last</b>"))=="<b>last</b>","Final closing tags retained, unlike legacy end-iterator omission");
 xc_rich_document *c=nullptr;check(xc_rich_document_parse("<b>original</b>",&c)==XC_OK,"C rich parse");xc_rich_document *next=nullptr;check(xc_rich_document_format(c,0,8,1,-1,"",&next)==XC_OK,"C rich format");xc_rich_document_release(c);check(std::string(xc_rich_document_html(next))=="<b><i>original</i></b>","Owned C format result");xc_rich_document_release(next);
}
int main(int argc,char **argv){try{
 EditorBuffer buffer("original");buffer.replace("changed");check(buffer.dirty()&&buffer.can_undo(),"Shared dirty/undo");buffer.mark_saved();buffer.undo();check(buffer.text()=="original"&&buffer.dirty(),"Undo after save remains dirty");buffer.redo();check(!buffer.dirty(),"Redo returns to saved baseline");
 formatting();check(argc==2,"Rich module fixture required");auto root=std::filesystem::u8path(argv[1]);fixture(root);
 {auto file=root/"mods.d/CaptureBook.conf";auto config=bytes(file);const auto at=config.find("SourceType=Plain");check(at!=config.npos,"Fixture source declaration");config.replace(at,16,"SourceType=ThML");std::ofstream out(file);out<<config;}
 const auto personal=root/"data/Personal";std::filesystem::create_directories(personal);check(RawFiles::createModule(personal.u8string().c_str())==0,"Personal fixture created");{RawFiles writer(personal.u8string().c_str());writer.setKey("John3:16");writer.setEntry("<b>Original personal note.</b>");writer.setKey("John3:17");writer.setEntry("Original independent note.");}
 {std::ofstream out(root/"mods.d/Personal.conf");out<<"[Personal]\nDataPath=./data/Personal/\nModDrv=RawFiles\nSourceType=ThML\nEncoding=UTF-8\nLang=en\nDescription=Original personal editing fixture\n";}
 {StudyWorker worker(root.u8string());auto tab=worker.create_tab().get(),other=worker.create_tab().get();(void)worker.visit_passage(tab,"Text","Personal","John3:16").get();(void)worker.select_book_document(other,{"CaptureBook",std::string("/Original")}).get();SearchQuery query;query.module="Text";query.expression="Original";auto generation=worker.search_module(tab,query).get().generation.id;auto history=worker.history(tab).get();auto note=worker.read_module_edit(tab,{"Personal","John3:16"}).get();check(note.text=="<b>Original personal note.</b>"&&note.driver=="RawFiles","Read owned exact RawFiles source");
  const auto replacement="<b>Revised Ω note &amp; literal %20.</b>";auto updated=worker.save_module_edit(tab,{"Personal",note.reference},note.revision,replacement).get();check(updated.text==replacement&&worker.read_passage(tab,"Text","Personal","John3:16").get().commentary->html.find("Revised")!=std::string::npos,"Saved personal edit reloads live SDK renderer");check(worker.search_generation(tab).get().id==0&&worker.search_module(tab,query).get().generation.id>generation,"Module write invalidates search caches without reusing generation IDs");check(worker.history(tab).get().entries.size()==history.entries.size()&&worker.reading_selection(tab).get()->module=="Text","Module edit retains live tab IDs/history/intent");
  rejects([&]{(void)worker.save_module_edit(tab,{"Personal",note.reference},note.revision,"stale").get();});check(worker.read_module_edit(tab,{"Personal",note.reference}).get().text==replacement,"Stale whole-module revision retains content");
  auto book=worker.read_module_edit(tab,{"CaptureBook","/Original"}).get();auto changed=worker.save_module_edit(tab,{"CaptureBook","/Original"},book.revision,"Original revised book prose.").get();check(changed.offset==book.offset&&worker.book_document_selection(other).get()->html.find("revised book")!=std::string::npos,"Book edit refreshes sibling-tab cached documents and preserves tree address");
  auto cleared=worker.save_module_edit(tab,{"CaptureBook","/Original"},changed.revision,"",true).get();check(cleared.text.empty()&&cleared.offset==book.offset,"Book erase clears content without deleting node");
  auto erased=worker.save_module_edit(tab,{"Personal",note.reference},updated.revision,"",true).get();check(erased.text.empty(),"Personal note delete uses SDK semantics");
  rejects([&]{(void)worker.read_module_edit(tab,{"Text","John3:16"}).get();});rejects([&]{(void)worker.read_module_edit(tab,{"CaptureBook","/Missing"}).get();});
  for(const auto *ambiguous:{"3:16","16","John3:16-17","John3:16garbage"})rejects([&]{(void)worker.read_module_edit(tab,{"Personal",ambiguous}).get();});
 }
 // Inject a post-install refresh failure: old SDK + original data must survive.
 {Reader reader(root.u8string());auto original=reader.read_module_edit({"Personal","John3:17"});rejects([&]{reader.save_module_edit({"Personal","John3:17"},original.revision,"Rollback candidate",false,[]{throw std::runtime_error("Injected cache refresh failure");});});check(reader.read_module_edit({"Personal","John3:17"}).text==original.text,"Failed refresh rolls back disk and old SDK state");}
 xc_worker *worker=nullptr;check(xc_worker_open(root.u8string().c_str(),&worker)==XC_OK,"C module editor worker");xc_request *r=nullptr;check(xc_worker_create_tab(worker,&r)==XC_OK&&wait(r)==XC_OK,"C module tab");const auto tab=xc_request_tab(r);xc_request_release(r);check(xc_worker_read_module_edit(worker,tab,"Personal","John3:17",&r)==XC_OK&&wait(r)==XC_OK,"C module read");auto input=xc_request_take_module_edit(r);xc_request_release(r);const std::string revision=xc_module_edit_revision(input);
 for(int fail:{0,1}){watching=true;fail_after=fail;auto status=xc_worker_save_module_edit(worker,tab,"Personal","John3:17",revision.c_str(),"Unaccepted",0,&r);watching=false;fail_after=-1;check(status!=XC_OK&&!r,"Module observer/envelope fails before acceptance");}
 check(xc_worker_save_module_edit(worker,tab,"Personal","John3:17",revision.c_str(),"Original C saved note",0,&r)==XC_OK,"C module save");xc_module_edit_release(input);watching=true;allocations=0;fail_after=0;auto status=wait(r);watching=false;fail_after=-1;check(status==XC_OK&&allocations==0,"Committed module polling allocates nothing");auto owned=xc_request_take_module_edit(r);check(owned&&!xc_request_take_module_edit(r),"Owned module result take once");xc_request_release(r);xc_worker_close(worker);check(std::string(xc_module_edit_text(owned))=="Original C saved note","Module result survives source/worker teardown");xc_module_edit_release(owned);
 std::cout<<"Rich formatting and staged RawFiles/RawGenBook edits, cache refresh and rollback passed\n";return 0;
 }catch(const std::exception &e){watching=false;std::cerr<<e.what()<<'\n';return 1;}}
