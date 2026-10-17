// SPDX-License-Identifier: GPL-2.0-or-later
#include "book_resource_source.hpp"
#include "book_tree_source.hpp"
#include <swmodule.h>
#include <stdexcept>
namespace xiphos::core::detail {
BookResourceContext capture_book_resource_context(sword::SWModule &module) {
  if (!indexed_book(module)) throw std::invalid_argument("Resource context requires an indexed general book");
  const auto copy = [&](const char *name) -> std::optional<std::string> {
    const auto *value = module.getConfigEntry(name);
    return value ? std::make_optional(std::string(value)) : std::nullopt;
  };
  return {module.getName(), copy("DataPath"), copy("AbsoluteDataPath"), copy("PrefixPath"), copy("PreferredCSSXHTML")};
}
}
