// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <string>
#include <vector>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
struct ParallelReferences {
  std::string reference, title;
  unsigned selected_verse;
  std::vector<std::string> rows;
};
// Owned textual row plan following the retained first-control rule. Uses cloned
// VerseKeys and the control's own canon/locale; does not move the live module.
// Rejects invalid/non-verse input and chapters over1024 rows. This is retained
// textual alignment, not a claim of cross-canon verse equivalence.
ParallelReferences parallel_references(sword::SWModule &control,
    const std::string &reference, const char *locale = nullptr);
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
