// SPDX-License-Identifier: GPL-2.0-or-later
// Manual opt-in check. Never downloads, writes modules, or prints source text.
#include "xiphos/core/worker.hpp"
#include <iostream>
#include <stdexcept>
#include <set>
#include <libxml/HTMLparser.h>
using namespace xiphos::core;
static void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
static std::set<std::string> internal_links(const std::string &html){
 std::unique_ptr<xmlDoc,decltype(&xmlFreeDoc)> doc(htmlReadMemory(html.data(),static_cast<int>(html.size()),nullptr,"UTF-8",HTML_PARSE_NONET|HTML_PARSE_NOERROR|HTML_PARSE_NOWARNING|HTML_PARSE_NODEFDTD),xmlFreeDoc);check(bool(doc),"Real book presentation parses for link audit");
 std::set<std::string> links;std::vector<xmlNode *> pending{xmlDocGetRootElement(doc.get())};std::size_t count=0;
 while(!pending.empty()){auto *node=pending.back();pending.pop_back();if(!node)continue;check(++count<=200000,"Real book link traversal stays bounded");if(node->next)pending.push_back(node->next);if(node->children)pending.push_back(node->children);if(node->type!=XML_ELEMENT_NODE||xmlStrcmp(node->name,BAD_CAST "a"))continue;auto *value=xmlGetProp(node,BAD_CAST "href");if(!value)continue;std::unique_ptr<xmlChar,decltype(xmlFree)> owned(value,xmlFree);std::string uri=reinterpret_cast<const char *>(owned.get());if(uri.rfind("passagestudy.jsp?",0)==0||uri.rfind("sword://",0)==0||uri.rfind("bible://",0)==0)links.insert(std::move(uri));check(links.size()<=10000,"Real book link count stays bounded");}
 return links;
}
int main(int argc,char **argv)try{
 check(argc==2,"Select a disposable library containing ASV, OSHB, MHCC, StrongsGreek, Pilgrim and KJVA");
 auto original=inspect_legacy_assets(argv[1]);
 StudyWorker worker(argv[1]);auto tab=worker.create_tab().get();
 for(const auto &selection:std::vector<std::pair<std::string,std::string>>{{"ASV","John3:16"},{"OSHB","Gen1:1"}}){
  auto chapter=worker.read_chapter(tab,selection.first,selection.second).get();
  check(!chapter.verses.empty()&&!chapter.html.empty()&&editable_document_text(chapter.html),"Real verse/chapter output must be nonempty UTF-8");
  std::cout<<selection.first<<": chapter verses="<<chapter.verses.size()<<"\n";
 }
 auto apocrypha=worker.read_chapter(tab,"KJVA","Tob1:1").get();check(!apocrypha.verses.empty()&&!apocrypha.html.empty()&&apocrypha.reference.find("Tob")!=std::string::npos,"Real extended canon retains Tobit instead of normalizing into the KJV canon");
 auto pilgrim=worker.search_keyed_module_task(tab,{"Pilgrim","Christian",SearchMode::phrase}).result.get();check(pilgrim.executed&&!pilgrim.references.empty()&&pilgrim.key_kind==SearchKeyKind::general_book,"Real ThML general book returns opaque tree keys");
 for(std::size_t i=0;i<std::min<std::size_t>(3,pilgrim.references.size());++i){auto entry=worker.preview_keyed_search(tab,"Pilgrim",pilgrim.references[i],pilgrim.key_kind,1024*1024).get();check(!entry.html.empty()&&editable_document_text(entry.html),"Real general-book exact hits preview as UTF-8");auto document=worker.select_book_document(tab,{"Pilgrim",pilgrim.references[i]}).get();check(!document.sections.empty()&&document.presentation.has_value(),"Real general-book document selection composes owned sections/presentation");}
 std::vector<std::uint64_t> pending{0};std::set<std::uint64_t> visited;std::size_t documents=0,bookLinks=0;
 for(std::size_t cursor=0;cursor<pending.size();++cursor){check(pending.size()<=4096,"Real book traversal stays bounded");auto offset=pending[cursor];check(visited.insert(offset).second,"Real book traversal contains no cycles or duplicate child addresses");std::optional<std::uint64_t> after;
  do{auto children=worker.book_children("Pilgrim",offset,after,64).get();for(const auto &child:children.children)pending.push_back(child.offset);auto previous=after;after=children.next_after;check(pending.size()<=4096&&(!after||after!=previous),"Real book pagination stays bounded and advances");}while(after);
  auto entry=worker.read_book(tab,"Pilgrim",offset).get();if(entry.has_entry){auto document=worker.select_book_document(tab,{"Pilgrim",offset}).get();check(document.presentation&&editable_document_text(document.presentation->html),"Every populated real book node composes UTF-8 presentation/resources");for(const auto &uri:internal_links(document.presentation->html)){try{(void)worker.follow_book_pane_link_at(tab,"ASV","Gen1:1",uri).get();}catch(const std::exception &error){throw std::runtime_error("Real book link rejected: "+uri+": "+error.what());}++bookLinks;}check(worker.book_document_selection(tab).get()->tree.node.offset==offset,"Real book link previews preserve committed source selection");++documents;}
 }
 check(documents>0&&bookLinks>0,"Populated real book nodes were traversed");std::cout<<"Pilgrim: tree nodes="<<visited.size()<<", populated documents="<<documents<<", internal links="<<bookLinks<<'\n';
 auto commentary=worker.read_passage(tab,"ASV","MHCC","Gen1:1").get();check(commentary.commentary&&!commentary.commentary->html.empty(),"Real linked-verse commentary composes with Bible text");
 for(auto mode:{SearchMode::phrase,SearchMode::words,SearchMode::regular_expression,SearchMode::multiword}){
  auto result=worker.search_module(tab,{"ASV","love",mode}).get();check(result.executed&&!result.references.empty(),"Real ASV text modes execute and return matches");
 }
 auto hebrew=worker.search_module(tab,{"OSHB","אלהים",SearchMode::phrase}).get();check(hebrew.executed&&!hebrew.references.empty(),"Real Hebrew search resolves unpointed UTF-8 query through module filters");
 auto dictionary=worker.search_keyed_module_task(tab,{"StrongsGreek","God",SearchMode::phrase}).result.get();check(dictionary.executed&&!dictionary.references.empty(),"Real compressed dictionary search finds matches");
 for(std::size_t i=0;i<std::min<std::size_t>(3,dictionary.references.size());++i){auto entry=worker.preview_keyed_search(tab,"StrongsGreek",dictionary.references[i],dictionary.key_kind,1024*1024).get();check(!entry.html.empty()&&editable_document_text(entry.html),"Real dictionary exact hits preview as owned UTF-8");}
 auto batch=worker.search_mixed_batch_task(tab,{{"ASV","love",SearchMode::phrase},{"StrongsGreek","God",SearchMode::phrase},{"MHCC","love",SearchMode::phrase},{"Pilgrim","Christian",SearchMode::phrase},{"KJVA","faith",SearchMode::phrase}}).result.get();check(!batch.failures&&batch.copied_results>0,"Real mixed verse/dictionary groups execute");
 SearchAllTextExportRequest request;request.generation=batch.generation.id;request.include_scripture=true;request.max_bytes=maximum_search_export_bytes;
 auto output=worker.export_search_all_text(tab,request).get();check(output.status==SearchExportStatus::ok&&!output.text.empty()&&editable_document_text(output.text),"Real mixed export is complete UTF-8");
 check(inspect_legacy_assets(argv[1]).revision==original.revision,"Real corpus source files remain byte-identical");
 std::cout<<"Real corpus checks passed; mixed rows="<<output.row_count<<"; no source text logged\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
