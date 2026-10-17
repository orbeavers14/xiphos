// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <memory>
namespace xiphos::core {
namespace detail { class SearchControlAccess; }
enum class SearchPhase { queued=0, running=1, complete=2, cancelled=3, failed=4 };
// Thread-safe control contains atomics only, never SDK/module pointers. Cancel
// is cooperative; completion can win a late request. Use one control per job.
class SearchControl {
  friend class detail::SearchControlAccess;
  std::atomic<bool> cancel_{false};
  std::atomic<unsigned> percent_{0};
  std::atomic<SearchPhase> phase_{SearchPhase::queued};
public:
  void request_cancel() noexcept {
    if(static_cast<int>(phase())<static_cast<int>(SearchPhase::complete))cancel_.store(true,std::memory_order_release);
  }
  bool cancel_requested() const noexcept{return cancel_.load(std::memory_order_acquire);}
  unsigned percent() const noexcept{return percent_.load(std::memory_order_relaxed);}
  SearchPhase phase() const noexcept{return phase_.load(std::memory_order_acquire);}
};
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
  bool cancelled=false,executed=false;
};
// This initial whole-module verse search bounds copied output only. SDK execution
// time/internal result allocation are not bounded. Cooperative cancellation is
// observed at SDK progress callbacks; range/last-search scopes remain separate.
}
