// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace xiphos { namespace core {
struct NoteNumberingState {
  int footnotes, cross_references;
  NoteNumberingState(int notes = 0, int references = 0) : footnotes(notes), cross_references(references) {}
};
struct NumberedNoteMarkers { std::string html; NoteNumberingState counters; };
// Owned version of legacy CleanupContent's literal *n/*x numbering stage.
// A reported first-note n attribute suppresses both counters for this entry.
// Counter state is explicit and may continue across a document. Keeps legacy
// four-digit truncation; does not parse HTML or decide SDK render-filter flags.
// NUL/negative state rejects; counter overflow throws before publishing a result.
NumberedNoteMarkers number_note_markers(std::string html, bool enabled,
                                        bool first_note_has_label,
                                        NoteNumberingState counters = NoteNumberingState());
}}
