// SPDX-License-Identifier: GPL-2.0-or-later
#include "export_png.hpp"
#include "xiphos/core/search_export.hpp"
#include "image_source_test_support.hpp"
#include <zlib.h>
#include <glib.h>
#include <iostream>
#include <future>
#include <cstring>
#include <stdexcept>
using namespace xiphos::core;
using namespace xiphos::core::detail;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void number(std::string &out,std::uint32_t n){for(int bit=24;bit>=0;bit-=8)out+=char((n>>bit)&255);}
std::string chunk(const std::string &type,const std::string &payload){
 std::string out;number(out,static_cast<std::uint32_t>(payload.size()));out+=type;out+=payload;
 number(out,static_cast<std::uint32_t>(crc32(crc32(0L,Z_NULL,0),reinterpret_cast<const Bytef *>(out.data()+4),static_cast<uInt>(out.size()-4))));return out;
}
std::string compressed(const std::string &raw){
 uLongf size=compressBound(raw.size());std::string out(size,'\0');
 check(compress2(reinterpret_cast<Bytef *>(out.data()),&size,reinterpret_cast<const Bytef *>(raw.data()),raw.size(),Z_BEST_SPEED)==Z_OK,"Create original compressed pixel fixture");out.resize(size);return out;
}
std::string one_pixel(unsigned depth,unsigned color,const std::string &row,const std::string &palette={}){
 std::string header;number(header,1);number(header,1);header+=char(depth);header+=char(color);header.append(3,'\0');
 return std::string("\x89PNG\r\n\x1a\n",8)+chunk("IHDR",header)+(palette.empty()?"":chunk("PLTE",palette))+chunk("IDAT",compressed(row))+chunk("IEND","");
}
ExportPng prepare(const std::string &data,std::size_t bytes=maximum_search_export_bytes,std::size_t encoded=maximum_search_export_bytes,std::size_t pixels=maximum_export_image_pixels){
 return prepare_export_png({"Original","original.png","image/png",data},bytes,encoded,pixels);
}
void empty_failure(const ExportPng &out,ExportPngStatus status){check(out.status==status&&out.data_url.empty()&&!out.width&&!out.height&&!out.original_bytes&&!out.pixels,"Failure has no partial URI or misleading successful image metadata");}
int main(){try{
 const auto png=image_test::preview_png();const auto out=prepare(png);
 check(out.status==ExportPngStatus::ok&&out.width==8&&out.height==8&&out.pixels==64&&out.original_bytes==png.size(),"Actual pixel decode and exact original source accounting");
 check(out.data_url.rfind("data:image/png;base64,",0)==0,"PNG data URL has fixed core-owned MIME");
 gsize length=0;std::unique_ptr<guchar,decltype(&g_free)> copied(g_base64_decode(out.data_url.substr(22).c_str(),&length),g_free);
 check(length==png.size()&&!std::memcmp(copied.get(),png.data(),png.size()),"Encoding preserves exact original PNG rather than lossy pixel conversion");
 check(prepare(png,png.size(),out.data_url.size(),64).data_url==out.data_url,"Exact aggregate input/output/pixel budgets accepted");
 empty_failure(prepare(png,png.size()-1),ExportPngStatus::output_limit);
 empty_failure(prepare(png,png.size(),out.data_url.size()-1),ExportPngStatus::output_limit);
 empty_failure(prepare(png,maximum_search_export_bytes,maximum_search_export_bytes,63),ExportPngStatus::output_limit);
 for(std::size_t n=0;n<png.size();++n)empty_failure(prepare(png.substr(0,n)),ExportPngStatus::unsupported);
 for(std::size_t i=0;i<png.size();++i){auto bad=png;bad[i]^=1;check(prepare(bad).status!=ExportPngStatus::ok,"Single-byte corruption never becomes a successful PNG artifact");}
 empty_failure(prepare(png+"<script>Original</script>"),ExportPngStatus::unsupported);
 const auto signature=png.substr(0,8),ihdr=png.substr(8,25),idat=png.substr(33,30),iend=png.substr(63,12);
 check(png.substr(37,4)=="IDAT"&&png.substr(67,4)=="IEND","Independent known original PNG chunk layout");
 empty_failure(prepare(signature+ihdr+iend),ExportPngStatus::unsupported);
 empty_failure(prepare(signature+ihdr+ihdr+idat+iend),ExportPngStatus::unsupported);
 empty_failure(prepare(signature+ihdr+chunk("RaNd","")+idat+iend),ExportPngStatus::unsupported);
 empty_failure(prepare(signature+ihdr+idat+chunk("raNd","")+idat+iend),ExportPngStatus::unsupported);
 for(const auto &type:{"acTL","fcTL","fdAT"})empty_failure(prepare(signature+ihdr+chunk(type,std::string(8,'\0'))+idat+iend),ExportPngStatus::unsupported);
 auto corrupt=png.substr(41,18);corrupt[0]=char(0xff);
 empty_failure(prepare(signature+ihdr+chunk("IDAT",corrupt)+iend),ExportPngStatus::unsupported); // Correct CRC, invalid compression stream.
 auto header=png.substr(16,13);header[0]=0;header[1]=0;header[2]=0;header[3]=0;
 empty_failure(prepare(signature+chunk("IHDR",header)+idat+iend),ExportPngStatus::unsupported);
 header=png.substr(16,13);header[0]=0;header[1]=0;header[2]=char(0x40);header[3]=1;
 empty_failure(prepare(signature+chunk("IHDR",header)+idat+iend),ExportPngStatus::output_limit);
 header=png.substr(16,13);header[8]=3; // Invalid bit depth, but CRC is correct.
 empty_failure(prepare(signature+chunk("IHDR",header)+idat+iend),ExportPngStatus::unsupported);
 std::string many=signature+ihdr;for(unsigned n=0;n<4094;++n)many+=chunk("raNd","");many+=idat+iend;
 empty_failure(prepare(many),ExportPngStatus::output_limit);
 for(const auto &original:std::vector<std::string>{
  one_pixel(1,0,std::string("\0\x80",2)),
  one_pixel(16,2,std::string("\0\x12\x34\x56\x78\x9a\xbc",7)),
  one_pixel(1,3,std::string("\0\x80",2),std::string("\0\0\0\xff\xff\xff",6)),
  one_pixel(8,4,std::string("\0\x80\xff",3)),
  one_pixel(8,6,std::string("\0\x24\x68\xac\xff",5))}){
  const auto pixel=prepare(original,original.size(),maximum_search_export_bytes,1);
  check(pixel.status==ExportPngStatus::ok&&pixel.width==1&&pixel.height==1,"Original grayscale/palette/16-bit/alpha pixels decode through shared codec");
 }
 auto cancelled=std::make_shared<SearchControl>();cancelled->request_cancel();
 empty_failure(prepare_export_png({"Original","original.png","image/png",png},png.size(),1024,64,cancelled),ExportPngStatus::cancelled);
 empty_failure(prepare_export_png({"Original","original.jpg","image/jpeg",png},png.size(),1024,64),ExportPngStatus::unsupported);
 for(unsigned field=0;field<3;++field)for(auto budget:{std::size_t{0},(field==2?maximum_export_image_pixels:maximum_search_export_bytes)+1}){
  std::size_t limits[]={maximum_search_export_bytes,maximum_search_export_bytes,maximum_export_image_pixels};limits[field]=budget;
  bool rejected=false;try{prepare(png,limits[0],limits[1],limits[2]);}catch(const std::invalid_argument &){rejected=true;}check(rejected,"Invalid PNG byte/encoded/pixel budget");
 }
 std::vector<std::future<ExportPng>> concurrent;for(unsigned n=0;n<8;++n)concurrent.push_back(std::async(std::launch::async,[png]{return prepare(png);}));
 for(auto &result:concurrent)check(result.get().data_url==out.data_url,"Independent codec state remains reentrant with immutable input");
 std::cout<<"Bounded static PNG container/CRC/pixel validation, exact encoding, cancellation and reentrancy passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
