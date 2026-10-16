// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/book_tree.hpp"
namespace sword { class SWModule; }
namespace xiphos::core::detail {
BookTree capture_book_tree(sword::SWModule &module, std::uint64_t offset);
BookTree capture_book_tree_path(sword::SWModule &module, const std::string &path);
// Validates source first, selects that exact live tree offset, and renders it.
// Caller owns engine-option scoping and serialization, as for ordinary reads.
BookEntry capture_book_entry(sword::SWModule &module, std::uint64_t offset);
}
