// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "profile.hpp"
#include <cstdint>
#include <variant>
namespace xiphos::core {
enum class BookRestoreMode { display_key, initial_window };
struct BookSelectionTarget {
  std::string module;
  std::variant<std::uint64_t, std::string> address;
};
// A candidate, not a selected/rendered entry. Initial-window restore reads the
// separate saved offset; tab/display restore reads key (missing -> "0").
// Missing/empty module -> no book. SDK existence/alignment/path resolution,
// profile writes and tab publication remain separate operations.
std::optional<BookSelectionTarget> saved_book_target(const SavedBookSelection &saved, BookRestoreMode mode);
}
