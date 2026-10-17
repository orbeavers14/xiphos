// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "xiphos/core/entry_attribute.hpp"
#include "xiphos/core/note_rendering.hpp"
#include "xiphos/core/note_markers.hpp"
#include <rawgenbook.h>
#include <markupfiltmgr.h>
#include <treekeyidx.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
using namespace sword;
namespace fs=std::filesystem;
#include "legacy_book_document_actual.inc"
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char **argv){try{
 check(argc==2,"Original combined note fixture required");const fs::path root(argv[1]);fs::create_directories(root/"mods.d");fs::create_directories(root/"data");
 for(const auto *id:{"TailLabel","FirstLabel","EmptyTail","NoNotes","AllEmpty","SingleLabel","SingleEmpty"}){
  const auto prefix=(root/"data"/id).string();check(RawGenBook::createModule(prefix.c_str())==0,"Create original combined note book");
  const bool first=std::string(id)=="FirstLabel"||std::string(id)=="SingleLabel",tail=std::string(id)=="TailLabel"||std::string(id)=="EmptyTail",none=std::string(id)=="NoNotes";
  const bool empty=std::string(id)=="AllEmpty"||std::string(id)=="SingleEmpty";
  {RawGenBook writer(prefix.c_str());writer.setKey("/Branch");writer.setEntry((empty?std::string():std::string("Original branch ")+(none?"plain":first?"<note n='first-label'>Original first note</note>":"<note>Original first note</note>")).c_str());writer.setKey("/Branch/Leaf");writer.setEntry((empty?std::string():std::string("Original leaf ")+(none?"plain":tail?"<note n='tail-label'>Original tail note</note>":"<note>Original tail note</note>")).c_str());if(std::string(id)=="EmptyTail"){writer.setKey("/Branch/Empty");writer.setEntry("");}}
  std::ofstream config(root/"mods.d"/(std::string(id)+".conf"));config<<'['<<id<<"]\nDataPath=./data/"<<id<<"\nModDrv=RawGenBook\nSourceType=OSIS\nEncoding=UTF-8\nGlobalOptionFilter=OSISFootnotes\nDisplayLevel="<<(std::string(id).find("Single")==0?1:2)<<'\n';
 }
 fs::remove(root/"mods.d/modules-conf.cache");
 Reader reader(root.string());
 EngineManager legacy(root.string().c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);
 EngineOptions options;options.footnotes=true;ScopedEngineOptions scope(legacy,options);
 for(const auto *id:{"TailLabel","FirstLabel","EmptyTail","NoNotes","AllEmpty","SingleLabel","SingleEmpty"}){
  auto &module=*legacy.getModule(id);set_render_note_numbers(module,true);auto &key=dynamic_cast<TreeKeyIdx &>(*module.getKey());key.setText("/Branch");(void)key.getText();const auto offset=key.getOffset();
  const bool single=std::string(id).find("Single")==0,all_empty=std::string(id)=="AllEmpty";
  SWBuf combined;if(single)combined=module.renderText();else _render_display_level(module,offset,2,1,combined);key.setOffset(offset);module.getRawEntry();
  const auto label=entry_attribute(module,"Footnote","1","n",false);
  const bool tail=std::string(id)=="TailLabel"||std::string(id)=="EmptyTail";
  check(label==(tail?"tail-label":std::string(id)=="SingleLabel"?"first-label":""),"Actual legacy combined cleanup sees last rendered attributes after root restoration, not first section label");
  const auto baseline=reader.read_book_document(id,offset,options);
  check(!baseline.note_source,"Existing Reader document overload retains unavailable metadata");
  const auto captured=reader.read_book_document(id,offset,options,true);
  check(captured.note_source && captured.note_source->reported_label==label && captured.note_source->last_rendered_offset.has_value()==!all_empty && captured.html==combined.c_str(),"Owned opt-in Reader source matches actual legacy combined HTML and terminal label");
  if(captured.note_source->last_rendered_offset){const auto last=reader.book_tree(id,*captured.note_source->last_rendered_offset);
   check(last.node.path==(single?"/Branch":"/Branch/Leaf"),"Last rendered source distinguishes single-entry and skipped empty nodes");}
  check(reader.read_book_document(id,offset,options).html==baseline.html,"Owned SDK label scope restores baseline after success");
  auto small=BookDocumentLimits{};small.max_nodes=1;
  if(!single){try{(void)reader.read_book_document(id,offset,options,true,small);throw std::logic_error("Expected traversal bound");}catch(const std::runtime_error &){}}
  check(reader.read_book_document(id,offset,options).html==baseline.html,"Bounded traversal failure restores owned SDK labels and selected source");
  const auto numbered=number_note_markers(combined.c_str(),true,!label.empty(),{});
  check(!label.empty()?numbered.counters.footnotes==0:(std::string(id)=="NoNotes"||all_empty||std::string(id)=="SingleEmpty")?numbered.counters.footnotes==0:numbered.counters.footnotes>0,"Terminal label controls one combined numbering pass");
 }
 std::cout<<"Actual legacy combined book note source and terminal label scope characterized\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
