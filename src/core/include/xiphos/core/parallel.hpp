// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include "engine_options.hpp"
#include <string>
#include <vector>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
constexpr std::size_t maximum_parallel_columns = 32;
constexpr std::size_t maximum_parallel_module_bytes = 4096;
constexpr std::size_t maximum_parallel_reference_bytes = 32768;
constexpr std::size_t maximum_parallel_snapshot_bytes = 16 * 1024 * 1024;
bool parallel_read_arguments(const std::vector<std::string> &modules,
    const std::string &reference, std::size_t max_bytes) noexcept;
enum class ParallelColumnStatus { ready, missing, unsupported };
struct ParallelColumn {
  std::string requested_module, module;
  ParallelColumnStatus status;
  bool via_abbreviation, bible, right_to_left;
};
struct ParallelVerse { std::string reference, html; std::vector<std::string> headings; };
struct ParallelCell : ParallelVerse {
  // For an SDK split mapping, keep every numbered verse after the primary.
  std::vector<ParallelVerse> additional;
};
struct ParallelRow { std::string reference; unsigned ordinal; std::vector<ParallelCell> cells; };
struct ParallelChapter {
  std::string control_module, reference, title;
  unsigned selected_verse;
  std::vector<ParallelColumn> columns;
  std::vector<ParallelRow> rows;
  std::size_t snapshot_bytes = 0;
  bool annotation_incomplete=false;
  std::size_t invalid_colors=0;
};
// Lightweight committed per-tab intent, suitable for future session storage.
// Raw module tokens retain order; actual control/reference come from success.
struct ParallelSelection {
  std::vector<std::string> modules;
  std::string control_module, reference;
  EngineOptions options;
  bool headings;
  std::size_t max_bytes;
  bool map_versification = false;
};
struct ParallelReferences {
  std::string reference, title;
  unsigned selected_verse;
  std::vector<std::string> rows;
};
// Owned textual row plan following the retained first-control rule. Uses cloned
// VerseKeys and the control's own canon/locale; does not move the live module.
// Rejects invalid/non-verse input and chapters over1024 rows. This is retained
// textual alignment, not a claim of cross-canon verse equivalence. Expressions
// use SDK control-key interpretation before counting the canonical chapter.
ParallelReferences parallel_references(sword::SWModule &control,
    const std::string &reference, const char *locale = nullptr);
// Same-canon references and SDK systems with a verified mapping table.
// Returns no numbered verse for unsupported or unavailable mappings.
std::vector<std::string> parallel_mapped_references(sword::SWModule &source,
    sword::SWModule &target, const std::string &reference);
struct ParallelLayout {
  unsigned column_percent;
  std::size_t image_scale;
};
// Retained detached parallel layout. Counts raw columns, including duplicates
// and missing modules. No implicit cap, filtering or redistribution. Widths
// truncate (three columns ->33%, >100 ->0%); zero columns ->100%.
inline ParallelLayout parallel_layout(std::size_t columns) noexcept {
  return {columns ? static_cast<unsigned>(100 / columns) : 100,
    (columns / 3) * 2 + ((columns % 3) * 2) / 3};
}
}}
