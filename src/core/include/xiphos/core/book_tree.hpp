// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <optional>
#include <string>
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
}
