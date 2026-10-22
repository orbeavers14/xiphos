// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "search_scope.hpp"
namespace xiphos::core::detail {
struct SearchRetainedKeys {
  std::shared_ptr<const SearchScope> keys;
  bool capacity_exceeded=false;
};
// Called only on engine owner, after display sorting. May reuse the unsorted
// scope only when it is also display order. Aggregate capacity is independent.
SearchRetainedKeys retain_search_keys(const sword::ListKey &keys,std::shared_ptr<const SearchScope> reusable,SearchRetentionLimits limits);
void validate_search_generation_read(SearchReferenceSource source,const SearchLimits &limits);
void validate_search_retention_limits(SearchRetentionLimits limits);
std::uint64_t next_search_generation_id(std::uint64_t current);
class SearchGenerationBuilder;
class SearchGeneration {
  friend class SearchGenerationBuilder;
  struct Group {
    std::optional<SearchResult> result;
    SearchBatchFailure failure=SearchBatchFailure::none;
    std::shared_ptr<const SearchScope> keys;
  };
  SearchGenerationInfo info_;
  std::vector<SearchQuery> requested_;
  std::vector<Group> groups_;
public:
  // Private authoritative visible row; no SDK traversal or output expansion.
  std::optional<std::pair<std::string,std::string>> visible_row(std::size_t group,std::size_t row) const;
  // Plain committed request/result metadata only; no SDK traversal or mutable UI fields.
  std::string export_header(std::size_t group) const;
  SearchGenerationInfo info() const noexcept{return info_;}
  SearchGenerationRead read(std::uint64_t id,std::size_t group,SearchReferenceSource source,const SearchLimits &limits) const;
};
class SearchGenerationBuilder {
  SearchGeneration value_;
  SearchRetentionLimits limits_;
public:
  SearchGenerationBuilder(std::uint64_t id,std::vector<SearchQuery> queries,SearchRetentionLimits limits);
  bool wants_keys() const noexcept{return value_.info_.retention==SearchRetentionStatus::complete;}
  SearchRetentionLimits remaining() const noexcept;
  void append(std::size_t index,const std::optional<SearchResult> &result,SearchBatchFailure failure,SearchRetainedKeys keys);
  std::shared_ptr<const SearchGeneration> finish(bool cancelled);
};
}
