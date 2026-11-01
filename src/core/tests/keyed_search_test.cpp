// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <rawld.h>
#include <rawgenbook.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
using namespace xiphos::core;
namespace fs=std::filesystem;
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static void rejects(const std::function<void()> &call){try{call();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected keyed search rejection");}
static xc_status wait(xc_request *r){auto until=std::chrono::steady_clock::now()+std::chrono::seconds(30);xc_status status;while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<until,"Request deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv)try{
 check(argc==3,"Scratch and original library required");fs::path root=argv[1];check(root.filename()=="keyed-search-fixture","Named disposable root only");fs::remove_all(root);fs::copy(fs::path(argv[2]),root,fs::copy_options::recursive);
 {sword::RawGenBook writer((root/"data/CaptureBook/entries").string().c_str());writer.setKey("/Original");writer.setEntry("Original keyed book signal Ω");}
 auto path=root/"data/KeyDictionary";fs::create_directories(path);auto prefix=(path/"entries").string();check(sword::RawLD::createModule(prefix.c_str())==0,"Create dictionary");{sword::RawLD writer(prefix.c_str());writer.setKey("John.3.16");writer.setEntry("Original dictionary signal Ω");writer.setKey("alpha");writer.setEntry("Original second signal");}
 {std::ofstream out(root/"mods.d/KeyDictionary.conf");out<<"[KeyDictionary]\nDataPath=./data/KeyDictionary/entries\nModDrv=RawLD\nSourceType=Plain\nEncoding=UTF-8\nDescription=Original keyed fixture\n";}
 {
 Reader reader(root.string());StudySession session(reader);auto initial=session.search_module({"Text","Original",SearchMode::phrase});auto scope=session.last_search_scope();auto generation=session.search_generation();
 for(auto mode:{SearchMode::phrase,SearchMode::regular_expression,SearchMode::words,SearchMode::multiword}){auto result=reader.search_keyed_module({"KeyDictionary","signal",mode});check(result.executed&&result.key_kind==SearchKeyKind::dictionary&&result.source_module()=="KeyDictionary"&&result.references.size()==2,"Dictionary text modes return typed opaque keys");for(const auto &key:result.references)check(reader.read_dictionary("KeyDictionary",key).html.find("signal")!=std::string::npos,"Each dictionary result reads its actual entry");}
 auto prefix_result=reader.search_keyed_module({"KeyDictionary","signal",SearchMode::phrase},{1,4096});check(prefix_result.limited&&prefix_result.references.size()==1&&prefix_result.total_results==2,"Copied prefix limits preserve total count");
 auto book=reader.search_keyed_module({"CaptureBook","Original",SearchMode::phrase});check(book.key_kind==SearchKeyKind::general_book&&book.references==std::vector<std::string>{"/Original"},"General book returns exact path, not scripture");auto tree=reader.book_tree_path("CaptureBook",book.references[0]);check(reader.read_book("CaptureBook",tree.node.offset).html.find("keyed book signal")!=std::string::npos,"Book search path resolves actual matching content");
 SearchQuery bad{"KeyDictionary","signal",SearchMode::phrase};bad.range=SearchRange{"Text","John3:16"};rejects([&]{reader.search_keyed_module(bad);});bad.range.reset();bad.within_last_results=true;rejects([&]{reader.search_keyed_module(bad);});for(auto mode:{SearchMode::indexed,SearchMode::lemma,SearchMode::morphology,SearchMode::footnote})rejects([&]{reader.search_keyed_module({"KeyDictionary","signal",mode});});rejects([&]{reader.search_keyed_module({"Text","Original",SearchMode::phrase});});
 auto control=std::make_shared<SearchControl>();control->request_cancel();auto cancelled=reader.search_keyed_module({"KeyDictionary","signal",SearchMode::phrase},{},control);check(cancelled.cancelled&&!cancelled.executed&&cancelled.key_kind==SearchKeyKind::dictionary&&control->phase()==SearchPhase::cancelled,"Pre-execution cancellation preserves typed source");check(session.last_search_scope().element_count==scope.element_count&&session.search_generation().id==generation.id,"Keyed queries leave verse scope/generation unchanged");
 }
 xc_worker *worker=nullptr;check(xc_worker_open(root.string().c_str(),&worker)==XC_OK,"C worker");xc_request *request=nullptr;check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C tab");auto tab=xc_request_tab(request);xc_request_release(request);std::string query="signal";check(xc_worker_search_keyed_module(worker,tab,"KeyDictionary",query.c_str(),nullptr,&request)==XC_OK,"C keyed admission");query="changed";check(wait(request)==XC_OK,"C keyed result");auto *result=xc_request_take_search(request);check(result&&!xc_request_take_search(request),"Take once");xc_request_release(request);xc_worker_close(worker);xc_search_info info{};check(xc_search_get_info(result,&info)==XC_OK&&info.result_count==2&&xc_search_key_kind(result)==1&&std::string(xc_search_module(result))=="KeyDictionary","Typed C copy survives request and worker");check(xc_search_reference(result,0)&&xc_search_key_kind(nullptr)==-1,"Owned bounded key access");xc_search_release(result);
 std::cout<<"Typed dictionary/general-book text modes, source identities, limits, rejects, cancellation and C ownership passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
