// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
namespace xiphos::core {
enum class SearchMode { regular_expression=0, phrase=1, words=2 };
struct SearchQuery {
  std::string module, expression;
  SearchMode mode=SearchMode::phrase;
  bool case_sensitive=false;
};
struct SearchLimits {
  std::size_t max_results=1000;
  std::size_t max_reference_bytes=1024*1024;
};
inline constexpr std::size_t maximum_search_results=10000;
inline constexpr std::size_t maximum_search_reference_bytes=8*1024*1024;
struct SearchResult {
  SearchQuery requested;
  std::string effective_expression; // Exact copied module stripText query.
  int engine_type=0; // Actual SDK type, including automatic indexed word choice.
  std::uint64_t total_results=0; // SDK list count, not a promise about expanded ranges.
  std::vector<std::string> references; // Complete owned SDK keys, ordered, never byte-truncated.
  bool limited=false;
};
// This initial whole-module verse search bounds copied output only. SDK execution
// time/internal result allocation are not bounded; cancellation/progress and
// explicit range/last-search scopes are separate capabilities.
}
