// SPDX-License-Identifier: GPL-2.0-or-later
// Manual only. Index a new disposable clone; never mutate the supplied corpus.
#include "xiphos/core/worker.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/search_policy.hpp"
#include <markupfiltmgr.h>
#include <filesystem>
#include <iostream>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool v,const char *why){if(!v)throw std::runtime_error(why);}
int main(int argc,char **argv)try{
 check(argc==3||argc==4,"Source library, new real-corpus-index-fixture folder and optional --require-index");
 const bool required=argc==4&&std::string(argv[3])=="--require-index";check(argc==3||required,"Unknown index option");
 auto original=inspect_legacy_assets(argv[1]);auto target=fs::u8path(argv[2]);check(target.filename()=="real-corpus-index-fixture","Named new disposable index folder only");
 copy_legacy_assets(argv[1],original.revision,argv[2]);
 {
  EngineManager manager(target.u8string().c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);
  for(const auto *id:{"ASV","StrongsGreek","Pilgrim"}){auto *module=manager.getModule(id);check(module,"Real index source is installed");module->deleteSearchFramework();const auto created=module->createSearchFramework();if(created!=0||!supports_indexed_search(*module)){check(inspect_legacy_assets(argv[1]).revision==original.revision,"Unsupported indexing leaves original untouched");std::cout<<(required?"FAIL: ":"SKIP: ")<<"SDK cannot create real corpus index for "<<id<<'\n';return required?1:77;}}
 }
 StudyWorker worker(target.u8string());auto tab=worker.create_tab().get();
 for(const auto &query:std::vector<SearchQuery>{{"ASV","love",SearchMode::indexed},{"StrongsGreek","God",SearchMode::indexed},{"Pilgrim","Christian",SearchMode::indexed}}){auto result=query.module=="ASV"?worker.search_module(tab,query).get():worker.search_keyed_module_task(tab,query).result.get();check(result.executed&&result.engine_type==-4&&!result.index_fallback&&!result.references.empty(),"Real index executes without fallback and returns matches");std::cout<<query.module<<": indexed hits="<<result.total_results<<'\n';}
 check(inspect_legacy_assets(argv[1]).revision==original.revision,"Index clone never changes original corpus");std::cout<<"Real indexed verse/dictionary/general-book checks passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
