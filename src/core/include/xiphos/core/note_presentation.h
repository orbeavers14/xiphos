/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XIPHOS_CORE_NOTE_PRESENTATION_H
#define XIPHOS_CORE_NOTE_PRESENTATION_H
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
  XIPHOS_NOTE_NO_PRESENTATION = 0,
  XIPHOS_NOTE_HOVER_BODY = 1,
  XIPHOS_NOTE_CLICK_REFERENCES = 2,
  XIPHOS_NOTE_HOVER_REFERENCES = 3,
  XIPHOS_NOTE_HOVER_REFERENCE_LINKS = 4
} XiphosNotePresentation;
/* Main-reader note tag routing only. Nonzero flags are true; a null/empty
 * type has no presentation. Source/module setup, identifier validation, note
 * retrieval and dialog policy are separate. Mixed n/x tags preserve legacy
 * branch precedence. Click/hover reference routes stay distinct because the
 * legacy callbacks handle missing refList values differently. */
XiphosNotePresentation xiphos_core_note_presentation(const char *type, int clicked, int xrefs_in_verse_list);
#ifdef __cplusplus
}
#endif
#endif
