// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/search.hpp"
namespace sword { class ListKey; }
namespace xiphos::core::detail {
struct SearchReferences {
  std::uint64_t engine_count=0;
  std::vector<std::string> references;
  bool limited=false;
};
// Engine-owner only. Traverses the SDK list; owns every returned string.
// engine_count counts SDK elements, not expanded verses in interval elements.
SearchReferences copy_search_references(sword::ListKey &list,const SearchLimits &limits,bool allow_empty_output=false);
}
