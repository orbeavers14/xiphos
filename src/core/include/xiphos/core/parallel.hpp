// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
namespace xiphos { namespace core {
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
