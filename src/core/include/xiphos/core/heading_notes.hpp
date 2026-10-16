// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "study_note.hpp"
#include <cstddef>
#include <map>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
// Owned internal source for a future heading-aware presentation/lookup stage.
// These SDK identifiers are LOCAL TO THIS FRAGMENT, not body-note identifiers.
// Do not publish the HTML before assigning a heading-scoped link namespace.
// Already-filtered notes retain their source SDK IDs; unfiltered notes get IDs
// from an isolated fragment scope. Never use them for ordinary body-note lookup.
struct HeadingNoteSource {
  std::string html;
  std::string reported_note_label; // Legacy cleanup-point source Footnote/1/n, UTF8HTML.
  std::map<std::string, StudyNote> notes;
};
// Refresh the current verse, select its raw sequential Heading/Preverse fragment,
// then run its non-Headings option filters in an isolated attribute scope.
// Headings already extracted the fragment; repeating it would hide ThML output.
// Copies all
// metadata before optional note-body rendering can change SDK state. Restores
// the complete refreshed source attributes on success and exceptions, without
// moving/replacing its key. Also restores the SDK attribute-processing flag,
// including filter exceptions during the initial refresh. Requires attribute
// processing enabled, serialized ownership and configured filters
// (including Headings On). Throws out_of_range for absent fragments; NUL data
// rejects. Existing legacy heading collection and native snapshots are unchanged.
HeadingNoteSource capture_preverse_heading_notes(sword::SWModule &module,
                                                 std::size_t fragment, bool render_bodies = true);
}}
