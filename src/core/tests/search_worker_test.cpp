// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
xc_status wait(xc_request *request){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Search worker deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Original search fixture required");const std::string root=argv[1];
 {
  Reader reader(root);StudySession first(reader),other(reader);const auto passage=first.visit_passage("Text","Commentary","John1:2");const auto book=first.select_book_document({"Book",std::uint64_t(4)});
  const auto result=first.search_module({"Text","Original"});
  check(result.references.size()==3&&first.history().entries().size()==1&&first.history().entries().front().reference==passage.text.reference&&first.dictionary_history().entries().empty()&&first.book_selection()->document->html==book.html&&other.history().entries().empty()&&!other.book_selection(),"Read-only search preserves reading/history/book state across tabs");
 }
 std::future<SearchResult> drained;
 {StudyWorker worker(root);const auto tab=worker.create_tab().get();auto visit=worker.visit_passage(tab,"Text","Commentary","John1:2");auto selected=worker.select_book_document(tab,{"Book",std::uint64_t(4)});SearchLimits one;one.max_results=1;drained=worker.search_module(tab,{"Text","Original"},one);auto history=worker.history(tab);auto book=worker.book_document_selection(tab);check(visit.get().text.reference=="John 1:2"&&history.get().entries.size()==1&&book.get()->html==selected.get().html,"FIFO search does not replace earlier committed study selection");}
 const auto owned=drained.get();check(owned.limited&&owned.references.size()==1&&owned.total_results==3,"Queued search data survives worker close");
 xc_search_options options;xc_search_options_default(&options);xc_search_options_default(nullptr);check(options.mode==XC_SEARCH_PHRASE&&!options.case_sensitive&&options.max_results&&options.max_reference_bytes,"C search defaults");options.max_results=1;
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.c_str(),&worker)==XC_OK,"Open C search worker");check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create C search tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 std::string text="Original";check(xc_worker_search_module(worker,tab,"Text",text.c_str(),&options,&request)==XC_OK&&!xc_request_take_search(request),"Pending search does not expose data");text="unrelated";options.max_results=50;
 check(wait(request)==XC_OK&&!xc_request_take_entry(request),"Complete C search has distinct payload");auto *result=xc_request_take_search(request);check(result&&!xc_request_take_search(request),"Search payload transfers once");xc_request_release(request);xc_search_info info{};
 check(xc_search_get_info(result,&info)==XC_OK&&info.mode==XC_SEARCH_PHRASE&&info.engine_type==-1&&!info.case_sensitive&&info.limited&&info.total_results==3&&info.result_count==1&&std::string(xc_search_expression(result))=="Original"&&std::string(xc_search_reference(result,0))==owned.references.front(),"C request copies mutable options/query and owns complete result metadata");
 for(const auto *module:{"Missing","Dictionary","Book"}){check(xc_worker_search_module(worker,tab,module,"Original",nullptr,&request)==XC_OK&&wait(request)==(std::string(module)=="Missing"?XC_MODULE_NOT_FOUND:XC_INVALID_ARGUMENT)&&!xc_request_take_search(request),"Unsupported/missing search source fails without partial output");xc_request_release(request);}
 for(int kind:{-1,6}){options.mode=kind;request=reinterpret_cast<xc_request *>(1);check(xc_worker_search_module(worker,tab,"Text","Original",&options,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid mode clears request output");}xc_search_options_default(&options);options.case_sensitive=2;check(xc_worker_search_module(worker,tab,"Text","Original",&options,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid case boolean rejects");xc_search_options_default(&options);options.max_results=0;check(xc_worker_search_module(worker,tab,"Text","Original",&options,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid result limit rejects");
 check(xc_worker_search_module(worker,tab,nullptr,"Original",nullptr,&request)==XC_INVALID_ARGUMENT&&!request&&xc_worker_search_module(worker,tab,"Text",nullptr,nullptr,&request)==XC_INVALID_ARGUMENT&&!request,"NULL input rejects");
 info={9,9,9,9,9,9};check(xc_search_get_info(nullptr,&info)==XC_INVALID_ARGUMENT&&!info.result_count&&!info.total_results&&xc_search_get_info(result,nullptr)==XC_INVALID_ARGUMENT&&!xc_search_module(nullptr)&&!xc_search_reference(result,999)&&!xc_search_expression(nullptr)&&!xc_search_effective_expression(nullptr),"Invalid search getters clear optional output");xc_search_release(nullptr);
 check(xc_worker_search_module(worker,tab,"Text","café",nullptr,&request)==XC_OK,"Queue UTF-8 search before close");xc_worker_close(worker);check(wait(request)==XC_OK,"Close drains accepted search");auto *last=xc_request_take_search(request);xc_request_release(request);check(last&&std::string(xc_search_expression(last))=="café"&&xc_search_get_info(last,&info)==XC_OK&&info.result_count==1&&std::string(xc_search_module(result))=="Text"&&std::string(xc_search_expression(result))=="Original","Borrowed search strings own data after later searches and worker destruction");xc_search_release(last);xc_search_release(result);
 std::cout<<"Queued owned search, copied C requests, state neutrality and shutdown passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
