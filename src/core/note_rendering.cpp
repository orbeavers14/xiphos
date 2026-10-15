// SPDX-License-Identifier: GPL-2.0-or-later
// Xiphos set_render_numbers, Copyright (C) 2000-2026 Xiphos Developer Team.
#include "xiphos/core/note_rendering.hpp"
#include <swmodule.h>
#include <osisxhtml.h>
#include <thmlxhtml.h>
#include <gbfxhtml.h>
#include <teixhtml.h>
namespace xiphos { namespace core {
void set_render_note_numbers(sword::SWModule &module, bool enabled) {
  for (auto *filter : module.getRenderFilters()) {
    if (auto *osis = dynamic_cast<sword::OSISXHTML *>(filter)) osis->setRenderNoteNumbers(enabled);
    if (auto *thml = dynamic_cast<sword::ThMLXHTML *>(filter)) thml->setRenderNoteNumbers(enabled);
    if (auto *gbf = dynamic_cast<sword::GBFXHTML *>(filter)) gbf->setRenderNoteNumbers(enabled);
    if (auto *tei = dynamic_cast<sword::TEIXHTML *>(filter)) tei->setRenderNoteNumbers(enabled);
  }
}
}}
