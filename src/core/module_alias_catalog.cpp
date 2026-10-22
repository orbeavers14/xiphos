// SPDX-License-Identifier: GPL-2.0-or-later
#include "module_alias_catalog.hpp"
#include "search_validation.hpp"
#include "xiphos/core/engine_manager.hpp"
#include <swmodule.h>
#include <glib.h>
#include <stdexcept>
namespace xiphos::core::detail {
namespace {
void validate_alias(const std::string &text) {
  if (text.size() > 4096 || text.find('\0') != std::string::npos ||
      !g_utf8_validate(text.data(), text.size(), nullptr))
    throw std::invalid_argument("Invalid bounded UTF-8 module abbreviation");
}
std::string copy_sdk(const char *text) {
  if (!text) throw std::runtime_error("Module catalog has no name");
  std::size_t count = 0;
  while (count <= 4096 && text[count]) ++count;
  if (count > 4096) throw std::invalid_argument("Module alias metadata exceeds text bounds");
  return {text, count};
}
void account(std::size_t &bytes, std::size_t size) {
  if (size > maximum_alias_catalog_bytes - bytes)
    throw std::invalid_argument("Module alias catalog exceeds byte bounds");
  bytes += size;
}
}
ModuleAliasCatalog::ModuleAliasCatalog(const std::vector<ModuleAliasEntry> &entries) {
  if (entries.size() > maximum_alias_catalog_modules)
    throw std::invalid_argument("Module alias catalog exceeds module bounds");
  std::size_t bytes = 0;
  for (const auto &entry : entries) {
    validate_search_module_name(entry.module); account(bytes, entry.module.size());
    if (entry.abbreviation) { validate_alias(*entry.abbreviation); account(bytes, entry.abbreviation->size()); }
    if (!installed_.insert(entry.module).second)
      throw std::invalid_argument("Module alias catalog has duplicate installed IDs");
  }
  // Protect every installed ID, including IDs that appear later in SDK order.
  for (const auto &entry : entries)
    if (entry.abbreviation && !installed_.count(*entry.abbreviation))
      aliases_.add(entry.module.c_str(), entry.abbreviation->c_str());
}
ModuleNameResolution ModuleAliasCatalog::resolve(const std::string &requested) const {
  validate_search_module_name(requested);
  ModuleNameResolution result{requested, {}, false};
  if (installed_.count(requested)) result.module = requested;
  else if (const auto *name = aliases_.name_for(requested.c_str())) {
    result.module = name; result.via_abbreviation = true;
  }
  return result;
}
ModuleAliasCatalog capture_module_alias_catalog(EngineManager &manager) {
  if (manager.Modules.size() > maximum_alias_catalog_modules)
    throw std::invalid_argument("Module alias catalog exceeds module bounds");
  std::vector<ModuleAliasEntry> entries;
  entries.reserve(manager.Modules.size());
  std::size_t bytes = 0;
  for (const auto &item : manager.Modules) {
    auto &module = *item.second;
    ModuleAliasEntry entry{copy_sdk(module.getName()), {}};
    account(bytes, entry.module.size());
    if (const auto *alias = module.getConfigEntry("Abbreviation")) {
      entry.abbreviation = copy_sdk(alias); account(bytes, entry.abbreviation->size());
    }
    entries.push_back(std::move(entry));
  }
  return ModuleAliasCatalog(entries);
}
}
