// SPDX-License-Identifier: GPL-2.0-or-later
#include "parallel_interpolation_host.hpp"
int main(int argc,char **argv){try{
 check(argc==2,"Parallel fixture root required");const std::string root=argv[1];fixture(root);
 SWMgr old(root.c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false),now(root.c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);BackEnd baseline(old),current(now);
 std::size_t comparisons=0;
 const std::vector<std::vector<std::string>> sets={{"TextA","TextAlt"},{"TextAlt","TextA"},{"RtlText","TextA"},{"A","Alt","N"},{"TextA","Missing","TextA"},{"TextA","RtlText","Notes"}};
 for(const auto &set:sets)for(const auto *ref:{"John3:16","Psalms3:2","Tob1:1","Genesis1:1","John3:999","not-a-reference"})for(bool visible:{false,true})for(bool alternating:{false,true})for(bool headings:{false,true})for(const char *locale:std::vector<const char *>{nullptr,"en"}){
  const auto expected=run(frozen::interpolate_parallel_display,baseline,set,ref,visible,alternating,headings,locale);
  const auto result=run(actual::interpolate_parallel_display,current,set,ref,visible,alternating,headings,locale);
  check(result==expected,"Complete parallel SDK interpolation differs");++comparisons;
 }
 const auto canonical=run(actual::interpolate_parallel_display,current,{"TextA","TextAlt","Notes"},"John3:16",true,false,true,"en");
 check(canonical.selected==16&&canonical.html.find("Original TextA parallel prose for John3:16")!=std::string::npos&&canonical.html.find("Original TextAlt parallel prose for John3:16")!=std::string::npos,"Original content and selected anchor travel through actual SDK interpolation");
 auto occurrences=[](const std::string &text,const std::string &token){std::size_t count=0,at=0;while((at=text.find(token,at))!=std::string::npos){++count;at+=token.size();}return count;};
 const auto kjv=run(actual::interpolate_parallel_display,current,{"TextA","RtlText"},"Psalms3:2",true,false,false,"en");
 const auto synodal=run(actual::interpolate_parallel_display,current,{"RtlText","TextA"},"Psalms3:2",true,false,false,"en");
 VerseKey kjvKey,synodalKey;kjvKey.setVersificationSystem("KJV");synodalKey.setVersificationSystem("Synodal");kjvKey.setText("Psalms3:2");synodalKey.setText("Psalms3:2");
 check(occurrences(kjv.html,"<tr valign=")==static_cast<std::size_t>(kjvKey.getVerseMax())&&occurrences(synodal.html,"<tr valign=")==static_cast<std::size_t>(synodalKey.getVerseMax()),"First actual module controls chapter row count under differing versifications");
 const auto alias=run(actual::interpolate_parallel_display,current,{"A","Alt"},"John3:16",true,false,true,"en");
 check(canonical.html.find("Original TextA heading")!=std::string::npos&&alias.html.find("Original TextA heading")==std::string::npos,"Retained raw alias heading rendering loses title even though resolved body still renders");
 const auto missing=run(actual::interpolate_parallel_display,current,{"TextA","Missing","TextA"},"John3:16",true,false,false,"en");
 const auto rows=occurrences(missing.html,"<tr valign=");check(occurrences(missing.html,"<td width=")==rows*2&&occurrences(missing.html,"</td>")==rows*3,"Retained missing non-control emits unmatched closing cell; future typed results must preserve missing slot explicitly");
 std::size_t plans=0,rejected=0;
 for(const auto *id:{"TextA","TextAlt","RtlText","Notes"})for(const auto *ref:{"John3:16","Psalms3:2","Tob1:1","Genesis1:1","John3:999","not-a-reference"})for(const char *locale:std::vector<const char *>{nullptr,"en"}){
  const auto expected=run(frozen::interpolate_parallel_display,baseline,{id},ref,true,false,false,locale);
  auto *module=now.getModule(id);module->setKey("John3:23");const std::string before=module->getKeyText();
  try{
   const auto plan=xiphos::core::parallel_references(*module,ref,locale);
   std::vector<std::string> rows;const std::string prefix=std::string("render:")+id+":";
   for(const auto &line:expected.calls)if(line.compare(0,prefix.size(),prefix)==0)rows.push_back(line.substr(prefix.size()));
   check(plan.rows==rows&&plan.selected_verse==static_cast<unsigned>(expected.selected),"Owned cloned row plan differs from complete frozen interpolation references/selection");++plans;
  }catch(const std::invalid_argument &){++rejected;}
  check(std::string(module->getKeyText())==before,"Parallel row planning preserves live module position even on invalid input");
 }
 check(plans>=24&&rejected>0,"Row-plan comparison includes valid and explicitly rejected SDK input");
 const auto legacyPartial=run(frozen::interpolate_parallel_display,baseline,{"TextA"},"24",true,false,false,"en");
 auto *partialModule=now.getModule("TextA");partialModule->setKey("John3:23");
 const auto partial=xiphos::core::parallel_references(*partialModule,"24","en");
 check(partial.selected_verse==static_cast<unsigned>(legacyPartial.selected)&&partial.rows.size()==legacyPartial.cells.size(),"Bare numeric SDK reference retains the complete frozen control interpretation and row count");
 std::cout<<"Complete real-SDK parallel interpolation: "<<comparisons<<" frozen/current traces; "<<plans<<" owned plans and "<<rejected<<" bounded SDK rejections across KJV/KJVA/Synodal, aliases, sparse notes, missing modules, headings and gates agree\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
