// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
bool same_preferences(const ModulePresentationOptions &a,const ModulePresentationOptions &b){
 return a.image_content==b.image_content&&a.respect_font_faces==b.respect_font_faces&&a.note_numbers==b.note_numbers&&
  a.note_numbering_available==b.note_numbering_available&&a.double_space==b.double_space&&a.headings==b.headings&&
  a.commentary_by_chapter==b.commentary_by_chapter;
}
bool same_source(const Entry &a,const Entry &b){
 return a.reference==b.reference&&a.html==b.html&&a.heading_html==b.heading_html&&a.lexical.css==b.lexical.css&&
  a.lexical.requested==b.lexical.requested&&a.lexical.applied==b.lexical.applied&&
  a.note_source.captured==b.note_source.captured&&a.note_source.reported_label==b.note_source.reported_label&&
  a.note_source.headings.size()==b.note_source.headings.size();
}
int main(int argc,char **argv){try{
 check(argc==3,"Original lexical and note fixtures required");Entry held;
 {
  Reader reader(argv[1]);StudySession session(reader),other(reader);
  const auto other_baseline=other.read_entry_snapshot("Text","John4:1");
  session.enable_module_preferences();session.set_lexical_layout_enabled(true);session.set_note_numbering_enabled(true);session.set_heading_note_links_enabled(true);
  EngineOptions on;on.strongs=true;session.set_module_engine_options("Text",on);session.set_module_engine_options("ThMLNotes",on);
  session.set_double_space("Text",true);session.visit("Text","John3:17");
  const auto searched=session.search_module({"Text","Original"},{1,1024});auto generation=searched.generation;
  check(searched.effective_expression=="Original"&&searched.executed,"Real ThML prepared query excludes SDK's counted terminator");
  const auto plain=session.export_search_visible_text({generation.id,{{0,0}},"Original ThML export",true,8192});
  check(plain.status==SearchExportStatus::ok&&plain.text.find("Original")!=std::string::npos,"Real ThML scripture strip text shares counted-terminator handling");
  const auto history=session.history().entries().size(),cursor=session.history().cursor();const auto scope=session.last_search_scope();
  const auto retained_before=session.read_search_generation(generation.id,0).result->references;
  const auto before=session.module_presentation_options("Text");
  check(before.image_content==ContentPreference::unknown&&before.respect_font_faces==ContentPreference::unknown,"Fixture starts without learned content preferences");
  held=session.read_entry_snapshot("Text","John4:1");auto expected=session;const auto live=expected.read_entry("Text","John4:1");
  const auto bounded=session.capture_entry_html("Text","John4:1");
  check(bounded.module=="Text"&&bounded.reference==held.reference&&bounded.html==held.html&&bounded.css==held.lexical.css&&
   bounded.lexical_applied&&bounded.copied_bytes==bounded.module.size()+bounded.reference.size()+bounded.html.size()+bounded.css.size(),"Bounded body source carries exact matching HTML/CSS and aggregate bytes");
  check(session.capture_entry_html("Text","John4:1",{bounded.copied_bytes}).html==bounded.html,"Exact aggregate HTML source limit accepted");
  bool clipped=false;try{session.capture_entry_html("Text","John4:1",{bounded.copied_bytes-1});}catch(const std::length_error &){clipped=true;}
  check(clipped&&same_preferences(session.module_presentation_options("Text"),before),"One-byte-short source fails without publishing learned display choices");
  bool raw_clipped=false;try{session.capture_entry_html("Text","John4:1",{4});}catch(const std::length_error &){raw_clipped=true;}
  check(raw_clipped&&same_preferences(session.module_presentation_options("Text"),before),"SDK body guard also restores physical/logical state before aggregate source publication");
  for(auto cap:{std::size_t{0},maximum_html_source_bytes+1}){bool bad=false;try{session.capture_entry_html("Text","John4:1",{cap});}catch(const std::invalid_argument &){bad=true;}check(bad,"Invalid HTML source budget");}
  for(const auto &bad:std::vector<std::string>{std::string("bad\0tail",8),std::string("\xff",1),std::string(4097,'x')}){
   bool rejected=false;try{session.capture_entry_html("Text",bad);}catch(const std::invalid_argument &){rejected=true;}check(rejected,"Invalid HTML source reference rejects before SDK reads");
  }
  check(same_source(held,live)&&held.lexical.applied&&!held.lexical.css.empty()&&held.note_source.captured,"Snapshot uses the exact configured renderer, matching styles, cleanup and note source");
  check(same_preferences(session.module_presentation_options("Text"),before)&&
   expected.module_presentation_options("Text").image_content==ContentPreference::on&&expected.module_presentation_options("Text").respect_font_faces==ContentPreference::on,"Snapshot does not publish automatic image/font discovery that a live read publishes");
  const auto comment_before=session.module_presentation_options("ThMLNotes");
  const auto comment=session.read_commentary_entry_snapshot("ThMLNotes","John4:1");auto expected_comment=session;
  check(same_source(comment,expected_comment.read_commentary_entry("ThMLNotes","John4:1"))&&!comment.heading_html.empty()&&
   same_preferences(session.module_presentation_options("ThMLNotes"),comment_before),"Commentary source keeps heading/body CSS together without preference publication");
  session.set_image_content("Text",false);session.set_respect_font_faces("Text",false);
  const auto explicit_before=session.module_presentation_options("Text");
  const auto cleaned=session.read_entry_snapshot("Text","John4:1");auto expected_clean=session;
  check(same_source(cleaned,expected_clean.read_entry("Text","John4:1"))&&cleaned.html.find("<img ")==std::string::npos&&
   same_preferences(session.module_presentation_options("Text"),explicit_before),"Explicit content cleanup stays shared and independent of snapshot commit mode");
  const auto fallback=session.read_entry_snapshot("Text","John6:1");
  check(fallback.lexical.requested&&!fallback.lexical.applied&&fallback.lexical.css.find(".word")==std::string::npos,"Unsupported source retains matching fallback CSS without invented layout");
  bool missing=false;try{session.read_entry_snapshot("Missing","John1:1");}catch(const std::out_of_range &){missing=true;}
  check(missing&&same_preferences(session.module_presentation_options("Text"),explicit_before)&&
   session.history().entries().size()==history&&session.history().cursor()==cursor&&session.search_generation().id==generation.id&&
   session.last_search_scope().element_count==scope.element_count&&session.read_search_generation(generation.id,0).result->references==retained_before&&!session.search_text_options().options&&!session.book_selection(),"Success/failure preserve logical preferences, reading history, selection and search generation");
  check(!other.module_presentation_options("Text").double_space&&same_source(other.read_entry_snapshot("Text","John4:1"),other_baseline),"Snapshot respects independent unconfigured tab policy");
 }
 {
  Reader reader(argv[2]);StudySession session(reader);session.enable_module_preferences();session.set_note_numbering_enabled(true);
  EngineOptions engine;engine.footnotes=true;session.set_module_engine_options("Text",engine);
  for(bool enabled:{false,true}){
   session.set_note_numbers("Text",enabled);const auto before=session.module_presentation_options("Text");
   const auto source=session.read_entry_snapshot("Text","John4:5");auto expected=session;
   check(source.note_source.captured&&same_source(source,expected.read_entry("Text","John4:5"))&&same_preferences(session.module_presentation_options("Text"),before),"Real SDK note labels and numbering use the shared source/presentation pipeline");
  }
 }
 std::future<Entry> drained;std::future<EntryHtmlSource> bounded_drained;
 {
  StudyWorker worker(argv[1]);const auto tab=worker.create_tab().get();worker.set_lexical_layout_enabled(tab,true).get();
  worker.enable_module_preferences(tab).get();EngineOptions on;on.strongs=true;worker.set_module_engine_options(tab,"Text",on).get();
  drained=worker.read_entry_snapshot(tab,"Text","John4:1");bounded_drained=worker.capture_entry_html(tab,"Text","John4:1");worker.close_tab(tab).get();
 }
 check(drained.get().lexical.applied&&bounded_drained.get().lexical_applied&&held.html.find("Original")!=std::string::npos&&!held.lexical.css.empty(),"FIFO owner drains configured snapshots and copied HTML/CSS outlives all engine state");
 std::cout<<"Configured HTML/CSS sources, no preference publication, notes, fallback and owned worker lifetime passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
