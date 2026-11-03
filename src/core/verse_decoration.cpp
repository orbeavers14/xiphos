// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/verse_decoration.hpp"
#include <stdexcept>
namespace xiphos::core {
namespace {
std::string rgb(const std::string &raw){
 if(raw.empty()||raw[0]!='#'||(raw.size()!=4&&raw.size()!=7&&raw.size()!=13))return {};
 auto digit=[](char c)->int{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;};
 for(std::size_t i=1;i<raw.size();++i)if(digit(raw[i])<0)return {};
 const char hex[]="0123456789abcdef";std::string result="#";
 for(std::size_t c=0;c<3;++c){unsigned value=0;if(raw.size()==4)value=17*digit(raw[c+1]);else{auto width=raw.size()==7?2:4;for(int k=0;k<width;++k)value=value*16+digit(raw[1+c*width+k]);if(width==4)value=(value+128)/257;}result+=hex[value>>4];result+=hex[value&15];}return result;
}
}
std::string display_decorated_verse(const std::string &html,const VerseDecoration &plan,std::size_t &invalid_colors,std::size_t limit){
 if(!limit||limit>maximum_decorated_reading_bytes)throw std::invalid_argument("Invalid decorated reading limit");
 std::string result;auto add=[&](const std::string &value){if(value.size()>limit-result.size())throw std::length_error("Decorated reading exceeds byte limit");result+=value;};
 add("<div class=\"xc-reading-decoration\">");bool tag=false;
 if(!plan.tags.empty()){auto background=rgb(plan.tags[0].color),foreground=rgb(plan.tags[0].foreground);if(background.empty()||foreground.empty())++invalid_colors;else{tag=true;add("<div class=\"xc-reading-tag\" style=\"background-color:"+background+";color:"+foreground+"\">");}}
 const char *color="";switch(plan.inner_color){case XH_VERSE_COLOR_NONE:break;case XH_VERSE_COLOR_ANNOTATION:color=" xc-reading-annotation";break;case XH_VERSE_COLOR_CURRENT_HIGHLIGHT:color=" xc-reading-current-highlight";break;case XH_VERSE_COLOR_CURRENT_TEXT:color=" xc-reading-current-text";break;default:throw std::invalid_argument("Invalid reading highlight plan");}
 add(std::string("<div class=\"xc-reading-inner")+color+"\">");add(html);add("</div>");if(tag)add("</div>");
 if(!plan.note_html.empty()){add("<details class=\"xc-personal-note\"><summary>Personal note</summary><div>");add(plan.note_html);add("</div></details>");}add("</div>");return result;
}
VerseDecoration compose_verse_decoration(const AnnotationTarget &target,const BookmarkTags &tags,const AnnotationCache &notes,bool current,bool annotation_highlight,bool verse_highlight){
 if(target.control_module!=tags.control_module||target.control_module!=notes.module||target.address.book!=notes.book||tags.states.size()!=tags.candidates.size())throw std::invalid_argument("Decoration source contexts differ");
 VerseDecoration result;result.module=target.control_module;result.reference=target.address.book+"."+std::to_string(target.address.chapter)+"."+std::to_string(target.address.verse);result.incomplete=tags.limited||notes.incomplete||tags.skipped_modules||tags.skipped_references||tags.ignored_subtrees;
 std::size_t copied=0;
 auto account=[&](const BookmarkTagCandidate &tag){std::size_t size=0;auto add=[&](const std::string &value){if(value.size()>maximum_bookmark_snapshot_bytes-size)return false;size+=value.size();return true;};if(!add(tag.color)||!add(tag.foreground)||!add(tag.key)||(tag.module&&!add(*tag.module))||(tag.description&&!add(*tag.description))||(tag.source_versification&&!add(*tag.source_versification))||size>maximum_bookmark_snapshot_bytes-copied)return false;copied+=size;return true;};
 bool found=false;for(const auto &row:tags.verses)if(row.reference==result.reference){if(found)throw std::invalid_argument("Duplicate decoration verse");found=true;for(auto i:row.tags){if(i>=tags.candidates.size()||tags.states[i]==BookmarkTagState::missing||tags.states[i]==BookmarkTagState::unsupported)throw std::invalid_argument("Invalid decoration tag index");if(result.tags.size()==maximum_bookmark_tag_occurrences||!account(tags.candidates[i])){result.incomplete=true;break;}result.tags.push_back(tags.candidates[i]);}}
 auto note=annotation_cache_index(notes,target.address.chapter,target.address.verse);if(note)result.note_html=notes.entries[*note].html;
 result.inner_color=xh_choose_verse_color(annotation_highlight,note.has_value(),current,!result.tags.empty(),verse_highlight);return result;
}
}
