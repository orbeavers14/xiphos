// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "history.hpp"
#include "reader.hpp"
namespace xiphos::core {
// One instance per study tab/window. Calls and lifetime must be serialized.
// The reader must outlive this C++ session; C ABI sessions retain their reader.
class StudySession {
public:
  explicit StudySession(Reader &reader) : reader_(reader) {}
  Entry visit(const std::string &module, const std::string &reference);
  Entry navigate(int direction);
  Entry navigate_chapter(const std::string &module, const std::string &reference, int direction);
  const History &history() const { return history_; }
  void clear_history() { history_.clear(); }
private:
  Reader &reader_;
  History history_;
};
}
