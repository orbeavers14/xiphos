// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <filesystem>
#include <cstddef>
#include <string>
#include <stdexcept>
namespace xiphos::core::detail {
// Preserve existing invalid_argument callers while letting an export distinguish
// an exhausted byte budget from an unavailable/unsupported file without strings.
class BoundedFileLimitError:public std::invalid_argument {
public:using std::invalid_argument::invalid_argument;
};
// Binary/NUL-preserving bounded read shared by preference and resource adapters.
std::string read_bounded_file(const std::filesystem::path &path, std::size_t maximum_bytes,
                              const char *description = "File");
}
