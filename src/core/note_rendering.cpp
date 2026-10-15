// SPDX-License-Identifier: GPL-2.0-or-later
// Xiphos set_render_numbers, Copyright (C) 2000-2026 Xiphos Developer Team.
#include "xiphos/core/note_rendering.hpp"
#include <swmodule.h>
#include <osisxhtml.h>
#include <thmlxhtml.h>
#include <gbfxhtml.h>
#include <teixhtml.h>
#include <stdexcept>
namespace xiphos { namespace core {
namespace {
bool supported(sword::SWFilter *filter) {
  return dynamic_cast<sword::OSISXHTML *>(filter) || dynamic_cast<sword::ThMLXHTML *>(filter) ||
         dynamic_cast<sword::GBFXHTML *>(filter) || dynamic_cast<sword::TEIXHTML *>(filter);
}
void apply(sword::SWFilter *filter, bool enabled) noexcept {
  if (auto *osis = dynamic_cast<sword::OSISXHTML *>(filter)) osis->setRenderNoteNumbers(enabled);
  if (auto *thml = dynamic_cast<sword::ThMLXHTML *>(filter)) thml->setRenderNoteNumbers(enabled);
  if (auto *gbf = dynamic_cast<sword::GBFXHTML *>(filter)) gbf->setRenderNoteNumbers(enabled);
  if (auto *tei = dynamic_cast<sword::TEIXHTML *>(filter)) tei->setRenderNoteNumbers(enabled);
}
}
void set_render_note_numbers(sword::SWModule &module, bool enabled) {
  for (auto *filter : module.getRenderFilters()) apply(filter, enabled);
}
void NoteRenderingState::register_module(sword::SWModule &module, bool baseline) {
  // Complete allocations before mutating an SDK filter or the tracked owner.
  auto next = values_;
  std::vector<sword::SWFilter *> added;
  for (auto *filter : module.getRenderFilters())
    if (supported(filter) && next.emplace(filter, baseline).second) added.push_back(filter);
  for (auto *filter : added) apply(filter, baseline);
  values_.swap(next);
}
ScopedNoteNumbers::ScopedNoteNumbers(NoteRenderingState &owner, sword::SWModule &module, bool enabled) : owner_(owner) {
  for (auto *filter : module.getRenderFilters()) {
    if (!supported(filter)) continue;
    const auto found = owner_.values_.find(filter);
    if (found == owner_.values_.end()) throw std::invalid_argument("Note render filter has no owned baseline");
    previous_.push_back({filter, found->second});
  }
  for (const auto &saved : previous_) {
    apply(saved.first, enabled);
    owner_.values_.find(saved.first)->second = enabled;
  }
}
ScopedNoteNumbers::~ScopedNoteNumbers() {
  for (const auto &saved : previous_) {
    apply(saved.first, saved.second);
    owner_.values_.find(saved.first)->second = saved.second;
  }
}
}}
