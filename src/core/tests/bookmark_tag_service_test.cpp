// SPDX-License-Identifier: GPL-2.0-or-later
#include "bookmark_tag_host.hpp"
#include "xiphos/core/worker.hpp"
#include <markupfiltmgr.h>
using namespace xiphos::core;
template<class E,class F>static void rejection(F f){try{f();}catch(const E &){return;}throw std::runtime_error("Expected bookmark tag service rejection");}
static std::map<std::string,std::string> encoded(const BookmarkTags &tags){std::map<std::string,std::string> result;for(const auto &verse:tags.verses)for(auto index:verse.tags){const auto &candidate=tags.candidates.at(index);auto *label=candidate.description?g_uri_escape_string(candidate.description->c_str(),nullptr,TRUE):nullptr;result[verse.reference]+=candidate.color+"-"+candidate.foreground+"-"+(label?label:"")+"@:@:@";g_free(label);}return result;}
int main(int argc,char **argv){try{
 check(argc==2,"Original installed navigation fixture required");const std::string root=argv[1];const auto file=std::filesystem::u8path(root)/"annotation-tags.xml";std::size_t comparisons=0;BookmarkTags owned;
 {Reader reader(root);SWMgr catalog(root.c_str(),true,new MarkupFilterMgr(FMT_XHTML),false,false);
  for(const auto *control:{"Text","TextAlt","Notes"})for(const auto *target:{"Text","TextAlt","Notes","Personal","Dictionary","Daily","Missing","T",""}){
   const std::string source="<SwordBookmarks><Folder caption='First' color='#123456'><Folder caption='Nested' color='#AA2233'><Bookmark modulename='"+std::string(target)+"' key='John3:16-18;16' description='Original &amp; 文字'/><Bookmark key='John3:17#n1' description='Footnote'/></Folder><Bookmark key='John3:16' description='First'/></Folder><Folder caption='Second' color='#33CC44'><Bookmark key='John3:16' description='Second'/></Folder></SwordBookmarks>";
   {std::ofstream out(file);out<<source;}rows.clear();rows.push_back({});GtkTreeIter synthetic{0};actual::gui_parse_bookmarks(nullptr,BAD_CAST file.u8string().c_str(),&synthetic);
   auto *module=catalog.getModule(control);BackEnd baseline{*module,{},&catalog};backend=&baseline;settings.MainWindowModule=control;settings.currentverse="John3:16";auto *key=dynamic_cast<VerseKey *>(module->getKey());check(key,"Installed verse control");key->setText("John4:2");
   frozen_tags::build_tag_color_map(key);const auto expected=copied_map(frozen_tags::tag_color_map);const auto before=reader.navigation_target(control,"24").reference;
   owned=reader.bookmark_tags(parse_bookmarks(source),control,"John3:16");check(encoded(owned)==expected&&!owned.limited,"Owned exact-catalog tag map matches complete retained SDK map");
   check(reader.navigation_target(control,"24").reference==before,"Owned annotation map preserves live module key");
   check(owned.candidates.size()==4&&owned.states.size()==4&&owned.verses.size()>=2&&owned.verses.size()<=3&&owned.occurrences>=3&&owned.snapshot_bytes<=maximum_bookmark_snapshot_bytes,"Owned candidate/state/grouped occurrence shape");
   const auto expected_state=std::string(target).empty()?BookmarkTagState::generic:std::string(target)=="Missing"||std::string(target)=="T"?BookmarkTagState::missing:std::string(target)=="Personal"||std::string(target)=="Dictionary"||std::string(target)=="Daily"?BookmarkTagState::unsupported:BookmarkTagState::ready;
   check(owned.states[2]==expected_state&&owned.skipped_modules==static_cast<size_t>(expected_state==BookmarkTagState::missing||expected_state==BookmarkTagState::unsupported),"Actual installed exact-ID Bible/regular/personal/dictionary/devotional/missing/alias eligibility");++comparisons;
  }
  const auto snapshot=parse_bookmarks("<SwordBookmarks><Folder caption='Tags' color='#123456'><Bookmark key='John3:16-18;16'/></Folder></SwordBookmarks>");
  const auto full=reader.bookmark_tags(snapshot,"Text","John3:16");check(full.occurrences==4&&full.verses.size()==3&&full.verses[0].tags.size()==2,"Repeated bookmark occurrences remain ordered duplicates");
  auto limits=BookmarkTagLimits{};limits.occurrences=4;check(!reader.bookmark_tags(snapshot,"Text","John3:16",limits).limited,"Exact complete occurrence bound does not claim truncation");limits.occurrences=3;const auto prefix=reader.bookmark_tags(snapshot,"Text","John3:16",limits);check(prefix.limited&&prefix.occurrences==3&&prefix.verses.size()==3,"Next occurrence beyond cap discloses ordered prefix");
  limits={};limits.verses=1;const auto one=reader.bookmark_tags(snapshot,"Text","John3:16",limits);check(one.limited&&one.occurrences==1&&one.verses.size()==1,"Distinct-verse limit retains ordered prefix");
  std::size_t reference_bytes=0;for(const auto &verse:full.verses)reference_bytes+=verse.reference.size();const auto metadata_bytes=full.snapshot_bytes-reference_bytes;
  limits={};limits.snapshot_bytes=metadata_bytes+full.verses[0].reference.size()-1;const auto no_room=reader.bookmark_tags(snapshot,"Text","John3:16",limits);check(no_room.limited&&no_room.verses.empty()&&!no_room.occurrences,"Byte-limited map discloses missing first verse after complete metadata");
  limits.snapshot_bytes=metadata_bytes+full.verses[0].reference.size();const auto exact_first=reader.bookmark_tags(snapshot,"Text","John3:16",limits);check(exact_first.limited&&exact_first.verses.size()==1&&exact_first.occurrences==1&&exact_first.snapshot_bytes==limits.snapshot_bytes,"Exact byte bound retains first complete verse");
  limits={};limits.verses=1;const auto duplicate_only=reader.bookmark_tags(parse_bookmarks("<SwordBookmarks><Folder caption='Tags' color='#123456'><Bookmark key='John3:16;16'/></Folder></SwordBookmarks>"),"Text","John3:16",limits);check(!duplicate_only.limited&&duplicate_only.verses.size()==1&&duplicate_only.occurrences==2,"Existing-verse duplicates do not consume another distinct-verse slot");
  limits={};limits.snapshot_bytes=1;rejection<std::invalid_argument>([&]{reader.bookmark_tags(snapshot,"Text","John3:16",limits);});
  limits={};limits.occurrences=0;rejection<std::invalid_argument>([&]{reader.bookmark_tags(snapshot,"Text","John3:16",limits);});
  for(const auto *control:{"Dictionary","Personal","Daily"})rejection<std::invalid_argument>([&]{reader.bookmark_tags(snapshot,control,"John3:16");});
  for(const auto *control:{"Missing","T"})rejection<std::out_of_range>([&]{reader.bookmark_tags(snapshot,control,"John3:16");});
  rejection<std::invalid_argument>([&]{reader.bookmark_tags(snapshot,"Text",std::string("John3:16\0tail",13));});
  const auto empty=reader.bookmark_tags(parse_bookmarks("<SwordBookmarks/>"),"Text","John3:16");check(empty.verses.empty()&&empty.candidates.empty()&&!empty.limited,"Empty bookmark source gives empty annotation map");
  frozen_tags::free_tag_color_map();
 }
 check(!owned.verses.empty()&&owned.candidates[0].color=="#123456","Owned tag results outlive Reader/catalog");
 {StudyWorker worker(root);const auto tab=worker.create_tab().get(),sibling=worker.create_tab().get();
  EngineOptions options;options.strongs=true;worker.set_engine_options(tab,options).get();
  const auto original_bytes=bytes(file);
  const auto snapshot=parse_bookmarks(bytes(file));std::string path=file.u8string(),module="Text",context="John3:16";auto pending=worker.read_bookmark_tags(tab,path,module,context);path.clear();module.clear();context.clear();
  const auto tags=pending.get();check(tags.revision==snapshot.revision&&!tags.verses.empty(),"FIFO annotation read copies path/context and returns source revision");
  check(worker.history(tab).get().entries.empty()&&worker.history(sibling).get().entries.empty()&&!worker.parallel_selection(tab).get()&&!worker.search_generation(tab).get().id,"Annotation reads preserve histories, siblings, search and selection");
  check(worker.module_engine_options(tab,"Text").get()->strongs&&!worker.module_engine_options(sibling,"Text").get()&&bytes(file)==original_bytes,"Annotation queries preserve explicit options, unconfigured sibling options and exact source bytes");
  auto invalid=worker.read_bookmark_tags(tab,file.u8string(),"Text",std::string(32769,'x'));check(invalid.wait_for(std::chrono::seconds(0))==std::future_status::ready,"Oversized annotation context rejects before FIFO");rejection<std::invalid_argument>([&]{invalid.get();});
  worker.close_tab(tab).get();rejection<std::invalid_argument>([&]{worker.read_bookmark_tags(tab,file.u8string(),"Text","John3:16").get();});
 }
 std::cout<<"Owned annotation service: "<<comparisons<<" complete real-catalog/SDK map comparisons, copied FIFO reads, exact eligibility, duplicate overlap groups and disclosed limits passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
