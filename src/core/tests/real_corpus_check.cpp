// SPDX-License-Identifier: GPL-2.0-or-later
// Manual opt-in check. Never downloads, writes modules, or prints source text.
#include "xiphos/core/worker.hpp"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
static void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
int main(int argc,char **argv)try{
 check(argc==2,"Select a disposable library containing ASV, OSHB, MHCC and StrongsGreek");
 auto original=inspect_legacy_assets(argv[1]);
 StudyWorker worker(argv[1]);auto tab=worker.create_tab().get();
 for(const auto &selection:std::vector<std::pair<std::string,std::string>>{{"ASV","John3:16"},{"OSHB","Gen1:1"}}){
  auto chapter=worker.read_chapter(tab,selection.first,selection.second).get();
  check(!chapter.verses.empty()&&!chapter.html.empty()&&editable_document_text(chapter.html),"Real verse/chapter output must be nonempty UTF-8");
  std::cout<<selection.first<<": chapter verses="<<chapter.verses.size()<<"\n";
 }
 auto commentary=worker.read_passage(tab,"ASV","MHCC","Gen1:1").get();check(commentary.commentary&&!commentary.commentary->html.empty(),"Real linked-verse commentary composes with Bible text");
 for(auto mode:{SearchMode::phrase,SearchMode::words,SearchMode::regular_expression,SearchMode::multiword}){
  auto result=worker.search_module(tab,{"ASV","love",mode}).get();check(result.executed&&!result.references.empty(),"Real ASV text modes execute and return matches");
 }
 auto hebrew=worker.search_module(tab,{"OSHB","אלהים",SearchMode::phrase}).get();check(hebrew.executed&&!hebrew.references.empty(),"Real Hebrew search resolves unpointed UTF-8 query through module filters");
 auto dictionary=worker.search_keyed_module_task(tab,{"StrongsGreek","God",SearchMode::phrase}).result.get();check(dictionary.executed&&!dictionary.references.empty(),"Real compressed dictionary search finds matches");
 for(std::size_t i=0;i<std::min<std::size_t>(3,dictionary.references.size());++i){auto entry=worker.preview_keyed_search(tab,"StrongsGreek",dictionary.references[i],dictionary.key_kind,1024*1024).get();check(!entry.html.empty()&&editable_document_text(entry.html),"Real dictionary exact hits preview as owned UTF-8");}
 auto batch=worker.search_mixed_batch_task(tab,{{"ASV","love",SearchMode::phrase},{"StrongsGreek","God",SearchMode::phrase},{"MHCC","love",SearchMode::phrase}}).result.get();check(!batch.failures&&batch.copied_results>0,"Real mixed verse/dictionary groups execute");
 SearchAllTextExportRequest request;request.generation=batch.generation.id;request.include_scripture=true;request.max_bytes=maximum_search_export_bytes;
 auto output=worker.export_search_all_text(tab,request).get();check(output.status==SearchExportStatus::ok&&!output.text.empty()&&editable_document_text(output.text),"Real mixed export is complete UTF-8");
 check(inspect_legacy_assets(argv[1]).revision==original.revision,"Real corpus source files remain byte-identical");
 std::cout<<"Real corpus checks passed; mixed rows="<<output.row_count<<"; no source text logged\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
