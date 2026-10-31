// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "bookmarks.hpp"
namespace xiphos::core {
struct BookmarkTagCandidate {
  std::size_t bookmark=0,color_source=0;
  std::string color,foreground,key;
  std::optional<std::string> module,description;
};
inline constexpr std::size_t maximum_bookmark_tag_verses=65536;
inline constexpr std::size_t maximum_bookmark_tag_occurrences=262144;
struct BookmarkTagLimits {
  std::size_t verses=8192,occurrences=32768,snapshot_bytes=maximum_bookmark_snapshot_bytes;
};
enum class BookmarkTagState { generic,ready,missing,unsupported };
struct BookmarkTaggedVerse {
  std::string reference; // Retained fresh VerseKey OSIS book.chapter.verse identity.
  std::vector<std::size_t> tags; // Candidate indices; ordered duplicates remain.
};
struct BookmarkTags {
  std::string control_module,context,revision;
  std::vector<BookmarkTagCandidate> candidates;
  std::vector<BookmarkTagState> states; // One per candidate, exact installed IDs only.
  std::vector<BookmarkTaggedVerse> verses; // First-appearance order.
  bool limited=false;
  std::size_t occurrences=0,skipped_modules=0,skipped_references=0,ignored_subtrees=0,snapshot_bytes=0;
};
bool bookmark_tag_arguments(const std::string &module,const std::string &context,const BookmarkTagLimits &limits);
// Raw metadata plan in retained breadth-first order. Empty folder colors inherit;
// a nonempty nested color overrides. All top-level XML items are beneath the
// GUI's synthetic Bookmarks root. Module eligibility, SDK reference expansion,
// overlap grouping and rendering are separate. Raw colors are never CSS here.
std::vector<BookmarkTagCandidate> bookmark_tag_candidates(const BookmarkSnapshot &snapshot);
// Retained note/subverse marker stripping before verse-list expansion. Preserves
// the legacy broad trailing digit/n replacement, rather than guessing semantics.
std::string bookmark_tag_reference(const std::string &key);
// Exact retained sscanf-prefix/luminance policy, including malformed hex input.
// Returns a static white/black string; no allocation or CSS application.
const char *bookmark_tag_foreground(const char *color) noexcept;
}
