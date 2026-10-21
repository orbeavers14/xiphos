// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/verse_image.hpp"
#include "xiphos/core/search.hpp"
#include "xiphos/core/search_export.hpp"
#include <cstdint>
namespace xiphos::core::detail {
inline constexpr std::size_t maximum_export_image_pixels=maximum_search_export_image_pixels;
inline constexpr std::uint32_t maximum_export_image_side=16384;
enum class ExportPngStatus {ok,unsupported,output_limit,cancelled};
struct ExportPng {
  ExportPngStatus status=ExportPngStatus::unsupported;
  std::string data_url;
  std::size_t original_bytes=0,pixels=0;
  std::uint32_t width=0,height=0;
};
// Static PNG only: bounded complete container/CRC checks and actual libpng pixel
// decoding precede encoding of the original bytes. No file reads or SDK state.
// Limits cover logical bytes/pixels, not all codec temporaries or execution time.
ExportPng prepare_export_png(const VerseImageResource &source,std::size_t max_original_bytes,
    std::size_t max_encoded_bytes,std::size_t max_pixels,const std::shared_ptr<SearchControl> &control={});
}
