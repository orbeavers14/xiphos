// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <map>
#include <utility>
#include <vector>
namespace sword { class SWModule; class SWFilter; }
namespace xiphos { namespace core {
// Legacy OSIS/ThML/GBF/TEI XHTML label switch. Requires the serialized engine
// owner. Mutates all matching module render filters; not a scoped setting.
// SDK setters have no getter: native use requires an owned restoration policy.
void set_render_note_numbers(sword::SWModule &module, bool enabled);
// Explicit ownership replaces unavailable SDK getters. Register establishes a
// caller-chosen baseline on new filters only; aliases share the tracked pointer.
// All later writes must use these scopes. The owner/filters outlive every scope;
// registration and scopes run on the serialized engine thread, never concurrently.
class NoteRenderingState {
public:
  NoteRenderingState() = default;
  void register_module(sword::SWModule &module, bool baseline);
  NoteRenderingState(const NoteRenderingState &) = delete;
  NoteRenderingState &operator=(const NoteRenderingState &) = delete;
private:
  friend class ScopedNoteNumbers;
  std::map<sword::SWFilter *, bool> values_;
};
class ScopedNoteNumbers {
public:
  // Rejects unregistered matching filters before changing any state. Nested
  // scopes and exceptions restore the owner's exact prior per-filter values.
  ScopedNoteNumbers(NoteRenderingState &owner, sword::SWModule &module, bool enabled);
  ~ScopedNoteNumbers();
  ScopedNoteNumbers(const ScopedNoteNumbers &) = delete;
  ScopedNoteNumbers &operator=(const ScopedNoteNumbers &) = delete;
private:
  NoteRenderingState &owner_;
  std::vector<std::pair<sword::SWFilter *, bool>> previous_;
};
}}
