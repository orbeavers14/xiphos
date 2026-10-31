// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>
namespace xiphos::core {
inline constexpr std::size_t maximum_bookmark_items=10000;
inline constexpr std::size_t maximum_bookmark_depth=64;
inline constexpr std::size_t maximum_bookmark_text_bytes=32768;
inline constexpr std::size_t maximum_bookmark_snapshot_bytes=16*1024*1024;
enum class BookmarkKind { folder,bookmark };
struct BookmarkItem {
  BookmarkKind kind=BookmarkKind::bookmark;
  std::optional<std::size_t> parent; // Earlier flat item; absent means document root.
  std::size_t position=0; // Recognized sibling order, not a persistent identity.
  std::optional<std::string> caption,color,module,key,module_description,description;
  std::optional<std::string> label; // Derived legacy presentation; absent if unsafe.
};
struct BookmarkSnapshot {
  std::optional<std::string> syntax_version;
  std::string revision; // SHA-256 of exact source bytes; future stale-edit guard.
  std::vector<BookmarkItem> items; // Depth-first folder/bookmark order.
  std::size_t ignored_subtrees=0,snapshot_bytes=0;
};
// Explicit SwordBookmarks XML subset, with copied raw identities and optional
// values. Folder color is raw metadata, never validated/applied as CSS. Leaf color
// is ignored as in legacy. Unknown element subtrees are counted and skipped;
// named Folder nodes with missing captions remain typed, without unsafe UI attach.
// No module resolution, URI navigation, home discovery, mutation or rendering.
BookmarkSnapshot parse_bookmarks(const std::string &xml);
BookmarkSnapshot read_bookmarks(const std::string &path);
}
