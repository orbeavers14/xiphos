// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
namespace xiphos { namespace core {
struct DictionaryLocation { std::string module, key; };
// Dictionary history differs from Bible history: deduplicate only the current
// module/key pair, clear forward entries on a changed visit, retain at most 50.
class DictionaryHistory {
public:
  // Returns false for an unchanged current pair.
  bool visit(const DictionaryLocation &location);
  void clear();
  bool can_back() const;
  bool can_forward() const;
  const DictionaryLocation &destination(int direction) const;
  void move(int direction);
  const std::vector<DictionaryLocation> &entries() const { return entries_; }
  std::size_t cursor() const { return cursor_; }
private:
  std::vector<DictionaryLocation> entries_;
  std::size_t cursor_ = 0;
};
}}
