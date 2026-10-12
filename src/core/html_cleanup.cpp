// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/html_cleanup.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>
namespace xiphos { namespace core {
namespace {
HtmlCleanupResult blank(std::string html, const char *prefix, std::size_t prefix_length,
                        char end, char replacement, bool include_tag) {
  if (html.find('\0') != std::string::npos) throw std::invalid_argument("Cleanup HTML contains NUL");
  HtmlCleanupResult result; result.html = std::move(html);
  std::size_t position = 0;
  while ((position = result.html.find(prefix, position)) != std::string::npos) {
    const auto content = position + prefix_length;
    const auto closing = result.html.find(end, content);
    if (closing == std::string::npos) {
      result.malformed_offset = include_tag ? position : content;
      break;
    }
    const auto first = include_tag ? position : content;
    const auto last = include_tag ? closing + 1 : closing;
    std::fill(result.html.begin() + first, result.html.begin() + last, replacement);
    position = closing; // Legacy continues its next substring search here.
  }
  return result;
}
}
HtmlCleanupResult blank_images(std::string html) {
  return blank(std::move(html), "<img ", 4, '>', ' ', true);
}
HtmlCleanupResult blank_font_faces(std::string html) {
  return blank(std::move(html), "<font face=\"", 12, '"', '.', false);
}
ContentCleanupResult cleanup_html_content(std::string html, ContentPreference images,
                                         ContentPreference font_faces) {
  auto valid = [](ContentPreference value) {
    return value == ContentPreference::unknown || value == ContentPreference::off || value == ContentPreference::on;
  };
  if (!valid(images) || !valid(font_faces) || html.find('\0') != std::string::npos)
    throw std::invalid_argument("Invalid cleanup content/state");
  auto contains = [](const std::string &text, const std::string &pattern) {
    return std::search(text.begin(), text.end(), pattern.begin(), pattern.end(), [](char a, char b) {
      return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    }) != text.end();
  };
  ContentCleanupResult result; result.html = std::move(html);
  result.image_content = images; result.respect_font_faces = font_faces;
  if (images == ContentPreference::off) {
    auto cleaned = blank_images(std::move(result.html));
    if (cleaned.malformed_offset != std::string::npos)
      result.diagnostics.push_back({ContentCleanupKind::image, cleaned.html.substr(cleaned.malformed_offset)});
    result.html = std::move(cleaned.html);
  } else if (images == ContentPreference::unknown && contains(result.html, "<img ")) {
    result.image_content = ContentPreference::on; result.discovered_images = true;
  }
  if (font_faces == ContentPreference::off) {
    auto cleaned = blank_font_faces(std::move(result.html));
    if (cleaned.malformed_offset != std::string::npos)
      result.diagnostics.push_back({ContentCleanupKind::font_face, cleaned.html.substr(cleaned.malformed_offset)});
    result.html = std::move(cleaned.html);
  } else if (font_faces == ContentPreference::unknown && contains(result.html, "<font face=\"")) {
    result.respect_font_faces = ContentPreference::on; result.discovered_font_faces = true;
  }
  return result;
}
}}
