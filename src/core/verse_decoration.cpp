// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/verse_decoration.hpp"
#include <stdexcept>
namespace xiphos::core {
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
