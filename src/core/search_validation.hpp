// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/search.hpp"
namespace xiphos::core::detail {
void validate_search_module_name(const std::string &module);
void validate_search_query(const SearchQuery &query);
}
