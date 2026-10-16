// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "heading_notes.hpp"
#include "headings.hpp"
namespace xiphos::core {
// Pure native presentation gate: all HTML must match byte-for-byte except SDK
// note URI values. Known original SDK IDs retain exact local identity; empty
// originals use the scoped source's IDs. Scoped targets must match the fragment,
// copied note module/reference/type and cleanup label. Unsupported input returns
// false; it never chooses a note body by visible label or position.
bool compatible_heading_note_presentation(const HeadingFragment &raw,
    const HeadingNoteSource &candidate, std::size_t fragment);
}
