// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace xiphos { namespace core {
struct HtmlCleanupResult {
  std::string html;
  // Offset of the legacy diagnostic suffix, or npos when complete.
  std::string::size_type malformed_offset = std::string::npos;
};
// Exact legacy byte-preserving operations, not general HTML parsing. Inputs
// are null-free SDK HTML. Image tags become spaces; font face bytes become dots.
HtmlCleanupResult blank_images(std::string html);
HtmlCleanupResult blank_font_faces(std::string html);
}}
