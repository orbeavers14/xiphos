// SPDX-License-Identifier: GPL-2.0-or-later
// Derived behavior: Xiphos CleanupContent, Copyright (C) 2000-2026 Xiphos Developer Team.
#include "xiphos/core/note_markers.hpp"
#include <limits>
#include <stdexcept>
#include <utility>
namespace xiphos { namespace core {
NumberedNoteMarkers number_note_markers(std::string html, bool enabled,
                                        bool first_note_has_label, NoteNumberingState counters) {
  if (html.find('\0') != std::string::npos || counters.footnotes < 0 || counters.cross_references < 0)
    throw std::invalid_argument("Invalid note-marker text or counter state");
  if (!enabled || first_note_has_label) return {std::move(html), counters};
  std::string result;
  result.reserve(html.size());
  std::size_t copied = 0;
  for (std::size_t at = 0; at + 1 < html.size(); ++at) {
    if (html[at] != '*' || (html[at + 1] != 'n' && html[at + 1] != 'x')) continue;
    auto &counter = html[at + 1] == 'n' ? counters.footnotes : counters.cross_references;
    if (counter == std::numeric_limits<int>::max()) throw std::overflow_error("Note-marker counter overflow");
    // g_snprintf(value, 5, "%d", ++counter) in the retained legacy oracle.
    const auto digits = std::to_string(++counter).substr(0, 4);
    result.append(html, copied, at + 2 - copied);
    result += digits;
    copied = at + 2;
    ++at;
  }
  result.append(html, copied, std::string::npos);
  return {std::move(result), counters};
}
}}
