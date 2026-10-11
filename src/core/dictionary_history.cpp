// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/dictionary_history.hpp"
#include <stdexcept>
namespace xiphos { namespace core {
void DictionaryHistory::visit(const DictionaryLocation &location) {
  if (!entries_.empty() && entries_[cursor_].module == location.module && entries_[cursor_].key == location.key) return;
  // Build before publishing: allocation failure retains cursor and forward data.
  std::vector<DictionaryLocation> replacement(entries_.begin(), entries_.begin() + (entries_.empty() ? 0 : cursor_ + 1));
  replacement.push_back(location);
  if (replacement.size() > 50) replacement.erase(replacement.begin());
  entries_.swap(replacement); cursor_ = entries_.size() - 1;
}
void DictionaryHistory::clear() { entries_.clear(); cursor_ = 0; }
bool DictionaryHistory::can_back() const { return !entries_.empty() && cursor_ > 0; }
bool DictionaryHistory::can_forward() const { return !entries_.empty() && cursor_ + 1 < entries_.size(); }
const DictionaryLocation &DictionaryHistory::destination(int direction) const {
  if (direction == -1 && can_back()) return entries_[cursor_ - 1];
  if (direction == 1 && can_forward()) return entries_[cursor_ + 1];
  throw std::invalid_argument("Dictionary history destination is unavailable");
}
void DictionaryHistory::move(int direction) { destination(direction); cursor_ = direction == -1 ? cursor_ - 1 : cursor_ + 1; }
}}
