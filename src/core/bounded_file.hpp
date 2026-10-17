// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <filesystem>
#include <cstddef>
#include <string>
namespace xiphos::core::detail {
// Binary/NUL-preserving bounded read shared by preference and resource adapters.
std::string read_bounded_file(const std::filesystem::path &path, std::size_t maximum_bytes,
                              const char *description = "File");
}
