// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "annotations.hpp"
#include "bookmark_tags.hpp"
#include "verse_color.h"
namespace xiphos::core {
struct VerseDecoration {
 std::string module,reference,note_html;
 std::vector<BookmarkTagCandidate> tags; // First tag color wins; all ordered occurrences retained.
 xh_verse_color inner_color=XH_VERSE_COLOR_NONE;
 bool incomplete=false;
};
// Owned presentation plan only. Uses the retained color priority and already
// escaped note HTML. Does not inject raw color/metadata into HTML or mutate tabs.
VerseDecoration compose_verse_decoration(const AnnotationTarget &target,const BookmarkTags &tags,const AnnotationCache &notes,bool current,bool annotation_highlight,bool verse_highlight);
}
