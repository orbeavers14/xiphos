// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "history.hpp"
#include "reader.hpp"
#include <optional>
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
  // copied settings; clearing them restores the reader's unmodified defaults.
  void set_engine_options(const EngineOptions &options) { options_ = options; }
  void clear_engine_options() { options_.reset(); }
  Chapter read_chapter(const std::string &module, const std::string &reference,
                       const DisplayOptions &display = {});
  const History &history() const { return history_; }
  void clear_history() { history_.clear(); }
private:
  Reader &reader_;
  std::optional<EngineOptions> options_;
  History history_;
};
}
