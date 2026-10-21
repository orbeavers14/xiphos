// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "export_png.hpp"
#include <functional>
#include <map>
namespace xiphos::core::detail {
inline constexpr std::size_t maximum_export_image_count=maximum_search_export_images;
struct ExportImageLimits {
  std::size_t count=maximum_export_image_count,original_bytes=maximum_search_export_bytes;
  std::size_t pixels=maximum_export_image_pixels,encoded_bytes=maximum_search_export_bytes;
};
using ExportImageStatistics=SearchHtmlImageStatistics;
// Private per-document cache. Identity includes the actual source module and
// exact decoded DOM URI. No persistent cache, filesystem naming or SDK mutation.
class ExportImages {
public:
  using Loader=std::function<VerseImageResource(const std::string &,const std::string &,std::size_t)>;
  ExportImages(ExportImageLimits limits,Loader loader,std::shared_ptr<SearchControl> control={});
  ExportPng resolve(const std::string &module,const std::string &source);
  ExportImageStatistics statistics() const noexcept{return statistics_;}
private:
  ExportImageLimits limits_;Loader loader_;std::shared_ptr<SearchControl> control_;
  ExportImageStatistics statistics_;std::size_t encoded_bytes_=0;
  std::map<std::pair<std::string,std::string>,ExportPng> cache_;
};
}
