// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <string>
namespace xiphos::core {
inline constexpr std::size_t maximum_html_source_bytes=8*1024*1024;
struct HtmlSourceLimits {std::size_t max_bytes=1024*1024;};
// Body-only configured verse source, as used by legacy item export. Matching
// core CSS travels with HTML; headings/raw SDK note attributes are not included.
// Strings are NUL-free UTF-8. Assets, theme and complete documents are separate.
struct EntryHtmlSource {
  std::string module,reference,html,css;
  bool lexical_requested=false,lexical_applied=false;
  std::size_t copied_bytes=0;
};
}
