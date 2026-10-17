// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "book_tree.hpp"
#include <cstddef>
namespace xiphos::core {
// Resource bounds reject the whole result rather than publishing a partial book.
// Hosts may choose smaller bounds; the defaults are also the supported maxima.
struct BookDocumentLimits {
  std::uint32_t max_depth = 64, max_nodes = 4096;
  std::size_t max_html_bytes = 8 * 1024 * 1024;
};
struct BookDocumentSection {
  BookEntry entry;
  std::uint32_t depth = 1; // Selected node is level one, as in legacy display.
  bool rendered = false; // Empty raw sections above DisplayLevel one are skipped.
};
struct BookDocument {
  BookTree tree; // Selected source, distinct from descendant/linked section sources.
  std::uint32_t display_level = 1;
  std::vector<BookDocumentSection> sections; // Visited nodes, SDK depth-first order.
  std::string html; // Raw combined SDK HTML; display cleanup remains separate.
};
// Legacy atoi-defined prefix/default/sign behavior, with explicit overflow/NUL
// rejection. Values <=1 (including missing/nonnumeric) mean single-entry display.
std::uint32_t book_display_level(const std::optional<std::string> &configured);
}
