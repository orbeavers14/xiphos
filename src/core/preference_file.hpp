// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <filesystem>
#include <functional>
#include <limits>
#include <string>
namespace xiphos::core::detail {
std::string read_preference_file(const std::filesystem::path &path,
    std::size_t maximum_bytes = std::numeric_limits<std::size_t>::max());
// Prepare and verify the staged file in the callback before replacement.
// This catches observed external changes but is not an interprocess lock.
void replace_preference_file(const std::string &path,
    const std::function<void(const std::filesystem::path &, const std::string &, bool)> &prepare,
    std::size_t maximum_bytes = std::numeric_limits<std::size_t>::max());
}
