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
#include <swmgr.h>
#include <swmodule.h>
#include <versekey.h>
#include <markupfiltmgr.h>
using namespace sword;
#ifdef _WIN32
#define strdup _strdup
#endif
namespace frozen {
struct BackEnd{SWMgr *main_mgr;const char *get_osisref_from_key(const char *,const char *);};
#include "legacy_annotation_target.inc"
}
namespace actual {
struct BackEnd{SWMgr *main_mgr;const char *get_osisref_from_key(const char *,const char *);};
#include "legacy_annotation_target_actual.inc"
}
template<class E,class F>static void rejection(F call){try{call();}catch(const E &){return;}throw std::runtime_error("Expected annotation target rejection");}
int main(int argc,char **argv){try{
 check(argc==2,"Original installed navigation fixture required");const std::string root=argv[1];std::size_t comparisons=0;AnnotationTarget held;
 {Reader reader(root);SWMgr baseline(root.c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);frozen::BackEnd old{&baseline};actual::BackEnd current{&baseline};
  for(const auto *module:{"Text","TextAlt","Notes","Personal"})for(const auto *reference:{"Gen1:1","John3:16","John3:17","Ps119:176","Jude1:1"})for(bool generic:{false,true}){
   const std::unique_ptr<char,decltype(&std::free)> expected(const_cast<char *>(old.get_osisref_from_key(module,reference)),std::free),actual(const_cast<char *>(current.get_osisref_from_key(module,reference)),std::free);check(expected&&actual&&std::string(expected.get())==actual.get(),"Complete frozen/current SDK OSIS target helper");
   const auto before=reader.navigation_target(module,"3:16").reference;held=reader.annotation_target(module,reference,generic);check(held.label==(generic?std::string():std::string(module))+" "+expected.get()&&held.control_module==module&&held.address.module==(generic?"":module),"Owned module-sensitive generic/specific annotation target matches complete retained SDK OSIS helper");
   check(reader.navigation_target(module,"3:16").reference==before&&held.address.chapter>0&&held.address.verse>0&&decode_annotation_label(held.label)->packed==held.address.packed,"Owned annotation target preserves reading key and checked coordinates");++comparisons;
  }
  const auto apocrypha=reader.annotation_target("TextAlt","Tob1:1",true);check(apocrypha.address.book=="Tob"&&apocrypha.label==" Tob.1.1","Generic note uses explicit KJVA control canon, without KJV/first-module fallback");
  for(const auto *module:{"Missing","T"})rejection<std::out_of_range>([&]{reader.annotation_target(module,"John3:16");});for(const auto *module:{"Dictionary","Daily"})rejection<std::invalid_argument>([&]{reader.annotation_target(module,"John3:16");});
  rejection<std::invalid_argument>([&]{reader.annotation_target("Text","");});rejection<std::invalid_argument>([&]{reader.annotation_target("Text",std::string("John3:16\0tail",13));});
  rejection<std::invalid_argument>([]{annotation_label({"Module With Spaces","Gen",1,1,1001});});check(annotation_label({"","Gen",1,1,1001})==" Gen.1.1","Generic formatter preserves leading separator without truncation");
 }
 check(!held.label.empty()&&!held.reference.empty(),"Owned annotation target survives SDK shutdown");
 const auto file=(std::filesystem::u8path(root)/"verse-note-targets.xml").u8string();std::filesystem::remove(std::filesystem::u8path(file));
 {StudyWorker worker(root);const auto tab=worker.create_tab().get(),sibling=worker.create_tab().get();
  EngineOptions options;options.strongs=true;worker.set_engine_options(tab,options).get();std::string path=file,module="Text",reference="John3:16",note="Original verse note";
  auto save=worker.edit_verse_annotation(tab,path,module,reference,false,AnnotationAction::save,note);path.clear();module.clear();reference.clear();note.clear();auto cache=worker.read_verse_annotation_cache(tab,file,"Text","John3:17");check(save.get().records[0].label=="Text John.3.16"&&annotation_cache_index(cache.get(),3,16)==0,"Copied SDK-target save and FIFO book cache from a reading reference");
  (void)worker.edit_verse_annotation(tab,file,"TextAlt","John3:17",true,AnnotationAction::save,"Generic").get();check(worker.read_verse_annotation_cache(tab,file,"Notes","John3:17").get().entries.size()==1,"Generic note applies across explicit verse control modules");(void)worker.edit_verse_annotation(tab,file,"Notes","John3:17",true,AnnotationAction::remove).get();
  check(worker.history(tab).get().entries.empty()&&worker.history(sibling).get().entries.empty()&&!worker.parallel_selection(tab).get()&&!worker.search_generation(tab).get().id&&worker.module_engine_options(tab,"Text").get()->strongs,"Verse-note targets/edits preserve history/search/options/sibling selection");
  rejected(worker.annotation_target(tab,"Text",std::string(32769,'x')),true);rejected(worker.edit_verse_annotation(tab,file,"Text","John3:16",false,AnnotationAction::save,std::string(1,'\1')),true);
  const auto absent=(std::filesystem::u8path(root)/"closed-note-target.xml").u8string();std::filesystem::remove(std::filesystem::u8path(absent));worker.close_tab(tab).get();rejected(worker.edit_verse_annotation(tab,absent,"Text","John3:16",false,AnnotationAction::save,"Closed"));check(!std::filesystem::exists(std::filesystem::u8path(absent)),"Closed tab fails before target/file effects");
  worker.edit_verse_annotation(sibling,file,"Text","John3:18",false,AnnotationAction::save,"Drained");
 }
 check(read_annotations(file).records.size()==2,"Dropped SDK-target write drains");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C annotation target owner/tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 const std::string too_long(32769,'x');allocations=0;watching=true;auto status=xc_worker_annotation_target(worker,tab,"Text",too_long.c_str(),0,&request);watching=false;check(status==XC_INVALID_ARGUMENT&&!request&&!allocations,"Target reference bound rejects before observer allocation");
 check(xc_worker_annotation_target(worker,tab,"Text","John3:16",2,&request)==XC_INVALID_ARGUMENT&&!request,"C generic bool is strict");std::string module="TextAlt",reference="Tob1:1";check(xc_worker_annotation_target(worker,tab,module.c_str(),reference.c_str(),1,&request)==XC_OK,"C copied target accepted");module.clear();reference.clear();allocations=0;watching=true;status=wait(request);watching=false;check(status==XC_OK&&!allocations,"Owned SDK target publication allocates nothing");auto *target=xc_request_take_annotation_target(request);check(target&&!xc_request_take_annotation_target(request)&&!xc_request_take_annotations(request),"Distinct take-once target handle");xc_request_release(request);
 check(std::string(xc_annotation_target_control_module(target))=="TextAlt"&&std::string(xc_annotation_target_label(target))==" Tob.1.1"&&std::string(xc_annotation_target_book(target))=="Tob"&&std::string(xc_annotation_target_stored_module(target)).empty()&&xc_annotation_target_generic(target)&&xc_annotation_target_chapter(target)==1&&xc_annotation_target_verse(target)==1&&xc_annotation_target_packed(target)==1001&&xc_annotation_target_reference(target),"Owned generic KJVA target metadata");
 std::string copied_module="Text",copied_ref="John3:19",copied_note="Original & <>文字";xc_verse_annotation_edit edit{XC_ANNOTATION_SAVE,0,copied_module.c_str(),copied_ref.c_str(),copied_note.c_str()};
 for(int failure:{0,1}){watching=true;fail_after=failure;status=xc_worker_edit_verse_annotation(worker,tab,file.c_str(),&edit,&request);watching=false;fail_after=-1;check(status==XC_ENGINE_ERROR&&!request&&read_annotations(file).records.size()==2,"Verse-note observer/envelope failure cannot accept a write");}
 check(xc_worker_edit_verse_annotation(worker,tab,file.c_str(),&edit,&request)==XC_OK,"C copied verse-note save accepted");copied_module.clear();copied_ref.clear();copied_note.clear();allocations=0;watching=true;status=wait(request);watching=false;check(status==XC_OK&&!allocations,"SDK-target persistent result polling allocates nothing");auto *saved=xc_request_take_annotations(request);xc_request_release(request);check(xc_annotations_count(saved)==3&&std::string(xc_annotations_field(saved,2,XC_ANNOTATION_LABEL))=="Text John.3.19","C source label encoding stays in core");
 check(xc_worker_read_verse_annotation_cache(worker,tab,file.c_str(),"Text","John3:19",&request)==XC_OK&&wait(request)==XC_OK,"C cache derives book from SDK reference");auto *cache=xc_request_take_annotation_cache(request);xc_request_release(request);int found=0;size_t index=0;check(xc_annotation_cache_find(cache,3,19,&found,&index)==XC_OK&&found&&std::string(xc_annotation_cache_html(cache,index))=="Original &amp; &lt;&gt;文字","Core-derived verse cache retains escaped note");
 edit={XC_ANNOTATION_REMOVE,0,"Text","John3:19",nullptr};check(xc_worker_edit_verse_annotation(worker,tab,file.c_str(),&edit,&request)==XC_OK&&wait(request)==XC_OK,"C normalized verse-note removal");auto *removed=xc_request_take_annotations(request);xc_request_release(request);check(xc_annotations_count(removed)==2,"Canonical removal preserves other notes");xc_annotations_release(removed);
 edit={XC_ANNOTATION_SAVE,0,"Missing","John3:20","Missing"};check(xc_worker_edit_verse_annotation(worker,tab,file.c_str(),&edit,&request)==XC_OK&&wait(request)==XC_MODULE_NOT_FOUND,"Missing target never falls back to another module");xc_request_release(request);check(read_annotations(file).records.size()==2,"Failed target preserves profile");
 edit={XC_ANNOTATION_SAVE,1,"Text","John3:20","DrainC"};check(xc_worker_edit_verse_annotation(worker,tab,file.c_str(),&edit,&request)==XC_OK,"C dropped canonical save accepted");xc_request_release(request);check(xc_worker_read_verse_annotation_cache(worker,tab,file.c_str(),"Text","John3:20",&request)==XC_OK,"C cache queues behind normalized save");xc_worker_close(worker);check(wait(request)==XC_OK,"Canonical note/cache operations drain on shutdown");auto *drained=xc_request_take_annotation_cache(request);xc_request_release(request);check(xc_annotation_cache_find(drained,3,20,&found,&index)==XC_OK&&found&&std::string(xc_annotation_cache_html(drained,index))=="DrainC","Dropped canonical save persists before queued cache");xc_annotation_cache_release(drained);
 std::filesystem::remove(std::filesystem::u8path(file));check(std::string(xc_annotation_target_label(target))==" Tob.1.1"&&std::string(xc_annotations_field(saved,2,XC_ANNOTATION_NOTE))=="Original & <>文字"&&std::string(xc_annotation_cache_html(cache,2))=="Original &amp; &lt;&gt;文字","Owned target/profile/cache survive source and worker teardown");xc_annotation_target_release(target);xc_annotations_release(saved);xc_annotation_cache_release(cache);
 check(!xc_annotation_target_control_module(nullptr)&&!xc_annotation_target_reference(nullptr)&&!xc_annotation_target_label(nullptr)&&!xc_annotation_target_stored_module(nullptr)&&!xc_annotation_target_book(nullptr)&&!xc_annotation_target_chapter(nullptr)&&!xc_annotation_target_verse(nullptr)&&!xc_annotation_target_packed(nullptr)&&!xc_annotation_target_generic(nullptr),"Safe null target getters");xc_annotation_target_release(nullptr);
 std::cout<<"Annotation targets: "<<comparisons<<" complete retained SDK comparisons, generic control canon, core-owned labels/cache, FIFO/C persistence and ownership passed\n";return 0;
}catch(const std::exception &error){watching=false;std::cerr<<error.what()<<'\n';return 1;}}
