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
 check(root.filename()=="native-session-fixture","Only the named disposable fixture may be replaced");std::filesystem::remove_all(root);std::filesystem::create_directories(root/"mods.d");
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
static void rejects(const std::function<void()> &f){try{f();}catch(const std::exception &){return;}throw std::runtime_error("Expected native session rejection");}
static void replace_once(std::string &s,const std::string &from,const std::string &to){auto at=s.find(from);check(at!=std::string::npos,"Schema mutation target exists");s.replace(at,from.size(),to);}
static NativeSessionSnapshot example(){
 NativeSessionSnapshot s;s.active_key="study";NativeSessionTab tab;tab.key="study";auto &v=tab.state;
 v.reading=Location{"John 3:16","TextAlt","Notes"};v.history={{"John 3:16","Text","Notes"},{"John 3:17","TextAlt",""}};v.history_cursor=0;
 v.dictionary=DictionaryLocation{"Dictionary","beta"};v.dictionary_history={{"Dictionary","alpha"},{"Dictionary","beta"}};v.dictionary_cursor=1;
 v.engine=EngineOptions{};v.engine->strongs=true;v.engine->variants_primary=true;v.engine->variants_all=true;v.engine->morphseg=true;
 v.module_engine["not-installed"]=*v.engine;v.module_engine["Text"]=EngineOptions{};v.module_defaults=true;
 ModulePresentationOptions presentation;presentation.headings=false;presentation.double_space=true;presentation.image_content=ContentPreference::off;presentation.respect_font_faces=ContentPreference::on;presentation.note_numbers=false;v.presentation["Text"]=presentation;
 v.lexicons=LexiconPreferences{};v.lexicons->greek="Dictionary";v.lexicons->hebrew.reset();v.lexicons->greek_morph_nt="";v.cross_reference_preview_list=false;
 v.search_text=SearchTextOptions{true,false,true};v.search_retention={17,8192};v.parallel=ParallelSelection{{"Text","Missing","TextAlt"},"Text","John 3:17",*v.engine,false,1024*1024};
 tab.visibility={true,false,true,false};tab.commentary_showing=true;s.tabs.push_back(tab);NativeSessionTab empty;empty.key="empty";s.tabs.push_back(empty);return s;
}
static void codec(){
 auto s=example();const auto xml=serialize_native_session(s);const auto decoded=parse_native_session(xml);check(serialize_native_session(decoded)==xml,"Native codec is lossless/canonical across every state field");
 check(decoded.tabs[0].state.lexicons->hebrew==std::nullopt&&decoded.tabs[0].state.lexicons->greek_morph_nt==""&&!decoded.tabs[1].state.engine,"Absent/empty/unconfigured state remains distinct");
 auto text=s;text.tabs[0].key="tab<&\"\n Ω";text.active_key=text.tabs[0].key;text.tabs[0].state.dictionary->key="a\t\n<&\" Ω";check(serialize_native_session(parse_native_session(serialize_native_session(text)))==serialize_native_session(text),"Escaped whitespace and Unicode round trip");
 for(const auto &change:std::vector<std::pair<std::string,std::string>>{{"version=\"1\"","version=\"2\""},{"version=\"1\"","version=\"0\""},{"texts=\"1\"","texts=\"yes\""},{"cursor=\"0\"","cursor=\"99\""},{"retained_elements=\"17\"","retained_elements=\"-1\""},{"<XiphosSession ","<XiphosSession unknown=\"keep\" "},{"<state ","<state xmlns=\"unexpected\" "},{"</state>","<unknown/></state>"},{"</state>","<reading module=\"Text\" reference=\"John 3:16\" commentary=\"\"/></state>"}}){auto bad=xml;replace_once(bad,change.first,change.second);rejects([&]{parse_native_session(bad);});}
 rejects([&]{parse_native_session("<!DOCTYPE XiphosSession [<!ENTITY t 'x'>]>"+xml.substr(xml.find("<XiphosSession")));});
 auto hidden=xml;hidden.insert(hidden.size()/2,1,'\0');rejects([&]{parse_native_session(hidden);});
 auto bad=s;bad.tabs.push_back(bad.tabs[0]);rejects([&]{validate_native_session(bad);});bad=s;bad.active_key="absent";rejects([&]{validate_native_session(bad);});bad=s;bad.tabs[0].state.history.push_back(bad.tabs[0].state.history[0]);rejects([&]{validate_native_session(bad);});
 bad=s;bad.tabs[0].state.dictionary_history.resize(51);rejects([&]{validate_native_session(bad);});bad=s;bad.tabs[0].state.presentation["Text"].image_content=static_cast<ContentPreference>(2);rejects([&]{validate_native_session(bad);});
 bad=s;bad.tabs[0].state.reading->reference=std::string(32769,'a');rejects([&]{validate_native_session(bad);});bad=s;bad.tabs.resize(257);rejects([&]{validate_native_session(bad);});
 check(parse_native_session(serialize_native_session({})).tabs.empty(),"Zero tabs remains zero tabs");
}
int main(int argc,char **argv){try{
 codec();check(argc==2,"Native fixture required");const auto root=std::filesystem::u8path(argv[1]);fixture(root);const auto file=root/"native-session.xml";NativeSessionSnapshot saved;
 {
  StudyWorker worker(root.u8string());auto a=worker.create_tab().get(),b=worker.create_tab().get();
  worker.enable_module_preferences(a).get();worker.set_double_space(a,"Text",true).get();worker.set_image_content(a,"Text",false).get();
  (void)worker.visit_passage(a,"Text","Notes","John3:16").get();(void)worker.visit_passage(a,"TextAlt","Notes","John3:16").get();(void)worker.visit(a,"Text","John3:17").get();(void)worker.navigate_passage(a,-1).get();(void)worker.visit_passage(a,"TextAlt","Notes","John3:16").get();
  (void)worker.visit_dictionary(a,"Dictionary","alpha").get();(void)worker.visit_dictionary(a,"Dictionary","beta").get();(void)worker.navigate_dictionary_history(a,-1).get();
  (void)worker.visit_parallel(a,{"Text","Missing","TextAlt"},"John3:17",{},false).get();(void)worker.select_book_document(b,{"CaptureBook",std::string("/Original")}).get();(void)worker.set_search_retention_limits(a,{7,2048}).get();
  SearchQuery query;query.module="Text";query.expression="Original";query.mode=SearchMode::words;query.text_options=SearchTextOptions{true,false,true};
  const auto searched=worker.search_module(a,query).get();check(searched.generation_updated&&worker.last_search_scope(a).get().present,"Fixture has transient retained search results");
  saved=worker.save_native_session(file.u8string(),{{b,"book",{false,true,false,false},false,false},{a,"study",{},true,true}},"study").get();
  check(saved.tabs[1].state.reading->module=="TextAlt"&&saved.tabs[1].state.history[0].module=="Text"&&saved.tabs[1].state.history.size()==2&&saved.tabs[1].state.history_cursor==0,"Current module pair and exact dedup history/cursor saved independently");
  const auto xml=bytes(file);check(serialize_native_session(worker.read_native_session(file.u8string()).get())==xml,"Worker storage-only read is lossless");
  auto bad=saved;bad.tabs.back().state.reading->module="Missing";rejects([&]{(void)worker.restore_native_session(bad).get();});check(worker.reading_selection(a).get()->module=="TextAlt"&&worker.book_selection(b).get().has_value(),"Bad final tab retains all previous tabs");
  bad=saved;bad.tabs.front().state.book->path="/changed";rejects([&]{(void)worker.restore_native_session(bad).get();});check(worker.history(a).get().entries.size()==2,"Changed saved book provenance rejects without replacing live tabs");
  const auto restored=worker.load_native_session(file.u8string(),{true,true,true}).get();check(restored.tabs.size()==2&&restored.tabs[0].key=="book"&&restored.active_tab==restored.tabs[1].tab&&restored.tabs[0].tab==b+1,"Restoration returns ordered fresh IDs and active tab");
  rejects([&]{(void)worker.history(a).get();});rejects([&]{(void)worker.history(b).get();});
  const auto fresh=worker.capture_native_session({{restored.tabs[0].tab,"book",saved.tabs[0].visibility,false,false},{restored.tabs[1].tab,"study",{},true,true}},"study").get();check(serialize_native_session(fresh)==xml,"Complete capture after restoration matches saved state, capabilities excluded");
  const auto id=restored.tabs[1].tab;check(worker.search_generation(id).get().id==0&&!worker.last_search_scope(id).get().present,"Restore discards transient search generations/scope");check(worker.search_text_options(id).get().options.has_value(),"Saved search annotation policy restored independently of results");check(worker.dictionary_history(id).get().can_forward&&worker.history(id).get().can_forward,"Both restored forward cursors survive");
  (void)worker.navigate_passage(id,1).get();(void)worker.navigate_dictionary_history(id,1).get();check(worker.dictionary_selection(id).get()->key=="beta","Restored histories navigate with original policies");
  worker.clear_history(id).get();worker.clear_dictionary_history(id).get();const auto cleared=worker.capture_native_session({{id,"only"}}).get();const auto replacement=worker.restore_native_session(cleared).get();check(worker.reading_selection(replacement.tabs[0].tab).get()&&worker.dictionary_selection(replacement.tabs[0].tab).get()&&worker.history(replacement.tabs[0].tab).get().entries.empty(),"Cleared histories restore committed selections without manufacturing entries");
  const auto old=bytes(file);rejects([&]{(void)worker.save_native_session(file.u8string(),{{replacement.tabs[0].tab,"x"},{replacement.tabs[0].tab,"y"}}).get();});check(bytes(file)==old,"Duplicate live tab save leaves bytes unchanged");
  {std::ofstream out(file);out<<"<unrelated keep='yes'/>";}rejects([&]{(void)worker.save_native_session(file.u8string(),{{replacement.tabs[0].tab,"x"}}).get();});check(bytes(file)=="<unrelated keep='yes'/>","Native snapshot refuses to erase unrelated documents");
  auto zero=worker.restore_native_session({}).get();check(zero.tabs.empty()&&!zero.active_tab,"Empty session closes all tabs without fallback");rejects([&]{(void)worker.history(replacement.tabs[0].tab).get();});
 }
 // A new process owner can restore the same saved intent, including missing
 // preference-only modules, but never a missing required selection.
 {StudyWorker worker(root.u8string());auto s=example();auto result=worker.restore_native_session(s).get();check(result.tabs.size()==2,"New owner restores canonical saved state");auto captured=worker.capture_native_session({{result.tabs[0].tab,"study",s.tabs[0].visibility,true,false},{result.tabs[1].tab,"empty"}},"study").get();check(serialize_native_session(captured)==serialize_native_session(s),"Dormant preferences and unknown/empty lexicon IDs retained");}
 // C admission, owned lifetimes and no-allocation commit-result polling.
 xc_worker *worker=nullptr;check(xc_worker_open(root.u8string().c_str(),&worker)==XC_OK,"C worker");xc_request *r=nullptr;check(xc_worker_create_tab(worker,&r)==XC_OK&&wait(r)==XC_OK,"C first tab");const uint64_t old=xc_request_tab(r);check(old!=0,"C old ID");xc_request_release(r);
 const auto xml=serialize_native_session(saved);xc_native_session *input=nullptr;check(xc_native_session_parse(xml.c_str(),&input)==XC_OK,"C offline parse");
 for(int fail:{0,1}){watching=true;fail_after=fail;auto status=xc_worker_restore_native_session(worker,input,nullptr,&r);watching=false;fail_after=-1;check(status!=XC_OK&&!r,"C observer/envelope failure before replacing tabs");check(xc_worker_history(worker,old,&r)==XC_OK&&wait(r)==XC_OK,"C old tab retained on admission failure");xc_request_release(r);}
 xc_session_capabilities cap;xc_session_capabilities_default(&cap);cap.note_numbering=2;check(xc_worker_restore_native_session(worker,input,&cap,&r)==XC_INVALID_ARGUMENT&&!r,"Strict C capability flags");
 check(xc_worker_restore_native_session(worker,input,nullptr,&r)==XC_OK,"C restore queued");xc_native_session_release(input);watching=true;allocations=0;fail_after=0;auto status=wait(r);watching=false;fail_after=-1;check(status==XC_OK&&allocations==0,"Committed restore polling allocates nothing");auto mapping=xc_request_take_restored_session(r);check(mapping&&!xc_request_take_restored_session(r),"Restore mapping take once");xc_request_release(r);
 auto first=xc_restored_session_tab_id(mapping,0),second=xc_restored_session_tab_id(mapping,1);check(first>old&&xc_restored_session_active_tab(mapping)==second,"C fresh IDs/active mapping");xc_tab_visibility visibility;int commentary=0,parallel=0;check(xc_restored_session_visibility(mapping,0,&visibility,&commentary,&parallel)==XC_OK&&!visibility.texts&&visibility.preview,"C host visibility copied");
 check(xc_worker_history(worker,old,&r)==XC_OK&&wait(r)!=XC_OK,"C old ID invalid after commit");xc_request_release(r);
 xc_native_tab_capture rows[2];for(auto &row:rows)xc_native_tab_capture_default(&row);rows[0].tab=first;rows[0].key="book";rows[0].visibility={0,1,0,0};rows[1].tab=second;rows[1].key="study";rows[1].commentary_showing=1;rows[1].parallel_showing=1;
 const auto cfile=root/"c-native.xml";const auto cpath=cfile.u8string();for(int fail:{0,1}){watching=true;fail_after=fail;auto st=xc_worker_save_native_session(worker,cpath.c_str(),rows,2,"study",&r);watching=false;fail_after=-1;check(st!=XC_OK&&!r&&!std::filesystem::exists(cfile),"Save envelope failure has no file effect");}
 check(xc_worker_save_native_session(worker,cpath.c_str(),rows,2,"study",&r)==XC_OK,"C save accepted");rows[1].key="changed";watching=true;allocations=0;fail_after=0;status=wait(r);watching=false;fail_after=-1;check(status==XC_OK&&allocations==0,"Committed save polling allocates nothing");auto snapshot=xc_request_take_native_session(r);check(snapshot&&!xc_request_take_native_session(r),"Snapshot take once");xc_request_release(r);
 rows[1].key="study";const auto drain=root/"drained-native.xml";check(xc_worker_save_native_session(worker,drain.u8string().c_str(),rows,2,"study",&r)==XC_OK,"Dropped save accepted");xc_request_release(r);xc_worker_close(worker);check(bytes(drain)==xml,"Dropped accepted save drains before shutdown");xc_restored_session_release(mapping);
 check(std::string(xc_native_session_tab_key(snapshot,1))=="study"&&std::string(xc_native_session_active_key(snapshot))=="study","Snapshot survives worker and caller input changes");xc_native_session_text *text=nullptr;check(xc_native_session_serialize(snapshot,&text)==XC_OK&&bytes(cfile)==xc_native_session_text_data(text),"C serialization remains owned after source release");xc_native_session_release(snapshot);check(std::string(xc_native_session_text_data(text))==xml,"C saved snapshot matches original complete intent");xc_native_session_text_release(text);
 std::cout<<"Native snapshot codec, strict staging, restart, histories and C ownership passed\n";return 0;
 }catch(const std::exception &e){watching=false;std::cerr<<e.what()<<'\n';return 1;}}
