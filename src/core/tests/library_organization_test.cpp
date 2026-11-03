// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/library_organization.hpp"
#include "xiphos/core/editable_document.hpp"
#include "xiphos/core/c_api.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace xiphos::core;
static void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>static void rejects(F call){try{call();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected organization rejection");}
int main(int argc,char **argv)try{
 check(argc==2,"Disposable path required");std::filesystem::path root=argv[1];check(root.filename()=="library-organization-fixture","Named fixture only");std::filesystem::remove_all(root);std::filesystem::create_directories(root/"mods.d");auto file=(root/"library.xml").string();
 auto first=set_library_organization(file,{},{{"sword","Original"},true,false,"Study Ω"});check(first.entries.size()==1&&first.entries[0].favorite,"Create favorite/group preferences");
 auto second=set_library_organization(file,first.revision,{{"future-provider","Original"},false,true,"Other"});check(second.entries.size()==2&&second.entries[0].favorite&&second.entries[1].hidden,"Provider-scoped identities cannot replace each other");
 rejects([&]{set_library_organization(file,first.revision,{{"sword","Original"},false,true,""});});rejects([&]{set_library_organization(file,{},{{"sword","Original"},false,true,""});});check(read_library_organization(file).revision==second.revision,"Stale/create-only edits preserve file");
 auto document=read_editable_document(file);auto position=document.text.find("<Item");document.text.insert(position,"<!--keep--><Extension value='original'/>");auto external=save_editable_document(file,document.revision,document.text);
 auto reset=set_library_organization(file,external.revision,{{"sword","Original"},false,false,""});auto bytes=read_editable_document(file).text;
 check(!reset.entries[0].favorite&&!reset.entries[0].hidden&&reset.entries[0].group.empty()&&reset.ignored==1&&bytes.find("<!--keep-->")!=std::string::npos,"Reset preferences retains unknown XML and other providers");
 rejects([&]{parse_library_organization("<XiphosLibrary version='1'><Item provider='sword' id='A'/><Item provider='sword' id='A'/></XiphosLibrary>");});
 rejects([&]{set_library_organization(file,reset.revision,{{"","Original"},true,false,""});});
 auto wait=[](xc_request *request){const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(20);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<until,"Organization request deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;};
 xc_worker *worker=nullptr;check(xc_worker_open(root.string().c_str(),&worker)==XC_OK,"C organization worker");xc_request *request=nullptr;const auto cfile=(root/"c-library.xml").string();std::string provider="sword",item="Original",group="Study";
 xc_library_organization_entry entry{provider.c_str(),item.c_str(),group.c_str(),1,0};
 check(xc_worker_set_library_organization(worker,cfile.c_str(),nullptr,&entry,&request)==XC_OK,"C organization create");provider.clear();item.clear();group.clear();check(wait(request)==XC_OK,"C copied organization inputs");auto *owned=xc_request_take_library_organization(request);check(owned&&!xc_request_take_library_organization(request),"Take organization once");xc_request_release(request);
 check(xc_library_organization_count(owned)==1&&std::string(xc_library_organization_provider(owned,0))=="sword"&&std::string(xc_library_organization_item(owned,0))=="Original"&&std::string(xc_library_organization_group(owned,0))=="Study"&&xc_library_organization_favorite(owned,0)==1,"C owned preference metadata");
 const std::string stale(64,'0');entry={"sword","Original",nullptr,0,1};check(xc_worker_set_library_organization(worker,cfile.c_str(),stale.c_str(),&entry,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT,"C stale preferences reject");xc_request_release(request);
 xc_worker_close(worker);std::filesystem::remove(cfile);check(std::string(xc_library_organization_group(owned,0))=="Study"&&xc_library_organization_hidden(owned,99)==-1,"Copied preferences outlive source and worker");xc_library_organization_release(owned);
 std::cout<<"Library favorites/hidden/groups, exact provider identity, revisions and unknown XML preservation passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
