// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/history.hpp"
#include <cstring>
#include <stdexcept>
extern "C" size_t xh_find_reference(const char *const *refs, size_t count,
                                    const char *reference) {
  for (size_t i = 0; i < count; ++i)
    if (std::strcmp(refs[i], reference) == 0) return i;
  return count;
}
namespace xiphos { namespace core {
void History::visit(const Location &location) {
  std::vector<const char *> refs;
  for (const auto &entry : entries_) refs.push_back(entry.reference.c_str());
  const auto found = xh_find_reference(refs.data(), refs.size(), location.reference.c_str());
  if (found < entries_.size()) { cursor_ = found; return; }
  // Allocate first so a failed allocation does not discard an existing entry.
  entries_.push_back(location);
  if (entries_.size() > XH_HISTORY_LIMIT) entries_.erase(entries_.begin());
  cursor_ = entries_.size() - 1;
}
void History::clear() { entries_.clear(); cursor_ = 0; }
bool History::can_back() const { return !entries_.empty() && cursor_ > 0; }
bool History::can_forward() const { return !entries_.empty() && cursor_ + 1 < entries_.size(); }
const Location &History::destination(int direction) const {
  if (direction == -1 && can_back()) return entries_[cursor_ - 1];
  if (direction == 1 && can_forward()) return entries_[cursor_ + 1];
  throw std::invalid_argument("History destination is unavailable");
}
void History::move(int direction) { destination(direction); cursor_ = direction == -1 ? cursor_ - 1 : cursor_ + 1; }
}}
