// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "study_note.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace xiphos::core {
// Owned SDK tree addresses, not verse references or positions in a UI list.
// Root is offset 0, has an empty SDK path, and uses the module ID as its title.
struct BookTreeNode {
  std::uint64_t offset = 0;
  std::string path, title;
  bool has_children = false;
};
struct BookTree {
  std::string module;
  BookTreeNode node;
  std::optional<BookTreeNode> parent, first_child, previous_sibling, next_sibling;
};
struct BookEntry {
  BookTree tree;
  bool has_entry = false;
  std::string html;
};
struct BookNote {
  BookTree tree;
  StudyNote note;
};
struct BookChildren {
  std::string module;
  BookTreeNode parent;
  std::vector<BookTreeNode> children;
  // Last returned child, only when another child exists. Pass it as `after`
  // to continue. A cursor must still be a direct child of the requested parent.
  std::optional<std::uint64_t> next_after;
};
}
