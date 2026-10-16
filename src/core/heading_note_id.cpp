// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/heading_notes.hpp"
#include <limits>
#include <stdexcept>
#include <utility>
namespace xiphos { namespace core {
namespace {
const std::string prefix = "xiphos-heading-";
void validate_local(const std::string &id) {
  if (id.empty() || id.size() > 256) throw std::invalid_argument("Heading note ID is empty or too long");
  for (unsigned char c : id) if (c < 32 || c == 127) throw std::invalid_argument("Control byte in heading note ID");
}
int nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}
}
std::string heading_note_identifier(std::size_t fragment, const std::string &local_id) {
  validate_local(local_id);
  std::string result = prefix + std::to_string(fragment) + '-';
  const char *hex = "0123456789abcdef";
  for (unsigned char c : local_id) { result += hex[c >> 4]; result += hex[c & 15]; }
  return result;
}
bool parse_heading_note_identifier(const std::string &id, HeadingNoteTarget &out) {
  out = HeadingNoteTarget{};
  if (id.compare(0, prefix.size(), prefix) != 0) return false;
  if (id.size() > prefix.size() + sizeof(std::size_t) * 3 + 1 + 512)
    throw std::invalid_argument("Heading note ID is too long");
  const auto separator = id.find('-', prefix.size());
  if (separator == std::string::npos || separator == prefix.size() ||
      (separator > prefix.size() + 1 && id[prefix.size()] == '0'))
    throw std::invalid_argument("Invalid heading fragment ID");
  std::size_t fragment = 0;
  for (auto i = prefix.size(); i < separator; ++i) {
    const auto c = id[i];
    if (c < '0' || c > '9' || fragment > (std::numeric_limits<std::size_t>::max() - (c - '0')) / 10)
      throw std::invalid_argument("Invalid heading fragment ID");
    fragment = fragment * 10 + (c - '0');
  }
  const auto size = id.size() - separator - 1;
  if (!size || size % 2 || size > 512) throw std::invalid_argument("Invalid heading note ID encoding");
  std::string local;
  for (auto i = separator + 1; i < id.size(); i += 2) {
    const auto high = nibble(id[i]), low = nibble(id[i + 1]);
    if (high < 0 || low < 0) throw std::invalid_argument("Invalid heading note ID encoding");
    local += static_cast<char>((high << 4) | low);
  }
  validate_local(local);
  out.fragment = fragment; out.local_id = std::move(local);
  return true;
}
}}
