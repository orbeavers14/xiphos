// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
xc_status wait(xc_request *r) {
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);xc_status status;
 while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<deadline,"Save observer deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;
}
std::string bytes(const std::filesystem::path &path) { std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}}; }
void write(const std::filesystem::path &path,const std::string &value) { std::ofstream out(path,std::ios::binary);out<<value;check(bool(out),"Write original-only fixture"); }
int main(int argc,char **argv){try{
 check(argc==2,"Disposable C save root required");const auto root=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(root);
 const auto file=root/std::filesystem::u8path("profile-café.xml"),fresh=root/"new.xml";
 const std::string original="<Xiphos><!--keep--><other custom='yes'>α café</other><modlists><modlist label='Original' list='Old' keep='yes'/><modlist label='Original' list='Shadowed'/></modlists><ranges><range label='Bounds' list='John1:2'/></ranges></Xiphos>";write(file,original);std::filesystem::remove(fresh);
 xc_worker *worker=nullptr;xc_request *request=nullptr;
 check(xc_worker_open(root.u8string().c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create isolated worker/tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 std::string path=file.u8string(),label="Original",value="Text, MissingModule,Text";
 xc_saved_search_list_edit edit{XC_SAVED_SEARCH_MODULES,label.c_str(),value.c_str()};
 check(xc_worker_save_search_list(worker,tab,path.c_str(),&edit,&request)==XC_OK&&!xc_request_take_search_lists(request)&&!xc_request_take_search_module_names(request),"Pending preallocated result cannot be taken before poll");
 path="Changed";label="Changed";value="Changed";edit.kind=99;
 check(wait(request)==XC_OK,"Accepted save uses copied path/edit/strings");auto *owned=xc_request_take_search_lists(request);
 check(owned&&!xc_request_take_search_lists(request)&&!xc_request_take_search_module_names(request),"Save result is take-once and not module-name transport");xc_request_release(request);
 check(xc_search_lists_count(owned,0)==2&&std::string(xc_search_lists_value(owned,0,0))=="Text, MissingModule,Text"&&std::string(xc_search_lists_value(owned,0,1))=="Shadowed","Prepared owned snapshot preserves duplicate shadows and literal values");
 const auto saved=bytes(file);check(saved.find("keep=\"yes\"")!=std::string::npos&&saved.find("<!--keep-->")!=std::string::npos&&saved.find("α café")!=std::string::npos&&saved.find("John1:2")!=std::string::npos,"Unrelated XML survives the C write");
 path=file.u8string();edit={XC_SAVED_SEARCH_RANGES,"café α"," John1:1-2 "};
 auto run=[&]{check(xc_worker_save_search_list(worker,tab,path.c_str(),&edit,&request)==XC_OK,"Queue valid C save shape");const auto status=wait(request);auto *result=xc_request_take_search_lists(request);xc_request_release(request);return std::make_pair(status,result);};
 auto range=run();check(range.first==XC_OK&&range.second&&xc_search_lists_count(range.second,1)==2&&std::string(xc_search_lists_value(range.second,1,1))==" John1:1-2 ","Range kind and Unicode values round trip");xc_search_lists_release(range.second);
 edit={XC_SAVED_SEARCH_MODULES,"",""};auto empty=run();check(empty.first==XC_OK&&std::string(xc_search_lists_label(empty.second,0,2)).empty()&&std::string(xc_search_lists_value(empty.second,0,2)).empty(),"Explicit empty attributes are valid");xc_search_lists_release(empty.second);
 const auto before=bytes(file);
 auto rejects=[&](const char *filePath,const xc_saved_search_list_edit *input){request=reinterpret_cast<xc_request *>(1);check(xc_worker_save_search_list(worker,tab,filePath,input,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid shape clears observer before accepting mutation");check(bytes(file)==before,"Rejected submission preserves original file");};
 rejects(nullptr,&edit);rejects("",&edit);rejects(path.c_str(),nullptr);edit.kind=2;rejects(path.c_str(),&edit);edit.kind=0;
 edit.label=nullptr;rejects(path.c_str(),&edit);edit.label="";edit.value=nullptr;rejects(path.c_str(),&edit);edit.value="";
 const std::string oversized(16*1024*1024+1,'x');rejects(oversized.c_str(),&edit);edit.label=oversized.c_str();rejects(path.c_str(),&edit);edit.label="";edit.value=oversized.c_str();rejects(path.c_str(),&edit);edit.value="";
 check(xc_worker_save_search_list(nullptr,tab,path.c_str(),&edit,&request)==XC_INVALID_ARGUMENT&&!request&&xc_worker_save_search_list(worker,tab,path.c_str(),&edit,nullptr)==XC_INVALID_ARGUMENT,"NULL worker/output rejected");
 for(const auto &bad:{std::string(1,char(0xff)),std::string(1,'\x01')}){
  edit.label=bad.c_str();auto failed=run();check(failed.first==XC_INVALID_ARGUMENT&&!failed.second&&bytes(file)==before,"Semantic invalid UTF-8/XML field fails on owner without replacement");
 }edit.label="";
 const auto malformed=root/"malformed.xml";write(malformed,"<Xiphos>");path=malformed.u8string();auto broken=run();check(broken.first!=XC_OK&&!broken.second&&bytes(malformed)=="<Xiphos>","Malformed selected profile remains intact");
 path=(root/"missing-parent/profile.xml").u8string();auto absent=run();check(absent.first!=XC_OK&&!absent.second&&!std::filesystem::exists(root/"missing-parent"),"Missing parent is not silently created");
 path=fresh.u8string();edit={XC_SAVED_SEARCH_RANGES,"Fresh","John1:2"};auto created=run();check(created.first==XC_OK&&std::string(xc_search_lists_value(created.second,1,0))=="John1:2","New explicit file returns its staged snapshot");xc_search_lists_release(created.second);
#ifndef _WIN32
 check(std::filesystem::status(fresh).permissions()==(std::filesystem::perms::owner_read|std::filesystem::perms::owner_write),"New saved profile permissions remain private");
#endif
 // Removal shares the preallocated snapshot channel but never creates a profile.
 std::string removalPath=fresh.u8string(),removalLabel="Fresh";
 check(xc_worker_remove_search_list(worker,tab,removalPath.c_str(),1,removalLabel.c_str(),&request)==XC_OK&&!xc_request_take_search_lists(request),"Queue removal with pre-poll guard");
 removalPath.clear();removalLabel="Changed";
 check(wait(request)==XC_OK,"Removal copies path and label");auto *removed=xc_request_take_search_lists(request);
 check(removed&&xc_search_lists_count(removed,1)==0&&!xc_request_take_search_lists(request),"Verified removal snapshot is owned and take-once");xc_request_release(request);
 auto rejectRemoval=[&](const char *p,int kind,const char *name){request=reinterpret_cast<xc_request *>(1);check(xc_worker_remove_search_list(worker,tab,p,kind,name,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid removal shape clears output");};
 rejectRemoval(nullptr,0,"");rejectRemoval("",0,"");rejectRemoval(path.c_str(),2,"");rejectRemoval(path.c_str(),0,nullptr);rejectRemoval(path.c_str(),0,oversized.c_str());rejectRemoval(oversized.c_str(),0,"");
 check(xc_worker_remove_search_list(nullptr,tab,path.c_str(),0,"",&request)==XC_INVALID_ARGUMENT&&!request&&xc_worker_remove_search_list(worker,tab,path.c_str(),0,"",nullptr)==XC_INVALID_ARGUMENT,"NULL removal worker/output rejected");
 check(xc_worker_remove_search_list(worker,tab,path.c_str(),0,"\x01",&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_search_lists(request)&&bytes(file)==before,"Malformed XML label fails before replacement");xc_request_release(request);
 check(xc_worker_remove_search_list(worker,tab,(root/"absent.xml").u8string().c_str(),0,"Missing",&request)==XC_OK&&wait(request)!=XC_OK&&!xc_request_take_search_lists(request)&&!std::filesystem::exists(root/"absent.xml"),"Missing removal profile remains absent");xc_request_release(request);
 xc_search_lists_release(removed);
 check(xc_worker_close_tab(worker,tab,&request)==XC_OK&&wait(request)==XC_OK,"Close save tab");xc_request_release(request);path=file.u8string();auto closed=run();check(closed.first==XC_INVALID_ARGUMENT&&!closed.second&&bytes(file)==before,"Closed tab rejects before persistent write");
 check(xc_worker_remove_search_list(worker,tab,path.c_str(),0,"Original",&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_search_lists(request)&&bytes(file)==before,"Closed removal tab preserves file");xc_request_release(request);
 check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create draining tab");const auto live=xc_request_tab(request);xc_request_release(request);
 edit={XC_SAVED_SEARCH_MODULES,"Detached","Text"};check(xc_worker_save_search_list(worker,live,path.c_str(),&edit,&request)==XC_OK,"Accept detached observer write");xc_request_release(request);
 edit={XC_SAVED_SEARCH_RANGES,"Drain","John1:3"};check(xc_worker_save_search_list(worker,live,path.c_str(),&edit,&request)==XC_OK,"Accept save before shutdown");xc_worker_close(worker);
 check(wait(request)==XC_OK,"Shutdown drains accepted writes");auto *drained=xc_request_take_search_lists(request);xc_request_release(request);
 check(drained&&xc_search_lists_count(drained,0)==4&&xc_search_lists_count(drained,1)==3,"Returned stage includes preceding FIFO detached mutation");
 check(xc_worker_open(root.u8string().c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create removal drain owner");const auto removalTab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_remove_search_list(worker,removalTab,path.c_str(),0,"Original",&request)==XC_OK,"Accept detached removal");xc_request_release(request);
 check(xc_worker_remove_search_list(worker,removalTab,path.c_str(),0,"Detached",&request)==XC_OK,"Accept shutdown removal");xc_worker_close(worker);
 check(wait(request)==XC_OK,"Shutdown drains removals");auto *removalDrained=xc_request_take_search_lists(request);xc_request_release(request);
 check(removalDrained&&xc_search_lists_count(removalDrained,0)==2&&std::string(xc_search_lists_value(removalDrained,0,0))=="Shadowed","FIFO removals expose later duplicate without invalidating prior snapshots");
 std::filesystem::remove(file);std::filesystem::remove(fresh);std::filesystem::remove(malformed);
 check(std::string(xc_search_lists_value(owned,0,0))=="Text, MissingModule,Text"&&std::string(xc_search_lists_value(drained,1,2))=="John1:3","Owned staged values survive request/worker/source teardown");
 xc_search_lists_release(removalDrained);
 xc_search_lists_release(owned);xc_search_lists_release(drained);xc_search_lists_release(nullptr);
 std::cout<<"C verified saved-list writes, copied submission, strict bounds, failure preservation and owned stage lifetime passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
