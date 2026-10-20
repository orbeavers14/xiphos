// SPDX-License-Identifier: GPL-2.0-or-later
#include "export_images.hpp"
#include "html_document.hpp"
#include "bounded_file.hpp"
#include "image_source_test_support.hpp"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
using namespace xiphos::core::detail;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 const auto png=image_test::preview_png();const auto encoded=prepare_export_png({"Text","preview.png","image/png",png},75,8192,64);
 check(encoded.status==ExportPngStatus::ok,"Original PNG must decode");
 std::size_t loads=0;
 auto load=[&](const std::string &module,const std::string &source,std::size_t max){
  ++loads;check(max>0&&max<=maximum_search_export_bytes,"Loader receives remaining positive raw budget");
  return VerseImageResource{module,source,"image/png",png};
 };
 ExportImages cache({3,150,128,encoded.data_url.size()*2},load);
 check(cache.resolve("Text","preview.png").data_url==encoded.data_url&&cache.resolve("Text","preview.png").data_url==encoded.data_url&&loads==1,"Repeated identity reuses the first validated copy");
 check(cache.resolve("Notes","preview.png").status==ExportPngStatus::ok&&loads==2,"Same URI in a different source module is a distinct resource");
 const auto stats=cache.statistics();check(stats.count==3&&stats.unique_count==2&&stats.original_bytes==150&&stats.pixels==128,"Occurrence and unique-byte/pixel accounting differ deliberately");
 check(cache.resolve("Text","preview.png").status==ExportPngStatus::output_limit&&loads==2,"Occurrence limit includes cache hits and prevents further loads");
 for(unsigned which=0;which<3;++which){
  ExportImageLimits limits{4,150,128,encoded.data_url.size()*2};
  if(which==0)--limits.original_bytes;if(which==1)--limits.pixels;if(which==2)--limits.encoded_bytes;
  ExportImages limited(limits,load);check(limited.resolve("A","one").status==ExportPngStatus::ok&&limited.resolve("A","two").status==ExportPngStatus::output_limit,"One below aggregate raw/pixel/encoded boundary rejects second unique resource");
  check(limited.statistics().unique_count==1,"Failed resource does not publish successful statistics");
 }
 for(unsigned which=0;which<5;++which){
  ExportImageLimits limits;if(which==0)limits.count=0;if(which==1)limits.count=65;if(which==2)limits.original_bytes=maximum_search_export_bytes+1;if(which==3)limits.pixels=0;if(which==4)limits.encoded_bytes=0;
  bool rejected=false;try{ExportImages invalid(limits,load);}catch(const std::invalid_argument &){rejected=true;}check(rejected,"Invalid aggregate image budget rejects before loading");
 }
 for(unsigned which=0;which<3;++which){
  ExportImages failing({},[&](const auto &,const auto &,std::size_t)->VerseImageResource{
   if(which==0)throw BoundedFileLimitError("Original test limit");
   if(which==1)throw std::invalid_argument("Original missing file");
   throw std::runtime_error("Original changed file");
  });
  check(failing.resolve("A","one").status==(which==0?ExportPngStatus::output_limit:ExportPngStatus::unsupported)&&!failing.statistics().count,"Typed loader limit is distinct from unsupported/unavailable assets");
 }
 auto control=std::make_shared<SearchControl>();
 ExportImages cancelled({},[&](const auto &module,const auto &source,std::size_t){control->request_cancel();return VerseImageResource{module,source,"image/png",png};},control);
 check(cancelled.resolve("Text","preview.png").status==ExportPngStatus::cancelled&&!cancelled.statistics().count,"Cancellation after a bounded read publishes no successful asset");
 const auto before=loads;ExportImages prestopped({},load,control);
 check(prestopped.resolve("Text","preview.png").status==ExportPngStatus::cancelled&&loads==before,"Pre-cancel does not call the loader");
 ExportImages document({},load);auto resolve=[&](const std::string &source){return document.resolve("Text",source);};
 EntryHtmlSource source;source.html="<p>Original café<img src=\"a&amp;b.png\" alt=\"Original &amp; blue\" width=\"8\" height=\"8\"></p>";
 auto output=prepare_export_html(source,8192,resolve);
 check(output.issue==HtmlExportIssue::none&&!output.limited&&image_test::images(output.html)==std::vector<std::string>{encoded.data_url}&&output.html.find("Original &amp; blue")!=std::string::npos,"Opt-in fragment replaces URI with validated PNG while preserving alt and dimensions");
 check(image_test::attributes(output.html,"img","width")==std::vector<std::string>{"8"},"Explicit image dimensions survive canonical serialization");
 const auto prior=loads;
 for(const auto &html:{"<img src=\"preview.png\"><script>bad</script>","<img src=\"preview.png\" onerror=\"bad()\">","<img src=\"preview.png\" srcset=\"other.png 2x\">","<img src=\"preview.png\" style=\"display:none\">","<img>","<img src=\"\">","<video src=\"original.mp4\"></video>"}){
  EntryHtmlSource invalid;invalid.html=html;auto result=prepare_export_html(invalid,8192,resolve);
  check(result.issue!=HtmlExportIssue::none&&result.html.empty()&&loads==prior,"Entire structure validates before any resource read");
 }
 auto failure=[](const std::string &){return ExportPng{};};
 check(prepare_export_html(source,8192,failure).issue==HtmlExportIssue::resources,"Unsupported resource rejects entire fragment");
 auto stop=[](const std::string &){ExportPng out;out.status=ExportPngStatus::cancelled;return out;};
 const auto stopped=prepare_export_html(source,8192,stop);check(stopped.cancelled&&stopped.html.empty(),"Cancelled resource rejects entire fragment");
 EntryHtmlSource many;for(unsigned i=0;i<65;++i)many.html+="<img src=\"preview.png\">";
 const auto many_result=prepare_export_html(many,8192,resolve);check(many_result.limited&&many_result.html.empty()&&loads==prior,"Per-fragment occurrence guard runs before resource resolution");
 // Serialized output, including source prose/attributes, has its own final cap.
 check(prepare_export_html(source,output.html.size(),resolve).html==output.html,"Exact serialized fragment budget accepted");
 check(prepare_export_html(source,output.html.size()-1,resolve).limited,"One byte short never returns clipped HTML");
 std::cout<<"Per-document image cache, strict aggregate limits, complete DOM policy and cancellation passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
