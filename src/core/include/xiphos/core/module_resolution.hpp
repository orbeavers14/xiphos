// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <optional>
#include <string>
namespace xiphos::core {
// Copied result: requested token remains literal; absence means no installed
// exact ID or registered abbreviation. Resolution never selects/reads a module.
struct ModuleNameResolution {
  std::string requested;
  std::optional<std::string> module;
  bool via_abbreviation = false;
};
}
