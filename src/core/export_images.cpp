// SPDX-License-Identifier: GPL-2.0-or-later
#include "export_images.hpp"
#include "bounded_file.hpp"
#include "xiphos/core/search_export.hpp"
#include <stdexcept>
namespace xiphos::core::detail {
ExportImages::ExportImages(ExportImageLimits limits,Loader loader,std::shared_ptr<SearchControl> control)
  :limits_(limits),loader_(std::move(loader)),control_(std::move(control)){
  if(!loader_||!limits.count||limits.count>maximum_export_image_count||!limits.original_bytes||
     limits.original_bytes>maximum_search_export_bytes||!limits.pixels||limits.pixels>maximum_export_image_pixels||
     !limits.encoded_bytes||limits.encoded_bytes>maximum_search_export_bytes)
    throw std::invalid_argument("Invalid HTML image export limits or loader");
}
ExportPng ExportImages::resolve(const std::string &module,const std::string &source){
  const auto stopped=[&]{return control_&&control_->cancel_requested();};
  const auto fail=[](ExportPngStatus status){ExportPng out;out.status=status;return out;};
  if(stopped())return fail(ExportPngStatus::cancelled);
  if(statistics_.count==limits_.count)return fail(ExportPngStatus::output_limit);
  const auto key=std::make_pair(module,source);const auto found=cache_.find(key);
  if(found!=cache_.end()){++statistics_.count;return found->second;}
  const auto bytes=limits_.original_bytes-statistics_.original_bytes;
  const auto pixels=limits_.pixels-statistics_.pixels;
  const auto encoded=limits_.encoded_bytes-encoded_bytes_;
  if(!bytes||!pixels||!encoded)return fail(ExportPngStatus::output_limit);
  VerseImageResource resource;
  try{resource=loader_(module,source,bytes);}
  catch(const BoundedFileLimitError &){return fail(stopped()?ExportPngStatus::cancelled:ExportPngStatus::output_limit);}
  catch(const std::invalid_argument &){return fail(stopped()?ExportPngStatus::cancelled:ExportPngStatus::unsupported);}
  catch(const std::runtime_error &){return fail(stopped()?ExportPngStatus::cancelled:ExportPngStatus::unsupported);}
  if(stopped())return fail(ExportPngStatus::cancelled);
  auto image=prepare_export_png(resource,bytes,encoded,pixels,control_);
  if(image.status!=ExportPngStatus::ok)return image;
  cache_.emplace(key,image);++statistics_.count;++statistics_.unique_count;
  statistics_.original_bytes+=image.original_bytes;statistics_.pixels+=image.pixels;encoded_bytes_+=image.data_url.size();
  return image;
}
}
