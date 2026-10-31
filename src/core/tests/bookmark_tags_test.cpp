// SPDX-License-Identifier: GPL-2.0-or-later
#include "bookmark_tag_host.hpp"
int main(int argc,char **argv){try{
 check(argc==2,"Disposable tag root required");const auto root=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(root);const auto module_root=root/"module";std::filesystem::create_directories(module_root);check(RawText::createModule(module_root.u8string().c_str())==0,"Create original empty SDK verse module");RawText module(module_root.u8string().c_str());BackEnd owner{module,{}};backend=&owner;auto *key=dynamic_cast<VerseKey *>(module.getKey());check(key,"Verse key fixture");model=reinterpret_cast<void *>(1);
 const auto file=root/"tags.xml";std::size_t cases=0;
 for(const auto *outer:{"#123456","","#FFFFFF"})for(const auto *inner:{"#AA0000","","#000000"})for(const auto *target:{"Text","Notes","Dictionary","Personal","Missing",""}){
  const std::string leaf=std::string("<Bookmark modulename='")+target+"' key='John3:16-18' description='Original &amp; 文字'/>";
  const std::string source="<SwordBookmarks><Folder caption='First' color='"+std::string(outer)+"'><Folder caption='Deep' color='"+inner+"'>"+leaf+"<Bookmark key='John3:17#n1' description='Note'/></Folder>"+leaf+"</Folder><Folder caption='Second' color='#33CC44'><Bookmark key='John3:16' description='Other'/></Folder></SwordBookmarks>";
  {std::ofstream out(file);out<<source;}rows.clear();rows.push_back({});GtkTreeIter synthetic{0};actual::gui_parse_bookmarks(nullptr,BAD_CAST file.u8string().c_str(),&synthetic);
  const auto plan=bookmark_tag_candidates(parse_bookmarks(source));std::vector<TagCall> expected_calls;const auto expected=planned_map(plan,module,expected_calls);
  key->setText("John4:2");const auto before=std::string(key->getText());owner.calls.clear();frozen_tags::build_tag_color_map(key);const auto baseline=copied_map(frozen_tags::tag_color_map);const auto baseline_calls=owner.calls;
  check(std::string(key->getText())==before,"Complete retained map restores live verse key");owner.calls.clear();actual_tags::build_tag_color_map(key);
  check(copied_map(actual_tags::tag_color_map)==baseline&&owner.calls==baseline_calls,"Complete frozen/current tag map and SDK parse call ordering match");
  check(expected==baseline&&expected_calls==baseline_calls,"Owned candidate BFS/inheritance/marker stripping reproduces complete tag map, module filtering and ordered overlaps");
  key->setText("John3:16");auto *lookup=actual_tags::get_tag_color_for_versekey(key);check((lookup?std::string(lookup):"")==baseline.at("John.3.16"),"Complete retained map lookup preserves all ordered occurrences");g_free(lookup);++cases;
 }
 for(const auto *color:{static_cast<const char *>(nullptr),"","red","#123","#000000","#FFFFFF","#808080","#7F7F7F","#ff0000","#00ff00","#0000ff","#ZZZZZZ","#12xx34","#fffffftrailing"})check(std::string(bookmark_tag_foreground(color))==frozen_tags::text_color_for_bg(color)&&std::string(actual_tags::text_color_for_bg(color))==frozen_tags::text_color_for_bg(color),"Exact retained foreground prefix/luminance behavior");
 check(bookmark_tag_reference("John3:16#n1, John3:18")=="John3:16   , Joh  :  ","Marker stripping retains broad legacy trailing digit/n behavior");
 const auto simple=parse_bookmarks("<SwordBookmarks><Folder caption='A' color='#123456'><Bookmark key='John3:16'/></Folder></SwordBookmarks>");auto invalid=simple;invalid.items[1].parent=1;rejects([&]{bookmark_tag_candidates(invalid);});invalid=simple;invalid.items[1].position=2;rejects([&]{bookmark_tag_candidates(invalid);});invalid=simple;invalid.items[1].color="#fff";rejects([&]{bookmark_tag_candidates(invalid);});invalid=simple;invalid.items[0].color=std::string(32769,'x');rejects([&]{bookmark_tag_candidates(invalid);});
 settings.tag_colorize=false;actual_tags::build_tag_color_map(key);check(copied_map(actual_tags::tag_color_map).empty()&&!actual_tags::get_tag_color_for_versekey(key),"Disabled retained tags clear cache and suppress lookup");actual_tags::free_tag_color_map();frozen_tags::free_tag_color_map();
 std::cout<<"Bookmark tags: "<<cases<<" complete frozen/current real-SDK maps, BFS inheritance, module eligibility, overlap order, marker stripping and foreground comparisons passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
