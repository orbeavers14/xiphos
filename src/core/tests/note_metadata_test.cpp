// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include "xiphos/core/session.hpp"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
bool same_metadata(const StudyNote &first,const StudyNote &second) {
 return first.module==second.module && first.reference==second.reference && first.identifier==second.identifier &&
   first.type==second.type && first.label==second.label && first.reference_list==second.reference_list;
}
int main(int argc,char **argv) {
 try {
  check(argc==2,"generated note fixture required");
  StudyNote retained;
  unsigned comparisons=0;
  {
   Reader reader(argv[1]); StudySession session(reader);
   (void)session.visit("OptionOther","John3:23");
   for(unsigned flags=0;flags<4;++flags) {
    auto options=engine_options_from_preferences({}); options.strongs=flags&1; options.footnotes=flags&2;
    for(const auto *reference:{"John3:16","John3:18","John3:19","John3:20"}) {
     const auto metadata=reader.study_note_metadata("OptionOther",reference,"1",options);
     const auto full=reader.study_note("OptionOther",reference,"1",options);
     check(metadata.html.empty() && same_metadata(metadata,full),"exact-source metadata equals existing rendered note without body payload");
     session.set_module_engine_options("OptionOther",options);
     check(same_metadata(session.study_note_metadata("OptionOther",reference,"1"),full),"metadata respects per-tab effective engine settings");
     if(std::string(reference)=="John3:20") check(metadata.type=="crossReference"&&!metadata.reference_list.empty(),"cross-reference attributes copied before body render");
     if(std::string(reference)=="John3:19") check(full.html.find("Original other note")!=std::string::npos,"full body renderer retains original output");
     retained=metadata; ++comparisons;
    }
   }
   const auto expected=reader.study_note("OptionOther","John3:19","1");
   const auto first=reader.study_note_metadata("OptionOther","John3:19","1");
   (void)reader.study_note("OptionOther","John3:20","1");
   check(same_metadata(first,expected) && first.reference=="John 3:19" && first.html.empty(),"copied metadata survives later attribute replacement");
   check(session.history().entries().size()==1 && session.dictionary_history().entries().empty(),"metadata reads do not visit histories");
   bool module_rejected=false;
   try { (void)reader.study_note_metadata(std::string("OptionOther\0ignored",19),"John3:19","1"); } catch(const std::invalid_argument &) { module_rejected=true; }
   check(module_rejected,"module name cannot silently truncate at NUL");
   for(const auto &target:{std::pair<std::string,std::string>{"John3:17","1"}, {"John3:19","missing"}, {"","1"}, {"John3:19",std::string("1\0ignored",9)}, {std::string("John3:19\0ignored",16),"1"}}) {
    bool failed=false; try { (void)reader.study_note_metadata("OptionOther",target.first,target.second); } catch(const std::invalid_argument &) { failed=true; }
    check(failed,"metadata missing/invalid input rejects without synthetic attributes");
   }
  }
  check(retained.type=="crossReference" && !retained.reference_list.empty() && retained.html.empty(),"metadata survives reader/session close");
  std::cout<<"Owned metadata agrees with existing full notes across "<<comparisons<<" passage/settings cases\n";
 } catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
