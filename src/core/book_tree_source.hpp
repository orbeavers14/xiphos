// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/book_tree.hpp"
namespace sword { class SWModule; }
namespace xiphos::core::detail {
BookTree capture_book_tree(sword::SWModule &module, std::uint64_t offset);
BookTree capture_book_tree_path(sword::SWModule &module, const std::string &path);
}
