// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "bookmarks.hpp"
namespace xiphos::core {
struct AndBibleImport {
 std::string xml,revision;
 std::size_t imported=0,skipped=0,unknown_versification=0,invalid_ordinal=0,invalid_identity=0;
};
// Explicit standalone SQLite backup only, <=64MiB. Private immutable copy,
// bounded read-only transaction/queries; no original writes or hidden discovery.
// Supported schema: BibleBookmark, optional Label/ToLabel/Notes tables. UUIDs,
// label order/colors, module IDs, versification and notes remain XML metadata.
AndBibleImport read_andbible_bookmarks(const std::string &path);
// Append one imported document beneath an explicit new folder; preserve current
// unknown XML and imported root metadata. Exact revision guard; reject stable
// AndBible identity collisions instead of silently duplicating/updating notes.
BookmarkSnapshot append_bookmark_document(const std::string &path,const std::optional<std::string> &revision,const std::string &xml,const std::string &folder);
BookmarkSnapshot export_bookmark_document(const std::string &source,const std::string &source_revision,const std::string &destination,const std::optional<std::string> &destination_revision);
}
