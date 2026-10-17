// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/engine_manager.hpp"
#include <versekey.h>
#include <markupfiltmgr.h>
#include <listkey.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
using namespace xiphos::core;
using namespace sword;
namespace fs=std::filesystem;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected search range error");}
void main_dialog_search_percent_update(char,void *){}
void main_sidebar_search_percent_update(char,void *){}
struct BackEnd {
 EngineManager *main_mgr;SWModule *search_mod=nullptr;ListKey results,search_scope_list,search_range;SWKey *current_scope=nullptr;
 SWModule *get_SWModule(const char *name){return main_mgr->getModule(name);}
 int set_range(const char *,const char *);void set_scope2range();
 int do_module_search(char *,const char *,int,int,int);
};
#include "legacy_search_range_actual.inc"
#include "legacy_search_backend_actual.inc"
std::vector<std::string> refs(ListKey &list){std::vector<std::string> out;list.setPosition(TOP);while(list.getCount()&&!list.popError()){out.emplace_back(list.getText());list++;}return out;}
xc_status wait(xc_request *r){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Range request deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==3,"Source and private range fixtures required");const fs::path source=argv[1],root=argv[2];fs::create_directories(root);
 fs::copy(source/"mods.d",root/"mods.d",fs::copy_options::recursive|fs::copy_options::overwrite_existing);fs::copy(source/"data",root/"data",fs::copy_options::recursive|fs::copy_options::overwrite_existing);
 {std::ofstream conf(root/"mods.d/Alternate.conf");conf<<"[Alternate]\nDataPath=./data/text/\nModDrv=RawText\nSourceType=OSIS\nEncoding=UTF-8\nVersification=Vulg\n";}fs::remove(root/"mods.d/modules-conf.cache");
 SearchResult held;
 {
  Reader reader(root.string());EngineManager manager(root.string().c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);BackEnd backend{&manager};
  for(const auto *target:{"Text","Commentary"})for(const auto *scope:{"Text","Alternate"})for(const auto *expression:{"John1:2-John1:3","John1:1;John1:3","Gen-Rev","unknown-original-bounds"}){
   SearchQuery query{target,"Original"};query.range=SearchRange{scope,expression};held=reader.search_module(query);
   check(backend.set_range(scope,expression)>=0,"Actual legacy range parser completes");backend.set_scope2range();
   const auto stripped=manager.getModule(target)->stripText("Original");check(backend.do_module_search(const_cast<char *>(target),stripped.c_str(),-1,XIPHOS_SWORD_SEARCH_ICASE_FLAG,0)>=0,"Actual bounded backend search completes");
   check(held.references==refs(backend.results)&&held.requested.range->module==scope&&held.requested.range->expression==expression,"Copied scoped search matches actual legacy range/target module semantics, including alternate versification");
   check(reader.read("Text","John1:1").html.find("Original Alpha")!=std::string::npos,"Scoped search restores target module reading behavior");
  }
  SearchQuery range{"Text","Original"};range.range=SearchRange{"Text","John1:2-John1:3"};held=reader.search_module(range);check(held.references.size()==2&&held.total_results==2,"Explicit bounds restrict the actual matches");
  SearchLimits one;one.max_results=1;auto prefix=reader.search_module(range,one);check(prefix.references.size()==1&&prefix.limited&&prefix.total_results==2,"Output bounds apply after interval search");
  auto cancelled=std::make_shared<SearchControl>();cancelled->request_cancel();
  const auto stopped=reader.search_module(range,{},cancelled);check(stopped.cancelled&&!stopped.executed&&stopped.requested.range->expression=="John1:2-John1:3","Cancelled range preserves owned scope without executing search");
  range.range=SearchRange{"Text","Gen-Rev"};check(reader.search_module(range).references.size()==3,"Whole-Bible interval is not truncated by the preview's 2000-verse expansion guard");
  for(const auto &bad:std::vector<SearchRange>{{"Missing","John1:1"},{"Dictionary","John1:1"},{"Text",""},{"Text",std::string("bad\0range",9)},{"Text",std::string(4097,'a')},{"Text",std::string("\xff",1)}}){range.range=bad;if(bad.module=="Missing")rejects<std::out_of_range>([&]{reader.search_module(range);});else rejects<std::invalid_argument>([&]{reader.search_module(range);});}
 }
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.string().c_str(),&worker)==XC_OK,"Open C range worker");check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create C range tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 std::string bounds="John1:2-John1:3";check(xc_worker_search_module_in_range(worker,tab,"Text","Original","Text",bounds.c_str(),nullptr,&request)==XC_OK,"Queue scoped search");bounds="unrelated";check(wait(request)==XC_OK,"Complete copied range");auto *result=xc_request_take_search(request);xc_request_release(request);xc_search_info info{};
 check(result&&xc_search_get_info(result,&info)==XC_OK&&info.result_count==2&&std::string(xc_search_range_module(result))=="Text"&&std::string(xc_search_range_expression(result))=="John1:2-John1:3","C owns explicit scope metadata after caller mutation");
 check(xc_worker_search_module_in_range(worker,tab,"Text","Original",nullptr,"John1:1",nullptr,&request)==XC_INVALID_ARGUMENT&&!request&&xc_worker_search_module_in_range(worker,tab,"Text","Original","Text",nullptr,nullptr,&request)==XC_INVALID_ARGUMENT&&!request,"NULL range inputs clear request");
 check(xc_worker_search_module(worker,tab,"Text","Original",nullptr,&request)==XC_OK&&wait(request)==XC_OK,"Unscoped search stays independent of previous bounds");auto *whole=xc_request_take_search(request);xc_request_release(request);check(xc_search_get_info(whole,&info)==XC_OK&&info.result_count==3&&!xc_search_range_module(whole)&&!xc_search_range_expression(whole),"Scope does not leak to later whole-module search");xc_search_release(whole);xc_worker_close(worker);
 check(std::string(xc_search_range_expression(result))=="John1:2-John1:3"&&!xc_search_range_module(nullptr)&&!xc_search_range_expression(nullptr),"Scope metadata survives worker close; NULL getters are safe");xc_search_release(result);
 std::cout<<"Owned SDK interval search matches legacy bounds, v11n sources, output limits and C lifetime passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
