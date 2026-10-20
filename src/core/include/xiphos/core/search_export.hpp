// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "search.hpp"
namespace xiphos::core {
// Ordinals refer to the retained visible prefix, never a reconstructed key list.
// Request order and duplicates are significant. No all-results implication.
struct SearchVisibleRow {std::size_t group=0,row=0;};
inline constexpr std::size_t maximum_search_export_rows=10000;
inline constexpr std::size_t maximum_search_export_bytes=8*1024*1024;
struct SearchTextExportRequest {
  std::uint64_t generation=0;
  std::vector<SearchVisibleRow> rows;
  std::string title="Search results"; // Plain UTF-8 label, caller may localize.
  bool include_scripture=false;
  std::size_t max_bytes=1024*1024;
};
// All returned retained matches, not all requested modules. Source failures,
// unattempted groups or cancellation require explicit partial-source acceptance.
struct SearchAllTextExportRequest {
  std::uint64_t generation=0;
  std::string title="Search results";
  bool include_scripture=false;
  std::size_t max_bytes=1024*1024,max_rows=maximum_search_export_rows;
  bool allow_partial_source=false;
};
enum class SearchExportStatus {ok=0,stale=1,invalid_selection=2,output_limit=3,cancelled=4,module_not_found=5,engine_error=6,capacity_exceeded=7,source_incomplete=8};
enum class SearchExportSelection {visible=0,all_retained=1};
struct SearchTextExport {
  SearchExportStatus status=SearchExportStatus::stale;
  SearchGenerationInfo generation; // Current metadata even on stale request.
  std::size_t row_count=0; // Zero on failure; text is never a partial document.
  std::optional<std::size_t> failed_row; // Request ordinal, when applicable.
  bool includes_scripture=false;
  std::string text;
  SearchExportSelection selection=SearchExportSelection::visible;
};
// Selection/assembly/rendering run on one owner. Output cap bounds final copied
// bytes, not SDK temporary allocation/time. No destination write or history visit.
}
