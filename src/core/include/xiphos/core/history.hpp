// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "history.h"
#include <string>
#include <vector>
namespace xiphos { namespace core {
struct Location {
  std::string reference, module, commentary;
};
// Characterized from src/main/tab_history.c: reference-only deduplication,
// retain forward entries on a new visit, evict oldest at 50 entries.
class History {
public:
  void visit(const Location &location);
  void clear();
  bool can_back() const;
  bool can_forward() const;
  const Location &destination(int direction) const;
  void move(int direction);
  const std::vector<Location> &entries() const { return entries_; }
  size_t cursor() const { return cursor_; }
private:
  std::vector<Location> entries_;
  size_t cursor_ = 0;
};
}}
