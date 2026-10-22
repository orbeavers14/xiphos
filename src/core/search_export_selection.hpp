// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/search_export.hpp"
#include <utility>
namespace xiphos::core::detail {
class SearchGeneration;
struct SearchExportRow {
  std::string first,second; // Actual module/key, preserving the existing formatter inputs.
  std::size_t group=0;
  std::shared_ptr<const std::string> context; // One copied header per emitting group.
};
using SearchExportRows=std::vector<SearchExportRow>;
struct SearchExportDocument {
  SearchExportRows rows;
  std::string source_summary; // Copied requested-group dispositions, including no-row sources.
};
void validate_search_export_output(const std::string &title,std::size_t bytes);
// Engine-owner only. The same stale/capacity/partial/ordinal policy serves all
// document formats. Output consists solely of copied module/key pairs/metadata.
SearchExportMetadata resolve_search_export_rows(const SearchGeneration *generation,std::uint64_t expected,
  SearchExportSelection selection,const std::vector<SearchVisibleRow> &selected,std::size_t max_rows,
  bool allow_partial,const std::shared_ptr<SearchControl> &control,SearchExportDocument &document,bool include_context=false);
}
