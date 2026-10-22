// SPDX-License-Identifier: GPL-2.0-or-later
// Policy extracted from src/main/sword.cc, Copyright (C) 2000-2026
// Xiphos Developer Team. Kept compatible with the retained C++11 frontend.
#pragma once
#include <map>
#include <cstring>
#include <string>
namespace xiphos { namespace core {
// Exact legacy registration maps. Installed-ID collision exclusion belongs to
// catalog registration before add(); this helper preserves raw insertion order.
// Re-registering one side intentionally does not remove older inverse entries.
class ModuleAbbreviations {
  std::map<std::string, std::string> names_, abbreviations_;
public:
  // Non-null terminated strings, as in retained main_add_abbreviation.
  void add(const char *name, const char *abbreviation) {
    if (std::strchr(abbreviation, '(')) return;
    names_[name] = abbreviation;
    abbreviations_[abbreviation] = name;
  }
  void clear() { names_.clear(); abbreviations_.clear(); }
  // Borrowed internal values: copy before mutation/destruction or transport.
  // Empty values differ from no mapping; null query pointers return null.
  const char *abbreviation_for(const char *name) const {
    if (!name) return nullptr;
    const auto found = names_.find(name);
    return found == names_.end() ? nullptr : found->second.c_str();
  }
  const char *name_for(const char *abbreviation) const {
    if (!abbreviation) return nullptr;
    const auto found = abbreviations_.find(abbreviation);
    return found == abbreviations_.end() ? nullptr : found->second.c_str();
  }
};
} }
