// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
#include "presentation.hpp"
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
enum class ContentCleanupKind { image, font_face };
struct ContentCleanupDiagnostic { ContentCleanupKind kind; std::string suffix; };
struct ContentCleanupResult {
  std::string html;
  ContentPreference image_content = ContentPreference::unknown;
  ContentPreference respect_font_faces = ContentPreference::unknown;
  bool discovered_images = false, discovered_font_faces = false;
  std::vector<ContentCleanupDiagnostic> diagnostics;
};
// Legacy CleanupContent image/font stage only. Applies off states, discovers
// unknown states from content, and retains diagnostics in their original order.
// Returns discovery events; caller owns persistence, numbering and SDK context.
ContentCleanupResult cleanup_html_content(std::string html, ContentPreference images,
                                         ContentPreference font_faces);
}}
