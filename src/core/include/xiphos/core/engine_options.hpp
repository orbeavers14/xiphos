// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <utility>
#include <vector>
namespace sword { class SWMgr; class SWOptionFilter; }
namespace xiphos { namespace core {
class EngineManager;
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
// Snapshots and restores registered filter values around one serialized render.
// The manager must outlive the scope. Neither type grants thread safety.
class ScopedEngineOptions {
public:
  ScopedEngineOptions(EngineManager &manager, const EngineOptions &options);
  ~ScopedEngineOptions();
  ScopedEngineOptions(const ScopedEngineOptions &) = delete;
  ScopedEngineOptions &operator=(const ScopedEngineOptions &) = delete;
private:
  EngineManager &manager_;
  std::vector<std::pair<sword::SWOptionFilter *, std::string>> saved_;
};
} }
