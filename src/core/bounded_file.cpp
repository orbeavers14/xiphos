// SPDX-License-Identifier: GPL-2.0-or-later
#include "bounded_file.hpp"
#include <fstream>
#include <stdexcept>
namespace xiphos::core::detail {
std::string read_bounded_file(const std::filesystem::path &path, std::size_t maximum_bytes, const char *description) {
  std::ifstream input(path, std::ios::binary);
  if (!input) throw std::invalid_argument(std::string(description) + " cannot be read");
  std::string result;
  char buffer[8192];
  while (input.read(buffer, sizeof buffer) || input.gcount()) {
    const auto length = static_cast<std::size_t>(input.gcount());
    if (length > maximum_bytes - result.size()) throw std::invalid_argument(std::string(description) + " is too large");
    result.append(buffer, length);
  }
  if (input.bad()) throw std::runtime_error(std::string(description) + " read failed");
  return result;
}
}
