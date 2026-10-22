// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/search_policy.hpp"
#include "search_source.hpp"
#include <rawtext.h>
#include <rawcom.h>
#include <rawld.h>
#include <rawgenbook.h>
#include <markupfiltmgr.h>
#include <swoptfilter.h>
#include <swfilter.h>
#include <listkey.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace xiphos::core;
using namespace sword;
namespace fs=std::filesystem;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected search request error");}
int updates=0;
void main_dialog_search_percent_update(char,void *){++updates;}
void main_sidebar_search_percent_update(char,void *){++updates;}
struct BackEnd {
 EngineManager *main_mgr;SWModule *search_mod=nullptr;ListKey results,search_scope_list;SWKey *current_scope=nullptr;
 int do_module_search(char *module_name,const char *search_string,int search_type,int search_params,int is_dialog);
};
#include "legacy_search_backend_actual.inc"
std::vector<std::string> references(ListKey &list){std::vector<std::string> result;list.setPosition(TOP);while(list.getCount()&&!list.popError()){result.emplace_back(list.getText());list++;}return result;}
std::map<std::string,std::string> bytes(const fs::path &root){std::map<std::string,std::string> out;for(const auto &file:fs::recursive_directory_iterator(root))if(file.is_regular_file()){std::ifstream in(file.path(),std::ios::binary);out.emplace(fs::relative(file.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(in),{}));}return out;}
struct Fault:SWFilter {bool enabled=true;char processText(SWBuf &,const SWKey *,const SWModule *)override{if(enabled)throw std::runtime_error("Original injected strip failure");return 0;}};
int main(int argc,char **argv){try{
 check(argc==2,"Original search fixture required");const fs::path root(argv[1]);fs::create_directories(root/"mods.d");fs::create_directories(root/"data");
 fs::create_directories(root/"data/text");fs::create_directories(root/"data/commentary");
 check(RawText::createModule((root/"data/text").string().c_str())==0,"Create original search Bible");check(RawCom::createModule((root/"data/commentary").string().c_str())==0,"Create original search commentary");
 for(bool commentary:{false,true}){
  const std::string id=commentary?"Commentary":"Text",driver=commentary?"RawCom":"RawText",path=commentary?"commentary":"text";
  std::unique_ptr<SWModule> writer=commentary?std::unique_ptr<SWModule>(new RawCom((root/"data/commentary").string().c_str())):std::unique_ptr<SWModule>(new RawText((root/"data/text").string().c_str()));
  writer->setKey("John 1:1");writer->setEntry("Original Alpha beta café.");writer->setKey("John 1:2");writer->setEntry("Original alpha gamma.");writer->setKey("John 1:3");writer->setEntry("Original omega beta.");
  std::ofstream conf(root/"mods.d"/(id+".conf"));conf<<'['<<id<<"]\nDataPath=./data/"<<path<<"/\nModDrv="<<driver<<"\nSourceType=OSIS\nEncoding=UTF-8\nGlobalOptionFilter=UTF8GreekAccents\nGlobalOptionFilter=UTF8HebrewPoints\nGlobalOptionFilter=UTF8ArabicPoints\n";
 }
 check(RawLD::createModule((root/"data/dictionary").string().c_str())==0,"Create excluded search module family");{std::ofstream conf(root/"mods.d/Dictionary.conf");conf<<"[Dictionary]\nDataPath=./data/dictionary\nModDrv=RawLD\n";}
 check(RawGenBook::createModule((root/"data/book").string().c_str())==0,"Create search-neutral book source");
 {RawGenBook book((root/"data/book").string().c_str());book.setKey("/Entry");book.setEntry("Original independent book reading.");std::ofstream conf(root/"mods.d/Book.conf");conf<<"[Book]\nDataPath=./data/book\nModDrv=RawGenBook\nSourceType=ThML\nEncoding=UTF-8\n";}
 fs::remove(root/"mods.d/modules-conf.cache");const auto before=bytes(root/"data");SearchResult held;
 Fault fault;Reader reader(root.string());EngineManager legacy(root.string().c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);BackEnd backend{&legacy};
 check(reader.read("Text","John1:1").html.find("Original Alpha")!=std::string::npos && reader.read_commentary_entry("Commentary","John1:1").html.find("Original Alpha")!=std::string::npos,"Search fixture data is present before comparing engine results");
 for(const auto *module:{"Text","Commentary"})for(const auto mode:{SearchMode::phrase,SearchMode::regular_expression,SearchMode::words})for(bool sensitive:{false,true}){
  SearchQuery query{module,mode==SearchMode::regular_expression?"[Aa]lpha":"Alpha",mode,sensitive};
  auto result=reader.search_module(query);auto &source=*legacy.getModule(module);
  legacy.setGlobalOption("Greek Accents","Off");legacy.setGlobalOption("Hebrew Vowel Points","Off");legacy.setGlobalOption("Arabic Vowel Points","Off");
  const auto prepared=sidebar_search_text(query.expression,result.engine_type==-4);const std::string stripped=sword::SWBuf(source.stripText(prepared.c_str())).c_str();
  check(backend.do_module_search(const_cast<char *>(module),stripped.c_str(),result.engine_type,sensitive?0:XIPHOS_SWORD_SEARCH_ICASE_FLAG,0)>=0,"Actual legacy backend search completes");
  check(result.references==references(backend.results)&&result.total_results==static_cast<std::uint64_t>(backend.results.getCount())&&result.effective_expression==stripped&&result.requested.expression==query.expression,"Owned search keys/order/query match actual legacy backend in each mode/case/module");
  if(mode==SearchMode::phrase && result.references.size()!=(sensitive?1:2)){std::cerr<<"Phrase source="<<module<<" case_sensitive="<<sensitive<<" count="<<result.references.size()<<" case_flag="<<XIPHOS_SWORD_SEARCH_ICASE_FLAG<<" query="<<result.effective_expression<<" keys=";for(const auto &key:result.references)std::cerr<<key<<';';std::cerr<<'\n';}
  if(mode==SearchMode::phrase)check(result.references.size()==(sensitive?1:2),"Case flag matches linked SDK ABI and expected upper/lower phrase hits");held=std::move(result);
 }
 check(updates>0,"Actual SDK progress callback path exercised by oracle");
 auto stripped_markup=reader.search_module({"Text","<w>Alpha</w>",SearchMode::phrase,false});check(stripped_markup.effective_expression==sword::SWBuf(legacy.getModule("Text")->stripText("<w>Alpha</w>")).c_str()&&stripped_markup.requested.expression=="<w>Alpha</w>","Engine query uses exact SDK stripText output while original request stays unchanged");
 auto all=reader.search_module({"Text","Original",SearchMode::phrase,false});SearchLimits one;one.max_results=1;auto prefix=reader.search_module({"Text","Original",SearchMode::phrase,false},one);
 check(all.references.size()==3&&prefix.references.size()==1&&prefix.limited&&prefix.total_results==3&&prefix.references.front()==all.references.front(),"Result limit copies complete SDK prefix and retains total count");
 one.max_results=100;one.max_reference_bytes=1;auto no_key=reader.search_module({"Text","Original",SearchMode::phrase,false},one);check(no_key.references.empty()&&no_key.limited&&no_key.total_results==3,"Byte limit never truncates a reference");
 check(reader.search_module({"Text","unmatched-original-value",SearchMode::phrase,false}).references.empty(),"No-match result remains a valid owned snapshot");
 for(const auto &bad:std::vector<std::string>{"",std::string("bad\0query",9),std::string("\xff",1),std::string(32769,'a')})rejects<std::invalid_argument>([&]{reader.search_module({"Text",bad});});
 rejects<std::invalid_argument>([&]{reader.search_module({"Text","Original",static_cast<SearchMode>(99)});});rejects<std::invalid_argument>([&]{reader.search_module({"Dictionary","Original"});});rejects<std::out_of_range>([&]{reader.search_module({"Missing","Original"});});
 one.max_results=0;rejects<std::invalid_argument>([&]{reader.search_module({"Text","Original"},one);});
 auto &module=*legacy.getModule("Text");module.setKey("John1:2");module.setSkipConsecutiveLinks(true);const std::string saved_key=module.getKeyText();const bool skipped=module.isSkipConsecutiveLinks();legacy.setGlobalOption("Greek Accents","On");legacy.setGlobalOption("Hebrew Vowel Points","On");legacy.setGlobalOption("Arabic Vowel Points","On");module.setProcessEntryAttributes(false);
 std::vector<std::pair<SWOptionFilter *,std::string>> filters;for(auto *filter:legacy.option_filters())filters.emplace_back(filter,filter->getOptionValue());
 (void)detail::capture_module_search(legacy,module,{"Text","Original"},{});check(!module.isProcessEntryAttributes()&&std::string(module.getKeyText())==saved_key&&module.isSkipConsecutiveLinks()==skipped&&!module.terminateSearch,"Search restores caller position/flags under the checked SDK");for(const auto &filter:filters)check(filter.second==filter.first->getOptionValue(),"Search restores every filter baseline");
 module.addStripFilter(&fault);rejects<std::runtime_error>([&]{detail::capture_module_search(legacy,module,{"Text","Original"},{});});fault.enabled=false;check(!module.isProcessEntryAttributes(),"Strip failure restores caller attributes");for(const auto &filter:filters)check(filter.second==filter.first->getOptionValue(),"Strip failure restores temporary accent/vowel settings");
 check(detail::capture_module_search(legacy,module,{"Text","Original"},{}).references.size()==3,"Search recovers after injected strip failure");
 check(bytes(root/"data")==before&&!held.references.empty(),"Search leaves module data unchanged and retains owned result copies");
 std::cout<<"Owned search matches actual legacy backend, case modes, bounded output and filter restoration passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
