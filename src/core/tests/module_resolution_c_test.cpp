// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
void check(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
xc_status wait(xc_request *r) {
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
 xc_status status;
 while((status=xc_request_poll(r))==XC_PENDING) { check(std::chrono::steady_clock::now()<deadline,"Resolution poll deadline"); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
 return status;
}
int main(int argc,char **argv) { try {
 check(argc==2,"Generated resolution fixture required"); xc_worker *worker=nullptr; xc_request *request=nullptr;
 check(xc_worker_open(argv[1],&worker)==XC_OK && xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Open isolated resolver owner");
 const auto tab=xc_request_tab(request); xc_request_release(request);
 std::string token="Shared";
 check(xc_worker_resolve_module_name(worker,tab,token.c_str(),&request)==XC_OK && !xc_request_take_module_resolution(request) && !xc_request_take_search_lists(request),"Pending/wrong-kind resolver take guard");
 token="Changed"; check(wait(request)==XC_OK,"Copied resolution input");
 auto *held=xc_request_take_module_resolution(request);check(held&&!xc_request_take_module_resolution(request),"Resolution take-once ownership");xc_request_release(request);
 check(std::string(xc_module_resolution_requested(held))=="Shared" && std::string(xc_module_resolution_module(held))=="BSecond" && xc_module_resolution_via_abbreviation(held),"Owned requested/resolved/alias values");
 auto resolve=[&](const char *text) { check(xc_worker_resolve_module_name(worker,tab,text,&request)==XC_OK,"Queue valid resolution shape");const auto status=wait(request);auto *result=xc_request_take_module_resolution(request);xc_request_release(request);return std::make_pair(status,result); };
 auto exact=resolve("AFirst");check(exact.first==XC_OK&&std::string(xc_module_resolution_module(exact.second))=="AFirst"&&!xc_module_resolution_via_abbreviation(exact.second),"Exact IDs protected");xc_module_resolution_release(exact.second);
 auto unknown=resolve("Missing");check(unknown.first==XC_OK&&unknown.second&&!xc_module_resolution_module(unknown.second)&&!xc_module_resolution_via_abbreviation(unknown.second),"Absent module remains distinct from operation failure");xc_module_resolution_release(unknown.second);
 auto reject=[&](xc_worker *w,const char *text) { request=reinterpret_cast<xc_request *>(1);check(xc_worker_resolve_module_name(w,tab,text,&request)==XC_INVALID_ARGUMENT&&!request,"Rejected shape clears observer"); };
 reject(worker,nullptr);reject(worker,"");reject(nullptr,"Shared");const std::string tooLong(4097,'x');reject(worker,tooLong.c_str());
 check(xc_worker_resolve_module_name(worker,tab,"Shared",nullptr)==XC_INVALID_ARGUMENT,"Null output rejected");
 const auto invalid=resolve("\xff");check(invalid.first==XC_INVALID_ARGUMENT&&!invalid.second,"Malformed UTF-8 rejects at poll without handle");
 const auto maximum=resolve(std::string(4096,'x').c_str());check(maximum.first==XC_OK&&maximum.second&&!xc_module_resolution_module(maximum.second),"Exact input cap accepted");xc_module_resolution_release(maximum.second);
 check(!xc_module_resolution_requested(nullptr)&&!xc_module_resolution_module(nullptr)&&!xc_module_resolution_via_abbreviation(nullptr),"Null result getters");xc_module_resolution_release(nullptr);
 check(xc_worker_close_tab(worker,tab,&request)==XC_OK&&wait(request)==XC_OK,"Close resolver tab");xc_request_release(request);
 const auto closed=resolve("Shared");check(closed.first==XC_INVALID_ARGUMENT&&!closed.second,"Closed-tab resolution rejected");
 check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create draining tab");const auto live=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_resolve_module_name(worker,live,"文字 café",&request)==XC_OK,"Accept lookup before shutdown");xc_worker_close(worker);
 check(wait(request)==XC_OK,"Shutdown drains accepted lookup");auto *drained=xc_request_take_module_resolution(request);xc_request_release(request);
 check(drained&&std::string(xc_module_resolution_module(drained))=="Unicode"&&std::string(xc_module_resolution_module(held))=="BSecond","Copied values survive request/worker/SDK teardown");
 xc_module_resolution_release(held);xc_module_resolution_release(drained);
 std::cout<<"C owned module resolution, bounds, optional values, copied submission and lifetime passed\n";return 0;
 } catch(const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; } }
