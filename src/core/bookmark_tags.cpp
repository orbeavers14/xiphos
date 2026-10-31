// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/bookmark_tags.hpp"
#include "xml_document.hpp"
#include <cstdio>
#include <cstring>
#include <deque>
#include <stdexcept>
namespace xiphos::core {
const char *bookmark_tag_foreground(const char *color) noexcept {
  if(!color||color[0]!='#'||std::strlen(color)<7)return "#000000";
  unsigned r=0,g=0,b=0;
  (void)std::sscanf(color+1,"%02x%02x%02x",&r,&g,&b);
  const double luminance=0.2126*r+0.7152*g+0.0722*b;
  return luminance<128.0?"#FFFFFF":"#000000";
}
std::string bookmark_tag_reference(const std::string &key) {
  detail::validate_xml_text(key,maximum_bookmark_text_bytes);std::string result=key;
  auto *text=result.data();char *marker;
  while((marker=std::strpbrk(text,"#!"))) {
    *(marker++)=' ';
    while((marker=std::strpbrk(marker,"n0123456789"))) *(marker++)=' ';
  }
  return result;
}
std::vector<BookmarkTagCandidate> bookmark_tag_candidates(const BookmarkSnapshot &snapshot) {
  if(snapshot.items.size()>maximum_bookmark_items)throw std::invalid_argument("Too many bookmark tag nodes");
  const auto count=snapshot.items.size();std::vector<std::vector<std::size_t>> children(count+1);
  std::vector<std::size_t> depths(count),positions(count+1);std::size_t bytes=0;
  auto string=[&](const std::string &value){
    detail::validate_xml_text(value,maximum_bookmark_text_bytes);
    if(value.size()>maximum_bookmark_snapshot_bytes-bytes)throw std::invalid_argument("Bookmark tag snapshot exceeds bounds");bytes+=value.size();
  };
  auto text=[&](const std::optional<std::string> &value){if(value)string(*value);};
  text(snapshot.syntax_version);string(snapshot.revision);
  for(std::size_t i=0;i<count;++i){const auto &item=snapshot.items[i];
    if(item.kind!=BookmarkKind::folder&&item.kind!=BookmarkKind::bookmark)throw std::invalid_argument("Invalid bookmark tag kind");
    if(item.parent&&(*item.parent>=i||snapshot.items[*item.parent].kind!=BookmarkKind::folder))throw std::invalid_argument("Invalid bookmark tag parent");
    const auto parent=item.parent.value_or(count);depths[i]=item.parent?depths[*item.parent]+1:1;
    if(depths[i]>maximum_bookmark_depth||item.position!=positions[parent]++)throw std::invalid_argument("Invalid bookmark tag depth/order");
    if(item.kind==BookmarkKind::folder?(item.module||item.key||item.module_description||item.description):(item.caption||item.color))
      throw std::invalid_argument("Bookmark tag fields do not match node kind");
    for(const auto *value:{&item.caption,&item.color,&item.module,&item.key,&item.module_description,&item.description,&item.label})text(*value);
    children[parent].push_back(i);
  }
  struct Pending{std::size_t index;std::optional<std::size_t> color;};std::deque<Pending> queue;
  for(auto index:children[count])queue.push_back({index,std::nullopt});
  std::vector<BookmarkTagCandidate> result;std::size_t output_bytes=0;
  auto own=[&](const std::string &value){if(value.size()>maximum_bookmark_snapshot_bytes-output_bytes)throw std::invalid_argument("Bookmark tag plan exceeds bounds");output_bytes+=value.size();};
  while(!queue.empty()){
    const auto next=queue.front();queue.pop_front();const auto &item=snapshot.items[next.index];auto color=next.color;
    if(item.color&&!item.color->empty())color=next.index;
    if(item.key){
      if(color){const auto &value=*snapshot.items[*color].color;auto key=bookmark_tag_reference(*item.key);
        const std::string foreground=bookmark_tag_foreground(value.c_str());own(value);own(foreground);own(key);if(item.module)own(*item.module);if(item.description)own(*item.description);
        result.push_back({next.index,*color,value,foreground,std::move(key),item.module,item.description});
      }
    } else for(auto child:children[next.index])queue.push_back({child,color});
  }
  return result;
}
}
