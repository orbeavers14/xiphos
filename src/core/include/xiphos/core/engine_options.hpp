// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <map>
#include <utility>
#include <vector>
namespace sword { class SWMgr; class SWOptionFilter; class SWModule; }
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
// Exact modops.conf engine-setting semantics from legacy main_new_globals.
// Unknown keys are ignored; this parser does not read or write user files.
// Select exactly one reading while retaining every unrelated option. Invalid choices reject.
EngineOptions select_variant_reading(EngineOptions options, int choice);
using EngineOptionPreferences = std::map<std::string, std::string>;
EngineOptions engine_options_from_preferences(const EngineOptionPreferences &preferences);
// Legacy modops.conf encoding; a no-variant selection reloads as primary.
EngineOptionPreferences engine_options_to_preferences(const EngineOptions &options);
// Read-only legacy SWORD configuration decoding. No home-directory discovery,
// file creation, preference writes or installed-module filtering.
using ModuleEnginePreferences = std::map<std::string, EngineOptions>;
using ModuleOptionValues = std::map<std::string, EngineOptionPreferences>;
// One legacy SWConfig decode shared by engine and presentation imports.
ModuleOptionValues read_module_option_values(const std::string &path);
ModuleEnginePreferences read_module_engine_preferences(const std::string &path);
struct ModuleOptions {
  EngineOptions declared;
  bool headings = false;
};
// Eligibility from Xiphos's exact GlobalOptionFilter declarations, not proof
// that the installed SDK implements a filter or the module contains markup.
// Italic headings require this eligibility AND display headings enabled.
// Global transliteration availability is separate from module declarations.
ModuleOptions module_options(const sword::SWModule &module);
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
