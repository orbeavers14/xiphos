// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <memory>
#include <optional>
#include "search_policy.hpp"
namespace xiphos::core {
namespace detail { class SearchControlAccess; class SearchGeneration; struct SearchRetainedKeys; }
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
// Attribute modes use legacy SDK paths; lemma/morph require whole-entry matching.
enum class SearchMode { regular_expression=0, phrase=1, words=2, lemma=3, morphology=4, footnote=5, multiword=6, indexed=7 };
// Legacy sidebar range uses the designated Bible module's own VerseKey parser,
// which may differ from the searched commentary module's versification.
struct SearchRange { std::string module, expression; };
struct SearchQuery {
  std::string module, expression;
  SearchMode mode=SearchMode::phrase;
  bool case_sensitive=false;
  std::optional<SearchRange> range;
  bool within_last_results=false; // Session-only; mutually exclusive with range.
  std::optional<SearchTextOptions> text_options; // Exact phrase preserves prior search settings.
};
struct SearchLimits {
  std::size_t max_results=1000;
  std::size_t max_reference_bytes=1024*1024;
};
inline constexpr std::size_t maximum_search_effective_bytes=65536;
inline constexpr std::size_t maximum_search_results=10000;
inline constexpr std::size_t maximum_search_reference_bytes=8*1024*1024;
struct SearchTextState { std::optional<SearchTextOptions> options; };
struct SearchScopeInfo { bool present=false; std::uint64_t element_count=0; };
inline constexpr std::size_t maximum_search_retained_elements=100000;
inline constexpr std::size_t maximum_search_retained_key_bytes=16*1024*1024;
struct SearchRetentionLimits {
  std::size_t max_elements=maximum_search_retained_elements;
  std::size_t max_key_bytes=maximum_search_retained_key_bytes;
};
enum class SearchRetentionStatus { none=0, complete=1, capacity_exceeded=2 };
// ID is nonzero only for a committed session result generation, scoped to its
// live tab/session; never persist it across worker/tab replacement. Complete
// means all RETURNED keys retained, not every requested module ran.
struct SearchGenerationInfo {
  std::uint64_t id=0;
  SearchRetentionStatus retention=SearchRetentionStatus::none;
  std::size_t requested_count=0,group_count=0,failed_groups=0,visible_results=0;
  std::uint64_t matched_elements=0,retained_elements=0;
  std::size_t accounted_key_bytes=0;
  bool cancelled=false;
};
struct SearchResult {
  SearchQuery requested;
  std::string effective_expression; // Exact copied module stripText query.
  int engine_type=0; // Selected SDK type; words cancelled before probing retain multiword fallback.
  std::uint64_t total_results=0; // SDK list count, not a promise about expanded ranges.
  std::vector<std::string> references; // Complete owned SDK keys, ordered, never byte-truncated.
  bool limited=false;
  bool cancelled=false,executed=false;
  SearchScopeInfo scope; // Current session scope after this request; no SDK keys.
  bool scope_updated=false;
  std::optional<SearchTextOptions> effective_text_options; // Named override policy prepared for this request.
  bool text_options_updated=false;
  bool index_fallback=false; // Explicit indexed request selected unindexed multiword instead.
  SearchGenerationInfo generation;
  bool generation_updated=false;
};
// One serialized job, in explicit request order (including duplicate modules).
// Last-results requests chain through earlier successful groups, never a union.
inline constexpr std::size_t maximum_search_batch_modules=32;
enum class SearchBatchFailure { none, module_not_found, invalid_request, engine_error };
struct SearchBatchGroup {
  std::size_t request_index=0;
  std::optional<SearchResult> result;
  SearchBatchFailure failure=SearchBatchFailure::none;
};
struct SearchBatchLimits {
  SearchLimits per_module;
  SearchLimits total{maximum_search_results,maximum_search_reference_bytes};
};
struct SearchBatchResult {
  std::vector<SearchQuery> requested;
  std::vector<SearchBatchGroup> groups; // Attempted prefix; failures keep their index.
  std::size_t copied_results=0,copied_reference_bytes=0,failures=0;
  bool limited=false,cancelled=false,scope_updated=false,text_options_updated=false;
  SearchScopeInfo scope;
  SearchTextState text_options;
  SearchGenerationInfo generation;
  bool generation_updated=false;
};
enum class SearchReferenceSource { visible=0, complete=1 };
enum class SearchGenerationReadStatus { ok=0, stale=1, invalid_group=2, not_attempted=3, failed_group=4, capacity_exceeded=5 };
struct SearchGenerationRead {
  SearchGenerationReadStatus status=SearchGenerationReadStatus::stale;
  SearchGenerationInfo generation; // Current committed generation even on stale ID.
  std::optional<SearchQuery> requested; // Present for a valid requested group.
  SearchBatchFailure failure=SearchBatchFailure::none;
  std::optional<SearchResult> result; // Owned bounded references only on ok.
};
// This initial verse search bounds copied output only. SDK execution
// time/internal result allocation are not bounded. Cooperative cancellation is
// observed at SDK progress callbacks. Explicit ranges stay as SDK interval keys,
// not expanded verse lists. Sessions retain complete scope independently of output.
}
