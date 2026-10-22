// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::linux_preview;
void check(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
void settle(ReaderModel &m) {const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(20);do {m.tick();check(std::chrono::steady_clock::now()<end,"Alias model deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(m.busy());check(m.error().empty(),m.error().c_str());}
int main(int argc,char **argv) {try {
 check(argc==2,"Generated alias fixture required");ReaderModel model(argv[1]);settle(model);
 check(model.visit("AFirst","","John1:1"),"Seed reading");settle(model);
 const auto before=model.passage();SearchInput input;input.modules={"Shared","AFirst","文字 café","Shared"};input.expression="Original";
 check(model.search(input),"Submit raw aliases");settle(model);const auto copied=model.search_results();
 check(copied.groups.size()==4&&copied.groups[0].module=="Shared"&&copied.hits.size()==4&&copied.hits[0].module=="BSecond"&&copied.hits[1].module=="AFirst"&&copied.hits[2].module=="Unicode"&&copied.hits[3].module=="BSecond"&&model.passage().bible==before.bible,"Raw group labels and actual hit source IDs stay distinct without navigation");
 check(model.preview_search_hit(0),"Preview actual alias source");settle(model);
 check(model.search_preview().module=="BSecond"&&model.search_preview().html.find("prose for BSecond")!=std::string::npos,"Alias preview uses installed source");
 check(model.export_search(false,true,{},true,true,false,false),"Export actual retained aliases");settle(model);
 check(model.search_export().disposition==XC_SEARCH_EXPORT_OK&&model.search_export().text.find("Resolved module: BSecond")!=std::string::npos&&model.search_export().text.find("prose for Unicode")!=std::string::npos,"Alias export consumes actual source with requested provenance");
 check(model.activate_search_hit(0),"Activate actual alias source");settle(model);
 check(model.passage().bible=="BSecond"&&model.search_results().hits[0].module=="BSecond","Alias activation routes source through existing core");
 std::cout<<"Portable Linux alias preview/activation/export model checks passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
