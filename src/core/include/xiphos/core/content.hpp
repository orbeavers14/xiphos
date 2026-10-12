// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <string>
#include <vector>
namespace xiphos::core {
struct Entry { std::string reference, html; };
struct DisplayOptions {
  bool verse_numbers = true;
  bool highlight_selected = true;
};
struct Chapter {
  std::string reference, title;
  std::vector<Entry> verses;
  std::size_t selected_verse = 0;
  std::string selected_anchor, html;
};
struct ReferenceList {
  std::string module, context, expression, html;
  std::vector<Entry> entries;
  bool limited = false;
  unsigned skipped = 0;
};
std::string display_reference_list(const ReferenceList &references);
// Shared fragment presentation; engine-rendered verse HTML is retained.
// Reference labels are escaped as attributes. Native hosts supply theme CSS.
std::string display_chapter(const Chapter &chapter, const DisplayOptions &options);
}
