// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/search.hpp"
#include "search_scope.hpp"
namespace sword { class SWModule; }
namespace xiphos::core { class EngineManager;
namespace detail {
bool supports_verse_search(sword::SWModule &module);
SearchResult capture_module_search(EngineManager &manager,sword::SWModule &module,
                                  SearchQuery query,const SearchLimits &limits,const std::shared_ptr<SearchControl> &control = {},
                                  const SearchScope *previous=nullptr,std::shared_ptr<const SearchScope> *next=nullptr,
                                  const SearchScopeLimits &scope_limits = {},
                                  const std::optional<SearchTextOptions> &previous_text = {});
}}
