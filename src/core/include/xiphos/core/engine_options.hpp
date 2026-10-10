// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
namespace sword { class SWMgr; }
namespace xiphos { namespace core {
// Only SWORD filter options. Display headings, spacing, note numbering and
// other Xiphos presentation settings belong to the separate display policy.
// Zero initialization is a value container, not a saved-preference default.
struct EngineOptions {
  bool strongs = false;
  bool morphs = false;
  bool footnotes = false;
  bool greekaccents = false;
  bool lemmas = false;
  bool scripturerefs = false;
  bool hebrewpoints = false;
  bool hebrewcant = false;
  bool italic_headings = false;
  bool words_in_red = false;
  bool xlit = false;
  bool enumerated = false;
  bool glosses = false;
  bool morphseg = false;
  bool transliteration = false;
  bool variants_primary = false;
  bool variants_secondary = false;
  bool variants_all = false;
};
// Mutates the manager's filter state. Call on the serialized engine owner
// before rendering; does not save preferences or promise concurrent managers.
// Retains legacy headings-on and textual-variant precedence/no-selection rules.
void apply_engine_options(sword::SWMgr &manager, const EngineOptions &options);
} }
