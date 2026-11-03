// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/library_organization.hpp"
#include "xiphos/core/editable_document.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace xiphos::core;
static void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>static void rejects(F call){try{call();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected organization rejection");}
int main(int argc,char **argv)try{
 check(argc==2,"Disposable path required");std::filesystem::path root=argv[1];check(root.filename()=="library-organization-fixture","Named fixture only");std::filesystem::remove_all(root);std::filesystem::create_directories(root);auto file=(root/"library.xml").string();
 auto first=set_library_organization(file,{},{{"sword","Original"},true,false,"Study Ω"});check(first.entries.size()==1&&first.entries[0].favorite,"Create favorite/group preferences");
 auto second=set_library_organization(file,first.revision,{{"future-provider","Original"},false,true,"Other"});check(second.entries.size()==2&&second.entries[0].favorite&&second.entries[1].hidden,"Provider-scoped identities cannot replace each other");
 rejects([&]{set_library_organization(file,first.revision,{{"sword","Original"},false,true,""});});rejects([&]{set_library_organization(file,{},{{"sword","Original"},false,true,""});});check(read_library_organization(file).revision==second.revision,"Stale/create-only edits preserve file");
 auto document=read_editable_document(file);auto position=document.text.find("<Item");document.text.insert(position,"<!--keep--><Extension value='original'/>");auto external=save_editable_document(file,document.revision,document.text);
 auto reset=set_library_organization(file,external.revision,{{"sword","Original"},false,false,""});auto bytes=read_editable_document(file).text;
 check(!reset.entries[0].favorite&&!reset.entries[0].hidden&&reset.entries[0].group.empty()&&reset.ignored==1&&bytes.find("<!--keep-->")!=std::string::npos,"Reset preferences retains unknown XML and other providers");
 rejects([&]{parse_library_organization("<XiphosLibrary version='1'><Item provider='sword' id='A'/><Item provider='sword' id='A'/></XiphosLibrary>");});
 rejects([&]{set_library_organization(file,reset.revision,{{"","Original"},true,false,""});});
 std::cout<<"Library favorites/hidden/groups, exact provider identity, revisions and unknown XML preservation passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
