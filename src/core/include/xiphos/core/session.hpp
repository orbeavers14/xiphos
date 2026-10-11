// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "history.hpp"
#include "reader.hpp"
#include <optional>
#include <map>
namespace xiphos::core {
// One instance per study tab/window. Calls and lifetime must be serialized.
// The reader must outlive this C++ session; C ABI sessions retain their reader.
class StudySession {
public:
  explicit StudySession(Reader &reader) : reader_(reader) {}
  // Render with this tab's settings without moving or adding history.
  Entry read_entry(const std::string &module, const std::string &reference);
  Entry visit(const std::string &module, const std::string &reference);
  Entry navigate(int direction);
  Entry navigate_chapter(const std::string &module, const std::string &reference, int direction);
  // Presentation changes do not add history entries. All future tab reads use
  // copied settings. Clearing this layer retains explicit module overrides
  // and enabled module defaults; otherwise it restores unconfigured SDK state.
  void set_engine_options(const EngineOptions &options) { options_ = options; }
  void clear_engine_options() { options_.reset(); }
  // Opt-in module preferences use legacy defaults without reading user files.
  // Explicit module overrides win over tab-wide overrides; then opt-in defaults.
  // Existing sessions remain unconfigured until explicitly enabled or set.
  void enable_module_preferences() { module_preferences_ = true; }
  // Explicit read-only import replaces this tab's engine preferences for all
  // installed modules. Missing sections use legacy defaults. Failed imports
  // preserve settings/history; other tabs and source bytes are untouched.
  void import_module_preferences(const std::string &path);
  // Saves explicit effective options, never guesses unconfigured SDK state.
  void save_module_preferences(const std::string &path) const;
  void set_module_engine_options(const std::string &module, const EngineOptions &options);
  void clear_module_engine_options(const std::string &module);
  std::optional<EngineOptions> module_engine_options(const std::string &module) const;
  Chapter read_chapter(const std::string &module, const std::string &reference,
                       const DisplayOptions &display = {});
  const History &history() const { return history_; }
  void clear_history() { history_.clear(); }
private:
  Reader &reader_;
  std::optional<EngineOptions> options_;
  bool module_preferences_ = false;
  std::map<std::string, EngineOptions> module_options_;
  void require_module(const std::string &module) const;
  History history_;
};
}
