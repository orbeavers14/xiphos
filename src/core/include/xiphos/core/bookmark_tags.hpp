// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "bookmarks.hpp"
namespace xiphos::core {
struct BookmarkTagCandidate {
  std::size_t bookmark=0,color_source=0;
  std::string color,foreground,key;
  std::optional<std::string> module,description;
};
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
