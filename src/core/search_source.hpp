// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/search.hpp"
namespace sword { class SWModule; }
namespace xiphos::core { class EngineManager;
namespace detail {
SearchResult capture_module_search(EngineManager &manager,sword::SWModule &module,
                                  SearchQuery query,const SearchLimits &limits);
}}
