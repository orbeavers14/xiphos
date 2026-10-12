// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <string>
#include <vector>
namespace xiphos::core {
struct ParsedReferenceList;
struct DictionaryEntry { std::string module, key, html; };
struct Entry { std::string reference, html, heading_html; };
struct DisplayOptions {
  bool verse_numbers = true;
  bool highlight_selected = true;
};
struct Chapter {
  std::string reference, title;
  // Bible: dense canonical verses. Commentary: legacy module iterator positions.
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
// Link-only information preview plus a return link to the committed context.
// Native hosts provide theme CSS; the initial label is the English baseline.
std::string display_reference_links(const ParsedReferenceList &references);
// Shared fragment presentation; engine-rendered verse HTML is retained.
// Reference labels are escaped as attributes. Native hosts supply theme CSS.
std::string display_chapter(const Chapter &chapter, const DisplayOptions &options);
// Sparse/linked commentary sections need their engine verse ordinals.
std::string display_commentary_chapter(const Chapter &chapter, const std::vector<unsigned> &positions,
                                       const DisplayOptions &options);
}
