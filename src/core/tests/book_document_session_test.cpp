// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected document selection failure");}
int main(int argc,char **argv){try{
 check(argc==2,"Original document fixture required");Reader reader(argv[1]);StudySession a(reader),b(reader);EngineOptions on;on.footnotes=true;on.words_in_red=true;a.set_engine_options(on);
 const std::string uri="passagestudy.jsp?action=showNote&type=n&value=1&module=Document5&passage=%2FBranch%2FLeaf";
 rejects<std::invalid_argument>([&]{a.book_document_note_link(uri);});
 auto selected=a.select_book_document({"Document5",std::uint64_t(4)});
 check(selected.sections.size()==5 && selected.html.find("Original descendant") == std::string::npos && a.book_selection()->document->html==selected.html,"Document publication owns selected source and all configured sections");
 const auto target=book_document_note_link(uri,selected);check(target.offset==8 && target.command.reference=="/Branch/Leaf","Descendant note resolves to its exact rendered source");
 auto note=a.book_document_note_link(uri);check(note.tree.node.offset==8 && note.note.html.find("Original descendant note")!=std::string::npos && a.book_selection()->tree.node.offset==4,"Document note reads descendant without changing selected root");
 for(const auto &bad:{"passagestudy.jsp?action=showNote&type=n&value=1&module=Document5&passage=%2FBranch%2FEmpty", "passagestudy.jsp?action=showNote&type=n&value=1&module=Document5&passage=%2FLinked", "passagestudy.jsp?action=showNote&type=n&value=1&module=Document4&passage=%2FBranch%2FLeaf", "https://example.invalid"})rejects<std::invalid_argument>([&]{a.book_document_note_link(bad);});
 auto duplicate=selected;duplicate.sections.push_back(duplicate.sections[1]);rejects<std::invalid_argument>([&]{book_document_note_link(uri,duplicate);});
 auto metadata=a.book_document_note_link(uri,false);check(metadata.note.html.empty() && metadata.note.identifier==note.note.identifier,"Document note metadata/body share exact source policy");
 rejects<std::runtime_error>([&]{a.select_book_document({"TooDeep",std::uint64_t(4)});});
 rejects<std::out_of_range>([&]{a.restore_book_document_selection({"Missing","4",{}},BookRestoreMode::display_key);});
 rejects<std::invalid_argument>([&]{a.navigate_book_document(static_cast<BookNavigation>(99));});
 check(a.book_selection()->document->html==selected.html && a.book_selection()->tree.module=="Document5" && !b.book_selection(),"Traversal/restore/command failures retain atomic owned document and other tab state");
 check(!a.restore_book_document_selection({},BookRestoreMode::display_key) && a.book_selection()->document->html==selected.html,"Absent document restore is a no-op");
 (void)a.read_book("Document5",8);(void)a.read_book_document("Document5",0);check(a.book_selection()->document->html==selected.html,"Raw document/entry reads preserve committed document");
 auto child=a.navigate_book_document(BookNavigation::first_child);check(child->tree.node.offset==8 && child->sections.size()==2,"Document navigation reuses shared relation policy");
 check(a.navigate_book_document(BookNavigation::parent)->tree.node.offset==4 && a.navigate_book_document(BookNavigation::parent)->tree.node.offset==4,"Parent and legacy numeric root reading minimum are shared");
 auto root=a.restore_book_document_selection({"Document5","/","4"},BookRestoreMode::display_key);check(root->tree.node.offset==0,"Explicit document path root remains root");
 check(!a.navigate_book_document(BookNavigation::parent) && a.book_selection()->tree.node.offset==0,"Root parent no-op preserves document");
 check(a.restore_book_document_selection({"Document5","/","8"},BookRestoreMode::initial_window)->tree.node.offset==8,"Initial restore uses separate saved offset");
 (void)a.select_book({"Document5",std::uint64_t(4)});check(!a.book_selection()->document,"Successful single-entry publication invalidates older document context");rejects<std::invalid_argument>([&]{a.book_document_note_link(uri);});
 check(a.history().entries().empty() && a.dictionary_history().entries().empty(),"Document selection and notes do not create Bible/dictionary visits");
 check(selected.tree.node.offset==4 && note.tree.node.offset==8,"Copied document/note retain independent root and descendant identities");
 std::cout<<"Atomic document selection, shared navigation and exact rendered-section note routing passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
