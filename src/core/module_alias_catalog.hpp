// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/module_abbreviations.hpp"
#include "xiphos/core/module_resolution.hpp"
#include <set>
#include <vector>
namespace xiphos::core { class EngineManager;
namespace detail {
inline constexpr std::size_t maximum_alias_catalog_modules = 65536;
inline constexpr std::size_t maximum_alias_catalog_bytes = 16 * 1024 * 1024;
struct ModuleAliasEntry { std::string module; std::optional<std::string> abbreviation; };
class ModuleAliasCatalog {
  std::set<std::string> installed_;
  ModuleAbbreviations aliases_;
public:
  explicit ModuleAliasCatalog(const std::vector<ModuleAliasEntry> &entries);
  ModuleNameResolution resolve(const std::string &requested) const;
};
ModuleAliasCatalog capture_module_alias_catalog(EngineManager &manager);
}}
