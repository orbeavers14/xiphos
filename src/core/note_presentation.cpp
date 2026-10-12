// SPDX-License-Identifier: GPL-2.0-or-later
// Extracted from main/url.cc, Copyright (C) 2004-2026 Xiphos Developer Team.
#include "xiphos/core/note_presentation.h"
#include <cstring>
XiphosNotePresentation xiphos_core_note_presentation(const char *type, int clicked, int xrefs_in_verse_list) {
  if (!type) return XIPHOS_NOTE_NO_PRESENTATION;
  if (std::strchr(type, 'x') && clicked) return XIPHOS_NOTE_CLICK_REFERENCES;
  if (std::strchr(type, 'n') && !clicked) return XIPHOS_NOTE_HOVER_BODY;
  if (std::strchr(type, 'x') && !clicked)
    return xrefs_in_verse_list ? XIPHOS_NOTE_HOVER_REFERENCES : XIPHOS_NOTE_HOVER_REFERENCE_LINKS;
  return XIPHOS_NOTE_NO_PRESENTATION;
}
