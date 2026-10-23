// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/book_selection.hpp"
#include <stdexcept>
#include <utility>
namespace xiphos::core {
namespace {
bool digit(char c) { return c >= '0' && c <= '9'; }
bool space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f'; }
void valid_text(const std::string &text) {
  if (text.size() > 4096 || text.find('\0') != std::string::npos)
    throw std::invalid_argument("Saved book address is too long or contains NUL");
}
std::uint64_t prefix(const std::string &text) {
  std::size_t at = 0;
  while (at < text.size() && space(text[at])) ++at;
  bool negative = false;
  if (at < text.size() && (text[at] == '+' || text[at] == '-')) negative = text[at++] == '-';
  std::uint64_t value = 0;
  for (; at < text.size() && digit(text[at]); ++at) {
    const auto next = static_cast<unsigned>(text[at] - '0');
    // SDK TreeKeyIdx stores signed 32-bit index positions on LP64 and LLP64.
    if (value > (2147483647u - next) / 10u)
      throw std::invalid_argument("Saved book offset exceeds the indexed-tree range");
    value = value * 10 + next;
  }
  if (negative && value) throw std::invalid_argument("Saved book offset is negative");
  return value;
}
}
BookSelectionTarget book_reading_target(BookSelectionTarget target) {
  if (auto *offset = std::get_if<std::uint64_t>(&target.address))
    if (*offset < 4) *offset = 4;
  return target;
}
BookSelectionTarget book_display_target(std::string module, std::string key) {
  if (module.empty() || module.find('\0') != std::string::npos)
    throw std::invalid_argument("Book module is empty or contains NUL");
  valid_text(key);
  BookSelectionTarget result{std::move(module), std::uint64_t(4)};
  if (!key.empty() && digit(key.front())) result.address = prefix(key);
  else result.address = std::move(key);
  return book_reading_target(std::move(result));
}
std::optional<BookSelectionTarget> saved_book_target(const SavedBookSelection &saved, BookRestoreMode mode) {
  if (mode != BookRestoreMode::display_key && mode != BookRestoreMode::initial_window)
    throw std::invalid_argument("Unknown book restore mode");
  if (!saved.module || saved.module->empty()) return std::nullopt;
  if (saved.module->find('\0') != std::string::npos) throw std::invalid_argument("Saved book module contains NUL");
  BookSelectionTarget result{*saved.module, std::uint64_t(4)};
  switch (mode) {
    case BookRestoreMode::initial_window: {
      const auto text = saved.offset_text.value_or("");
      valid_text(text);
      result.address = prefix(text);
      break;
    }
    case BookRestoreMode::display_key: {
      return book_display_target(*saved.module, saved.key.value_or("0"));
    }
    default: throw std::invalid_argument("Unknown book restore mode");
  }
  return book_reading_target(std::move(result));
}
}
