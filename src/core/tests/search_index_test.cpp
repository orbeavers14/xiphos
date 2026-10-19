// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/search_policy.hpp"
#include <markupfiltmgr.h>
#include <swmodule.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::map<std::string,std::string> bytes(const fs::path &root){std::map<std::string,std::string> result;for(const auto &file:fs::recursive_directory_iterator(root))if(file.is_regular_file()){std::ifstream in(file.path(),std::ios::binary);result.emplace(fs::relative(file.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(in),{}));}return result;}
int main(int argc,char **argv){try{
 check(argc==3||(argc==4&&std::string(argv[3])=="--require-index"),"Generated source fixture and isolated index-copy directory required; optional --require-index");
 const bool required=argc==4;
 const fs::path source=fs::absolute(argv[1]),root=fs::absolute(argv[2]);
 check(source!=root&&!fs::is_symlink(root),"Index fixture must be a separate generated copy");
 const auto original=bytes(source/"data");fs::create_directories(root);
 for(const auto &file:fs::recursive_directory_iterator(source))if(file.is_regular_file()&&file.path().filename()!="modules-conf.cache"){
  const auto target=root/fs::relative(file.path(),source);fs::create_directories(target.parent_path());fs::copy_file(file.path(),target,fs::copy_options::overwrite_existing);
 }
 fs::remove(root/"mods.d/modules-conf.cache");
 int created;
 {
  EngineManager manager(root.string().c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);
  auto *module=manager.getModule("Text");check(module,"Copied original Bible is installed");
  const char *path=module->getConfigEntry("AbsoluteDataPath");
  check(path&&fs::weakly_canonical(path)==fs::weakly_canonical(root/"data/text"),"SDK index writes are restricted to the copied original-prose module");
  // This first actual-index gate uses unannotated text only; annotation index
  // creation/option combinations remain a separate backend/corpus gate.
  manager.setGlobalOption("Footnotes","Off");manager.setGlobalOption("Strong's Numbers","Off");manager.setGlobalOption("Morphological Tags","Off");
  module->deleteSearchFramework();created=module->createSearchFramework();
  if(!module->hasSearchFramework()||!supports_indexed_search(*module)){
   check(bytes(source/"data")==original,"Unavailable index probe never modifies source fixture");
   std::cout<<(required?"FAIL required index gate: ":"SKIP: ")<<"actual SDK did not produce a usable indexed framework (create status "<<created<<"). Controlled dialect tests remain separate; indexed execution is unvalidated.\n";
   return required?1:77;
  }
 }
 {
  Reader reader(root.string());StudySession session(reader);
  const auto indexed=session.search_module({"Text","Original",SearchMode::indexed},{1,1024});
  check(indexed.engine_type==-4&&!indexed.index_fallback&&indexed.executed&&indexed.references==std::vector<std::string>{"John 1:1"}&&indexed.limited&&indexed.scope.element_count==4,"Actual indexed result prefix preserves complete typed next scope");
  const auto raw=session.search_module({"Text","alpha beta",SearchMode::indexed});
  const auto automatic=session.search_module({"Text","alpha beta",SearchMode::words});
  check(raw.engine_type==-4&&raw.effective_expression=="alpha beta"&&raw.references==std::vector<std::string>{"John 1:1","John 1:2"},"Actual raw advanced syntax uses SDK default OR with ordered display keys");
  check(automatic.engine_type==-4&&automatic.effective_expression=="+alpha +beta "&&automatic.references.empty(),"Actual indexed sidebar words retain AND preparation");
  SearchQuery bounded{"Text","Original",SearchMode::indexed};bounded.range=SearchRange{"Text","John1:2-John1:3"};
  check(session.search_module(bounded).references==std::vector<std::string>{"John 1:2","John 1:3"},"Actual external search respects explicit typed interval bounds");
  SearchQuery last{"Text","Original",SearchMode::indexed};last.within_last_results=true;
  check(session.search_module(last).references==std::vector<std::string>{"John 1:2","John 1:3"},"Actual external search respects complete prior last scope");
 }
 check(bytes(source/"data")==original,"Indexed creation/search never alters source fixture bytes");
 std::cout<<"Actual generated SDK index execution, advanced OR/sidebar AND, prefix/scope and bounds passed (create status "<<created<<")\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
