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
struct BookmarkFields {
  std::optional<std::string> caption,color,module,key,module_description,description;
};
enum class BookmarkAction { insert,update,remove,move };
struct BookmarkEdit {
  BookmarkAction action=BookmarkAction::insert;
  BookmarkKind kind=BookmarkKind::bookmark; // insert/update only.
  std::optional<std::size_t> target,parent;
  std::size_t position=0; // Destination recognized siblings, after removing moved target.
  BookmarkFields fields; // Complete replacement of known fields, insert/update only.
};
void validate_bookmark_edit(const BookmarkEdit &edit);
void validate_bookmark_edit_request(const std::optional<std::string> &revision,const BookmarkEdit &edit);
// Existing source must match its exact revision. Indices belong to that revision.
// Unknown attributes/subtrees/comments survive unless their owning node is removed.
std::string serialize_bookmark_edit(const std::string &xml,const std::string &revision,const BookmarkEdit &edit);
// Null revision requires a missing file and insert. Otherwise requires an existing
// matching file. Verified same-directory replacement returns the prepared snapshot.
BookmarkSnapshot edit_bookmarks(const std::string &path,const std::optional<std::string> &revision,const BookmarkEdit &edit);
}
