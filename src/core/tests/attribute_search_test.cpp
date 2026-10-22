// SPDX-License-Identifier: GPL-2.0-or-later
#include "../sdk_search.hpp"
#include "xiphos/core/session.hpp"
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/search_policy.hpp"
#include "search_source.hpp"
#include <rawtext.h>
#include <rawcom.h>
#include <markupfiltmgr.h>
#include <versekey.h>
#include <swoptfilter.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <map>
#include <chrono>
#include <thread>
using namespace xiphos::core;
using namespace sword;
namespace fs=std::filesystem;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void main_dialog_search_percent_update(char,void *){}
void main_sidebar_search_percent_update(char,void *){}
struct BackEnd {
 EngineManager *main_mgr;SWModule *search_mod=nullptr;ListKey results,search_scope_list;SWKey *current_scope=nullptr;
 int do_module_search(char *,const char *,int,int,int);
};
#include "legacy_search_backend_actual.inc"
std::vector<std::string> refs(ListKey &list){std::vector<std::string> out;list.setPosition(TOP);while(list.getCount()&&!list.popError()){out.emplace_back(list.getText());list++;}return out;}
std::map<std::string,std::string> data_bytes(const fs::path &root){std::map<std::string,std::string> out;for(const auto &file:fs::recursive_directory_iterator(root))if(file.is_regular_file()){std::ifstream in(file.path(),std::ios::binary);out.emplace(fs::relative(file.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(in),{}));}return out;}
xc_status wait(xc_request *r){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Attribute request deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Original attribute fixture required");const fs::path root=argv[1];fs::create_directories(root/"mods.d");fs::create_directories(root/"data/text");fs::create_directories(root/"data/commentary");
 check(RawText::createModule((root/"data/text").string().c_str())==0&&RawCom::createModule((root/"data/commentary").string().c_str())==0,"Create original attribute sources");
 const std::vector<std::string> entries{
  "Original <w lemma=\"strong:G140\" morph=\"robinson:N-ASF\">alpha</w><note type=\"study\" n=\"a\">Original café Alpha note.</note>",
  "Original <w lemma=\"strong:G1401\" morph=\"robinson:N-ASF1\">beta</w><note type=\"study\" n=\"b\">Original alpha note.</note>",
  "Original <w lemma=\"strong:G1404\" morph=\"robinson:N-ASF\">gamma</w><note type=\"study\" n=\"c\">Original continuation.</note>",
  "Original <w lemma=\"strong:G141 strong:G140\" morph=\"robinson:V-PAP\">delta</w>"};
 for(bool commentary:{false,true}){
  const std::string id=commentary?"Commentary":"Text",driver=commentary?"RawCom":"RawText",path=commentary?"commentary":"text";
  std::unique_ptr<SWModule> writer=commentary?std::unique_ptr<SWModule>(new RawCom((root/"data/commentary").string().c_str())):std::unique_ptr<SWModule>(new RawText((root/"data/text").string().c_str()));
  for(std::size_t i=0;i<entries.size();++i){writer->setKey(("John1:"+std::to_string(i+1)).c_str());writer->setEntry(entries[i].c_str());}
  std::ofstream conf(root/"mods.d"/(id+".conf"));conf<<'['<<id<<"]\nDataPath=./data/"<<path<<"\nModDrv="<<driver<<"\nSourceType=OSIS\nEncoding=UTF-8\nGlobalOptionFilter=OSISStrongs\nGlobalOptionFilter=OSISMorph\nGlobalOptionFilter=OSISFootnotes\n";
 }
 fs::remove(root/"mods.d/modules-conf.cache");
 const auto before=data_bytes(root/"data");
 {
 Reader reader(root.string());EngineManager legacy(root.string().c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);BackEnd backend{&legacy};
 for(const auto *module:{"Text","Commentary"})for(auto mode:{SearchMode::lemma,SearchMode::morphology,SearchMode::footnote})for(bool sensitive:{false,true}){
  const std::string text=mode==SearchMode::lemma?"G140":mode==SearchMode::morphology?"N-ASF":"Alpha";
  SearchQuery query{module,text,mode,sensitive};const auto result=reader.search_module(query);
  const auto kind=mode==SearchMode::lemma?AttributeSearch::lemma:mode==SearchMode::morphology?AttributeSearch::morphology:AttributeSearch::footnote;
  const auto prepared=attribute_search_text(text,kind);auto &source=*legacy.getModule(module);const std::string stripped=sword::SWBuf(source.stripText(prepared.expression.c_str())).c_str();
  const int flags=(sensitive?0:XIPHOS_SWORD_SEARCH_ICASE_FLAG)|(prepared.whole_entry?xiphos::core::detail::sdk_search_whole_entry:0);
  check(backend.do_module_search(const_cast<char *>(module),stripped.c_str(),xiphos::core::detail::sdk_search_attribute,flags,1)>=0,"Actual legacy attribute search completes");
  check(result.references==refs(backend.results)&&result.effective_expression==stripped&&result.requested.expression==text&&result.engine_type==xiphos::core::detail::sdk_search_attribute,"Owned attribute mode/case/query and ordered results match actual backend");
  const auto expected=mode==SearchMode::lemma?std::vector<std::string>{"John 1:1","John 1:4"}:mode==SearchMode::morphology?std::vector<std::string>{"John 1:1","John 1:3"}:sensitive?std::vector<std::string>{"John 1:1"}:std::vector<std::string>{"John 1:1","John 1:2"};
  if(result.references!=expected){std::cerr<<"attribute mode="<<static_cast<int>(mode)<<" source="<<module<<" case="<<sensitive<<" query="<<result.effective_expression<<" flags="<<flags<<" actual=";for(const auto &key:result.references)std::cerr<<key<<';';std::cerr<<'\n';}
  check(result.references==expected,"Exact lemma/morph identifiers exclude longer prefixes; component lemma and note case behavior are present");
 }
 StudySession session(reader);session.visit("Text","John1:2");
 const auto seed=session.search_module({"Text","N-ASF",SearchMode::morphology},{1,1024});
 check(seed.limited&&seed.references.size()==1&&seed.scope.element_count==2,"Attribute execution retains complete last scope despite display limits");
 SearchQuery narrowed{"Commentary","G140",SearchMode::lemma};narrowed.within_last_results=true;
 check(session.search_module(narrowed).references==std::vector<std::string>{"John 1:1"},"Attribute mode can narrow complete cross-module last scope");
 SearchQuery range{"Text","Alpha",SearchMode::footnote};range.range=SearchRange{"Text","John1:2-John1:3"};
 check(session.search_module(range).references==std::vector<std::string>{"John 1:2"}&&session.history().entries().size()==1,"Footnote mode uses shared explicit bounds without navigation");
 auto &module=*legacy.getModule("Text");module.setProcessEntryAttributes(false);module.setSkipConsecutiveLinks(true);module.setKey("John1:3");
 std::vector<std::pair<SWOptionFilter *,std::string>> filters;for(auto *filter:legacy.option_filters())filters.emplace_back(filter,filter->getOptionValue());
 const auto copied=detail::capture_module_search(legacy,module,{"Text","G140",SearchMode::lemma},{});
 check(copied.references.size()==2&&!module.isProcessEntryAttributes()&&module.isSkipConsecutiveLinks()&&std::string(module.getKeyText())=="John 1:3","Attribute work restores position and caller processing/link flags");
 for(const auto &filter:filters)check(filter.first->getOptionValue()==filter.second,"Attribute search retains every option baseline");
 for(const auto *enabled:{"Off","On"}){
  legacy.setGlobalOption("Strong's Numbers",enabled);legacy.setGlobalOption("Morphological Tags",enabled);legacy.setGlobalOption("Footnotes",enabled);
  for(auto mode:{SearchMode::lemma,SearchMode::morphology,SearchMode::footnote}){
   const auto text=mode==SearchMode::lemma?"G140":mode==SearchMode::morphology?"N-ASF":"Alpha";
   const auto found=detail::capture_module_search(legacy,module,{"Text",text,mode},{});
   check(found.references.size()==2,"SDK attributes remain searchable with displayed Strong/morph/note annotations On or Off");
   check(std::string(legacy.getGlobalOption("Strong's Numbers"))==enabled&&std::string(legacy.getGlobalOption("Morphological Tags"))==enabled&&std::string(legacy.getGlobalOption("Footnotes"))==enabled,"Attribute execution leaves caller annotation options intact");
  }
 }
 check(reader.search_module({"Text","café",SearchMode::footnote}).references==std::vector<std::string>{"John 1:1"},"UTF-8 footnote text remains exact");
 }
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.string().c_str(),&worker)==XC_OK,"Open C attribute worker");
 check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create attribute tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 xc_search *held=nullptr;
 for(int mode:{XC_SEARCH_LEMMA,XC_SEARCH_MORPHOLOGY,XC_SEARCH_FOOTNOTE}){
  xc_search_options options;xc_search_options_default(&options);options.mode=mode;std::string input=mode==XC_SEARCH_LEMMA?"G140":mode==XC_SEARCH_MORPHOLOGY?"N-ASF":"Alpha";
  check(xc_worker_search_module(worker,tab,"Text",input.c_str(),&options,&request)==XC_OK,"Queue copied attribute mode");input="unrelated";options.mode=XC_SEARCH_PHRASE;
  check(wait(request)==XC_OK,"Complete C attribute search");auto *result=xc_request_take_search(request);xc_request_release(request);xc_search_info info{};xc_search_scope_info scope{};
  check(result&&xc_search_get_info(result,&info)==XC_OK&&info.mode==mode&&info.engine_type==xiphos::core::detail::sdk_search_attribute&&info.result_count==2&&xc_search_get_scope(result,&scope)==XC_OK&&scope.element_count==2,"C transports attribute mode, flags and complete scope metadata through unchanged options layout");
  if(held)xc_search_release(held);held=result;
 }
 xc_search_options options;xc_search_options_default(&options);options.mode=XC_SEARCH_LEMMA;
 check(xc_worker_search_module_in_last_results(worker,tab,"Commentary","G140",&options,&request)==XC_OK&&wait(request)==XC_OK,"Attribute C last-scope query");auto *last=xc_request_take_search(request);xc_request_release(request);xc_search_info info{};
 check(xc_search_get_info(last,&info)==XC_OK&&info.result_count==1&&std::string(xc_search_reference(last,0))=="John 1:1","Attribute C query narrows copied prior footnote scope");xc_search_release(last);
 xc_worker_close(worker);check(std::string(xc_search_expression(held))=="Alpha"&&xc_search_get_info(held,&info)==XC_OK&&info.mode==XC_SEARCH_FOOTNOTE,"Owned C attribute result survives worker shutdown");xc_search_release(held);
 check(data_bytes(root/"data")==before,"Attribute searches preserve all source module bytes");
 std::cout<<"Owned lemma/morph/footnote search matches legacy exact identifiers, case, bounds and complete scopes passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
