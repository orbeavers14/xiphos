// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/link.hpp"
#include <stdexcept>
namespace xiphos::core {
namespace {
int hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
std::string decode(const std::string &url) {
  std::string value;
  for (std::size_t i = 0; i < url.size(); ++i) {
    char c = url[i];
    if (c == '+') c = ' ';
    else if (c == '%' && i + 2 < url.size() && hex(url[i+1]) >= 0 && hex(url[i+2]) >= 0) {
      c = static_cast<char>((hex(url[i+1]) << 4) | hex(url[i+2])); i += 2;
    }
    if (static_cast<unsigned char>(c) < 32 || c == 127)
      throw std::invalid_argument("Control characters in passage link");
    value += c;
  }
  return value;
}
}
PassageLink passage_link(const std::string &url, const std::string &current_module) {
  // Require the whole URI, unlike the legacy handler's substring dispatch.
  // External content must not accidentally become an internal study command.
  const auto decoded = decode(url);
  std::size_t start;
  if (decoded.compare(0, 8, "sword://") == 0) start = 8;
  else if (decoded.compare(0, 8, "bible://") == 0) start = 8;
  else throw std::invalid_argument("Unsupported passage link");
  const auto slash = decoded.find('/', start);
  if (slash == std::string::npos) throw std::invalid_argument("Passage link requires a reference path");
  PassageLink result{decoded.substr(start, slash - start), decoded.substr(slash + 1)};
  if (result.module.empty()) result.module = current_module;
  if (result.module.empty() || result.reference.empty() ||
      result.reference.find_first_of("#?!-;,/") != std::string::npos)
    throw std::invalid_argument("This slice supports unanchored single-reference links");
  return result;
}
std::string passage_uri(const std::string &reference) {
  static const char digits[] = "0123456789ABCDEF";
  std::string result = "sword:///";
  for (unsigned char c : reference) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == ':' || c == '.' || c == '_') result += c;
    else { result += '%'; result += digits[c >> 4]; result += digits[c & 15]; }
  }
  return result;
}
}
