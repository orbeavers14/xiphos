// SPDX-License-Identifier: GPL-2.0-or-later
#include "export_png.hpp"
#include "xiphos/core/search_export.hpp"
#include <png.h>
#include <zlib.h>
#include <glib.h>
#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>
#ifndef PNG_SIMPLIFIED_WRITE_SUPPORTED
#error Shared bitmap export requires libpng simplified write support
#endif
#ifndef PNG_SIMPLIFIED_READ_SUPPORTED
#error Shared PNG export validation requires libpng simplified read support
#endif
namespace xiphos::core::detail {
namespace {
std::uint32_t word(const unsigned char *p){
  return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3];
}
struct PngState {png_image value{};PngState(){value.version=PNG_IMAGE_VERSION;}~PngState(){png_image_free(&value);}};
}
ExportPng prepare_export_png(const VerseImageResource &source,std::size_t max_original_bytes,
    std::size_t max_encoded_bytes,std::size_t max_pixels,const std::shared_ptr<SearchControl> &control){
  if(!max_original_bytes||max_original_bytes>maximum_search_export_bytes||!max_encoded_bytes||
     max_encoded_bytes>maximum_search_export_bytes||!max_pixels||max_pixels>maximum_export_image_pixels)
    throw std::invalid_argument("Invalid export PNG budget");
  ExportPng out;
  const auto fail=[&](ExportPngStatus status){out.status=status;return out;};
  const auto stopped=[&]{return control&&control->cancel_requested();};
  if(stopped())return fail(ExportPngStatus::cancelled);
  const auto &data=source.data;
  if(data.size()>max_original_bytes)return fail(ExportPngStatus::output_limit);
  if(source.media_type!="image/png"||data.size()<8)return out;
  const auto *bytes=reinterpret_cast<const unsigned char *>(data.data());
  if(png_sig_cmp(bytes,0,8))return out;
  const std::string prefix="data:image/png;base64,";
  const auto encoded_size=prefix.size()+((data.size()+2)/3)*4;
  if(encoded_size>max_encoded_bytes)return fail(ExportPngStatus::output_limit);
  std::size_t position=8,chunks=0,pixels=0;std::uint32_t width=0,height=0;
  bool header=false,palette=false,idat=false,end_data=false,ended=false;
  while(position<data.size()){
    if(stopped())return fail(ExportPngStatus::cancelled);
    if(++chunks>4096)return fail(ExportPngStatus::output_limit);
    if(data.size()-position<12)return out;
    const auto length=word(bytes+position);
    if(length>0x7fffffffU||length>data.size()-position-12)return out;
    const std::string_view type(data.data()+position+4,4);
    if(!std::all_of(type.begin(),type.end(),[](unsigned char c){return (c>='A'&&c<='Z')||(c>='a'&&c<='z');})||
       type[2]<'A'||type[2]>'Z')return out;
    static_assert(std::numeric_limits<uInt>::max()>maximum_search_export_bytes,"PNG CRC input must not narrow");
    const auto crc=crc32(crc32(0L,Z_NULL,0),bytes+position+4,static_cast<uInt>(length+4));
    if(crc!=word(bytes+position+8+length))return out;
    if(!header){
      if(type!="IHDR"||length!=13)return out;
      width=word(bytes+position+8);height=word(bytes+position+12);
      if(!width||!height)return out;
      const auto count=std::uint64_t(width)*height;
      if(width>maximum_export_image_side||height>maximum_export_image_side||count>max_pixels)
        return fail(ExportPngStatus::output_limit);
      pixels=static_cast<std::size_t>(count);header=true;
    }else if(type=="IHDR")return out;
    else if(type=="PLTE"){
      if(palette||idat||!length||length>768||length%3)return out;
      palette=true;
    }else if(type=="IDAT"){
      if(end_data)return out;
      idat=true;
    }else if(type=="IEND"){
      if(length||!idat||position+12!=data.size())return out;
      ended=true;break;
    }else{
      // Do not validate only a default frame and silently export an animation.
      if(type=="acTL"||type=="fcTL"||type=="fdAT"||type[0]<'a'||type[0]>'z')return out;
      if(idat)end_data=true;
    }
    position+=12+length;
  }
  if(!header||!idat||!ended)return out;
  if(stopped())return fail(ExportPngStatus::cancelled);
  {
    PngState png;
    if(!png_image_begin_read_from_memory(&png.value,data.data(),data.size()))return out;
    if(png.value.width!=width||png.value.height!=height)return out;
    png.value.format=PNG_FORMAT_RGBA;
    std::vector<png_byte> decoded(pixels*4);
    if(!png_image_finish_read(&png.value,nullptr,decoded.data(),0,nullptr)||png.value.warning_or_error)return out;
  } // Release decoded pixels and codec state before making the encoded copy.
  if(stopped())return fail(ExportPngStatus::cancelled);
  std::unique_ptr<gchar,decltype(&g_free)> encoded(g_base64_encode(bytes,data.size()),g_free);
  auto uri=prefix+encoded.get();
  if(uri.size()!=encoded_size)throw std::logic_error("PNG data URL accounting mismatch");
  if(stopped())return fail(ExportPngStatus::cancelled);
  out.status=ExportPngStatus::ok;out.data_url=std::move(uri);out.width=width;out.height=height;
  out.pixels=pixels;out.original_bytes=data.size();return out;
}
ExportPng prepare_export_bitmap(const VerseImageResource &source,std::size_t max_original_bytes,
    std::size_t max_encoded_bytes,std::size_t max_pixels,const std::shared_ptr<SearchControl> &control){
  if(source.media_type=="image/png")return prepare_export_png(source,max_original_bytes,max_encoded_bytes,max_pixels,control);
  if(!max_original_bytes||max_original_bytes>maximum_search_export_bytes||!max_encoded_bytes||max_encoded_bytes>maximum_search_export_bytes||!max_pixels||max_pixels>maximum_export_image_pixels)throw std::invalid_argument("Invalid bitmap export budget");
  ExportPng out;const auto fail=[&](ExportPngStatus status){out.status=status;return out;};
  const auto stopped=[&]{return control&&control->cancel_requested();};
  if(stopped())return fail(ExportPngStatus::cancelled);
  const auto &data=source.data;if(data.size()>max_original_bytes)return fail(ExportPngStatus::output_limit);
  if(source.media_type!="image/bmp"||data.size()<54||data.compare(0,2,"BM"))return out;
  const auto u16=[&](std::size_t at){return unsigned(static_cast<unsigned char>(data[at]))|(unsigned(static_cast<unsigned char>(data[at+1]))<<8);};
  const auto u32=[&](std::size_t at){return std::uint32_t(u16(at))|(std::uint32_t(u16(at+2))<<16);};
  // Exact supported container; reject palettes, masks, compressed/premultiplied
  // variants, trailing payloads and offsets outside the fixed header.
  if(u32(2)!=data.size()||u32(6)||u32(10)!=54||u32(14)!=40||u16(26)!=1||u16(28)!=24||u32(30)||u32(46)||u32(50))return out;
  const auto width=u32(18),signed_height=u32(22);
  if(!width||width>0x7fffffffU||!signed_height)return out;
  const bool top_down=(signed_height&0x80000000U)!=0;
  const auto height=top_down?std::uint64_t(0x100000000ULL)-signed_height:std::uint64_t(signed_height);
  const auto pixels=std::uint64_t(width)*height;
  if(width>maximum_export_image_side||height>maximum_export_image_side||pixels>max_pixels)return fail(ExportPngStatus::output_limit);
  const auto stride=(std::uint64_t(width)*3+3)&~std::uint64_t(3),raw_size=stride*height;
  if(raw_size!=data.size()-54||(u32(34)&&u32(34)!=raw_size))return out;
  std::vector<png_byte> rgb(static_cast<std::size_t>(pixels)*3);
  for(std::size_t y=0;y<height;++y){if(stopped())return fail(ExportPngStatus::cancelled);const auto row=54+(top_down?y:height-1-y)*stride;
    for(std::size_t x=0;x<width;++x){const auto from=row+x*3;const auto to=(y*width+x)*3;rgb[to]=static_cast<unsigned char>(data[from+2]);rgb[to+1]=static_cast<unsigned char>(data[from+1]);rgb[to+2]=static_cast<unsigned char>(data[from]);}}
  PngState png;png.value.width=width;png.value.height=static_cast<png_uint_32>(height);png.value.format=PNG_FORMAT_RGB;
  png_alloc_size_t size=0;if(!png_image_write_to_memory(&png.value,nullptr,&size,0,rgb.data(),0,nullptr))return out;
  const std::string prefix="data:image/png;base64,";
  if(size>maximum_search_export_bytes||prefix.size()+((size+2)/3)*4>max_encoded_bytes)return fail(ExportPngStatus::output_limit);
  if(stopped())return fail(ExportPngStatus::cancelled);
  std::string encoded_png(size,'\0');if(!png_image_write_to_memory(&png.value,encoded_png.data(),&size,0,rgb.data(),0,nullptr))return out;encoded_png.resize(size);
  if(stopped())return fail(ExportPngStatus::cancelled);
  std::unique_ptr<gchar,decltype(&g_free)> encoded(g_base64_encode(reinterpret_cast<const guchar *>(encoded_png.data()),encoded_png.size()),g_free);
  const auto uri=prefix+encoded.get();if(uri.size()>max_encoded_bytes)return fail(ExportPngStatus::output_limit);
  if(stopped())return fail(ExportPngStatus::cancelled);
  out.status=ExportPngStatus::ok;out.data_url=uri;out.width=width;out.height=static_cast<std::uint32_t>(height);out.pixels=static_cast<std::size_t>(pixels);out.original_bytes=data.size();return out;
}

}
