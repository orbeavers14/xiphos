// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include "xiphos/core/session.hpp"
#include "xiphos/core/worker.hpp"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
int main(int argc,char **argv) {
 try {
  check(argc==2,"generated reference fixture required");
  ParsedReferenceList retained;
  unsigned comparisons=0;
  {
   Reader reader(argv[1]); StudySession session(reader);
   (void)session.visit("OptionOther","John3:23");
   for(const auto *module:{"OptionTest","OptionOther"}) for(const auto *context:{"John3:16","Genesis1:1"})
    for(const auto *expression:{"John3:16","John3:16-19;John3:16","16-18;20","John3:36-4:2","Jude","","not-a-reference","Gen1:1-Rev22:21"}) {
     const auto targets=reader.reference_targets(module,expression,context);
     const auto full=reader.read_reference_list(module,expression,context);
     check(targets.context==full.context && targets.limited==full.limited && targets.skipped==full.skipped && targets.references.size()==full.entries.size(),"targets/full snapshots have matching context/order/guard metadata");
     for(std::size_t i=0;i<targets.references.size();++i) check(targets.references[i]==full.entries[i].reference,"each target matches existing rendered snapshot reference");
     if(std::string(expression)=="Gen1:1-Rev22:21") check(targets.limited&&targets.references.size()==2000,"existing large-range cap retained without body payloads");
     ++comparisons;
    }
   EngineOptions enabled; enabled.strongs=true; enabled.footnotes=true;
   session.set_module_engine_options("OptionOther",enabled);
   retained=session.reference_targets("OptionOther","16-19;16","John3:23");
   session.set_module_engine_options("OptionOther",EngineOptions{});
   check(session.reference_targets("OptionOther","16-19;16","John3:23").references==retained.references,"targets do not depend on rendering flags");
   check(retained.references.size()==5 && retained.references.front()==retained.references.back() && session.history().entries().size()==1 && session.dictionary_history().entries().empty(),"owned target order/duplicates and no history visit");
   for(const auto &expression:{std::string("16\0ignored",10)}) {
    bool failed=false; try { (void)reader.reference_targets("OptionOther",expression,"John3:23"); } catch(const std::invalid_argument &) { failed=true; }
    check(failed,"embedded NUL expression rejected rather than truncated");
   }
   bool failed=false; try { (void)reader.reference_targets("OptionOther","16",""); } catch(const std::invalid_argument &) { failed=true; }
   check(failed,"empty target context rejected");
   failed=false; try { (void)reader.reference_targets("Missing","16","John3:23"); } catch(const std::out_of_range &) { failed=true; }
   check(failed,"missing target module rejected");
   (void)reader.read("OptionOther","John3:19");
   check(retained.references.size()==5,"targets survive later rendering");
  }
  check(retained.references.size()==5 && retained.context=="John 3:23","owned targets survive session/reader close");
  std::cout<<"Reference targets agree with existing rendered snapshots across "<<comparisons<<" module/context/expression cases\n";
 } catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
