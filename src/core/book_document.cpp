// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/book_document.hpp"
#include <stdexcept>
namespace xiphos::core {
std::uint32_t book_display_level(const std::optional<std::string> &configured) {
  if (!configured) return 1;
  const auto &text = *configured;
  if (text.size() > 4096 || text.find('\0') != std::string::npos)
    throw std::invalid_argument("Invalid general-book DisplayLevel text");
  std::size_t at = 0;
  while (at < text.size() && (text[at] == ' ' || text[at] == '\t' || text[at] == '\r' ||
         text[at] == '\n' || text[at] == '\v' || text[at] == '\f')) ++at;
  bool negative = false;
  if (at < text.size() && (text[at] == '+' || text[at] == '-')) negative = text[at++] == '-';
  const std::uint32_t limit = negative ? 2147483648u : 2147483647u;
  std::uint32_t value = 0;
  for (; at < text.size() && text[at] >= '0' && text[at] <= '9'; ++at) {
    const auto digit = static_cast<unsigned>(text[at] - '0');
    if (value > (limit - digit) / 10u)
      throw std::invalid_argument("General-book DisplayLevel exceeds the signed integer range");
    value = value * 10 + digit;
  }
  return negative || value <= 1 ? 1 : value;
}
}
