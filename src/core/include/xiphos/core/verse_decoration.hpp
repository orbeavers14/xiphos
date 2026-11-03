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
inline constexpr std::size_t maximum_decorated_reading_bytes=16*1024*1024;
struct DecoratedReading {std::string html,css;bool incomplete=false;std::size_t invalid_colors=0;};
// Shared bounded presentation. Only exact RGB hex forms (#RGB/#RRGGBB/GDK16)
// enter CSS; raw metadata never does. The first tag retains priority even when
// its color is invalid. Theme colors remain host-owned CSS variables/classes.
std::string display_decorated_verse(const std::string &html,const VerseDecoration &plan,std::size_t &invalid_colors,std::size_t limit=maximum_decorated_reading_bytes);
VerseDecoration compose_verse_decoration(const AnnotationTarget &target,const BookmarkTags &tags,const AnnotationCache &notes,bool current,bool annotation_highlight,bool verse_highlight);
}
