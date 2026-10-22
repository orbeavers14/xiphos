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
enum class SearchExportStatus {ok=0,stale=1,invalid_selection=2,output_limit=3,cancelled=4,module_not_found=5,engine_error=6,capacity_exceeded=7,source_incomplete=8,unsupported_html=9};
enum class SearchExportSelection {visible=0,all_retained=1};
struct SearchExportMetadata {
  SearchExportStatus status=SearchExportStatus::stale;
  SearchGenerationInfo generation;
  std::size_t row_count=0;
  std::optional<std::size_t> failed_row;
  bool includes_scripture=false;
  SearchExportSelection selection=SearchExportSelection::visible;
};
struct SearchTextExport:SearchExportMetadata {std::string text;};
enum class HtmlExportIssue {none=0,resources=1,markup=2,styles=3,links=4};
inline constexpr std::size_t maximum_search_export_images=64;
inline constexpr std::size_t maximum_search_export_image_pixels=16*1024*1024;
enum class SearchHtmlImagePolicy {reject=0,embedded_static_png=1};
struct SearchHtmlImageOptions {
  SearchHtmlImagePolicy policy=SearchHtmlImagePolicy::reject;
  std::size_t max_images=maximum_search_export_images;
  std::size_t max_original_bytes=maximum_search_export_bytes;
  std::size_t max_pixels=maximum_search_export_image_pixels;
};
// Counts are published only with a complete successful document. Occurrences
// include duplicates; bytes/pixels count unique (actual module, source URI) assets.
struct SearchHtmlImageStatistics {
  std::size_t count=0,unique_count=0,original_bytes=0,pixels=0;
};
struct SearchHtmlExport:SearchExportMetadata {
  std::string html;
  std::optional<std::string> plain_text; // Present only for a requested, complete rich export.
  HtmlExportIssue issue=HtmlExportIssue::none;
  SearchHtmlImageStatistics images;
};
struct SearchHtmlExportRequest {
  std::uint64_t generation=0;
  SearchExportSelection selection=SearchExportSelection::visible;
  std::vector<SearchVisibleRow> rows; // Empty for all_retained.
  std::string title="Search results";
  bool include_scripture=false;
  std::size_t max_bytes=1024*1024,max_rows=maximum_search_export_rows;
  bool allow_partial_source=false; // Applies only to all_retained.
  SearchHtmlImageOptions images; // Explicit opt-in; existing callers reject resources.
  std::optional<std::size_t> plain_text_max_bytes; // Independent UTF-8 cap; no fallback by default.
};
// Selection/assembly/rendering run on one owner. Output cap bounds final copied
// bytes, not SDK temporary allocation/time. No destination write or history visit.
}
