// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
xc_status wait(xc_request *request){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(20);xc_status status;
 while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<deadline,"C image export deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;
}
int main(int argc,char **argv){try{
 check(argc==2,"Original image fixture required");const auto root=(std::filesystem::path(argv[1])/std::filesystem::u8path("library %20 café")).string();
 xc_worker *worker=nullptr;xc_request *request=nullptr;
 check(xc_worker_open(root.c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create image-export worker and tab");
 const auto tab=xc_request_tab(request);xc_request_release(request);
 xc_search_query query;xc_search_query_default(&query);query.module="OSISNotes";query.expression="Original";
 check(xc_worker_search(worker,tab,&query,&request)==XC_OK&&wait(request)==XC_OK,"Search real original commentary image source");
 auto *search=xc_request_take_search(request);xc_request_release(request);xc_search_info source{};xc_search_generation_info generation{};
 check(search&&xc_search_get_info(search,&source)==XC_OK&&xc_search_get_generation(search,&generation)==XC_OK,"Copy retained image search metadata");
 std::size_t valid=source.result_count,missing=source.result_count;
 for(std::size_t i=0;i<source.result_count;++i){const std::string reference=xc_search_reference(search,i);if(reference=="John 1:8")valid=i;if(reference=="John 1:5")missing=i;}
 check(valid<source.result_count&&missing<source.result_count,"Original valid/missing rows present");xc_search_release(search);
 xc_search_visible_row rows[]={{0,valid},{0,valid}};
 xc_search_html_export_request input;xc_search_html_export_request_default(&input);input.generation=generation.id;input.include_scripture=1;input.rows=rows;input.row_count=2;
 xc_search_html_image_options images;xc_search_html_image_options_default(&images);
 check(images.policy==XC_HTML_IMAGES_REJECT&&images.max_images==64&&images.max_original_bytes==8*1024*1024&&images.max_pixels==16*1024*1024,"C image defaults explicit and bounded");
 auto take=[&]{check(wait(request)==XC_OK,"Image export outcome ready");auto *out=xc_request_take_search_html_export(request);check(out&&!xc_request_take_search_html_export(request)&&!xc_request_take_verse_image(request),"Document take once distinct from source image");xc_request_release(request);return out;};
 check(xc_worker_export_search_html(worker,tab,&input,&request)==XC_OK,"Existing function still accepted");auto *plain=take();xc_search_html_export_info info{};xc_search_html_image_info stats{};
 check(xc_search_html_export_get_info(plain,&info)==XC_OK&&info.disposition==XC_SEARCH_EXPORT_UNSUPPORTED_HTML&&xc_search_html_export_get_images(plain,&stats)==XC_OK&&!stats.count,"Existing ABI default rejects image-dependent output");xc_search_html_export_release(plain);
 images.policy=XC_HTML_IMAGES_EMBEDDED_STATIC_PNG;images.max_images=2;images.max_original_bytes=75;images.max_pixels=64;
 check(xc_worker_export_search_html_with_images(worker,tab,&input,&images,&request)==XC_OK&&!xc_request_take_search_html_export(request),"Accept image options before polling");
 images.max_pixels=1;rows[0]={99,99};auto *owned=take();
 check(xc_search_html_export_get_info(owned,&info)==XC_OK&&info.disposition==XC_SEARCH_EXPORT_OK&&info.row_count==2&&xc_search_html_export_get_images(owned,&stats)==XC_OK&&stats.count==2&&stats.unique_count==1&&stats.original_bytes==75&&stats.pixels==64,"C input options/rows copied; duplicate image counts use one owned source");
 const std::string html=xc_search_html_export_html(owned);check(html.find("img-src data:")!=std::string::npos&&html.find("data:image/png;base64,")!=std::string::npos,"Complete PNG document is borrowed until result release");
 images.max_pixels=64;rows[0]={0,valid};
 auto run=[&]{check(xc_worker_export_search_html_with_images(worker,tab,&input,&images,&request)==XC_OK,"Submit image policy");return take();};
 auto failure=[&](int expected){auto *out=run();check(xc_search_html_export_get_info(out,&info)==XC_OK&&info.disposition==expected&&xc_search_html_export_get_images(out,&stats)==XC_OK&&!stats.count&&!stats.unique_count&&!stats.original_bytes&&!stats.pixels&&!xc_search_html_export_size(out),"Failure clears all asset counts and document");xc_search_html_export_release(out);};
 input.max_bytes=html.size();auto *exact=run();check(std::string(xc_search_html_export_html(exact))==html,"Exact final C HTML image byte cap accepted");xc_search_html_export_release(exact);
 --input.max_bytes;failure(XC_SEARCH_EXPORT_OUTPUT_LIMIT);input.max_bytes=1024*1024;
 images.max_original_bytes=74;failure(XC_SEARCH_EXPORT_OUTPUT_LIMIT);images.max_original_bytes=75;
 images.max_pixels=63;failure(XC_SEARCH_EXPORT_OUTPUT_LIMIT);images.max_pixels=64;
 images.max_images=1;failure(XC_SEARCH_EXPORT_OUTPUT_LIMIT);images.max_images=2;
 images.max_original_bytes=150;images.max_pixels=128;rows[1]={0,missing};failure(XC_SEARCH_EXPORT_UNSUPPORTED_HTML);rows[1]={0,valid};images.max_original_bytes=75;images.max_pixels=64;
 ++input.generation;failure(XC_SEARCH_EXPORT_STALE);--input.generation;
 auto rejects=[&](const xc_search_html_image_options *value){request=reinterpret_cast<xc_request *>(1);check(xc_worker_export_search_html_with_images(worker,tab,&input,value,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid image input clears observer before submission");};
 rejects(nullptr);images.policy=2;rejects(&images);images.policy=1;images.max_images=0;rejects(&images);images.max_images=65;rejects(&images);images.max_images=2;
 images.max_original_bytes=0;rejects(&images);images.max_original_bytes=8*1024*1024+1;rejects(&images);images.max_original_bytes=75;
 images.max_pixels=0;rejects(&images);images.max_pixels=16*1024*1024+1;rejects(&images);images.max_pixels=64;
 check(xc_worker_export_search_html_with_images(nullptr,tab,&input,&images,&request)==XC_INVALID_ARGUMENT&&!request&&xc_worker_export_search_html_with_images(worker,tab,nullptr,&images,&request)==XC_INVALID_ARGUMENT&&!request&&xc_worker_export_search_html_with_images(worker,tab,&input,&images,nullptr)==XC_INVALID_ARGUMENT,"NULL image API arguments");
 stats={99,99,99,99};check(xc_search_html_export_get_images(nullptr,&stats)==XC_INVALID_ARGUMENT&&!stats.count&&!stats.unique_count&&!stats.original_bytes&&!stats.pixels&&xc_search_html_export_get_images(owned,nullptr)==XC_INVALID_ARGUMENT,"NULL image stats clear stale values");
 xc_search_html_image_options_default(nullptr);
 check(xc_worker_export_search_html_with_images(worker,999,&input,&images,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_search_html_export(request),"Closed/unknown tab rejects on owner");xc_request_release(request);
 check(xc_worker_export_search_html_with_images(worker,tab,&input,&images,&request)==XC_OK&&xc_request_cancel_search(request)==XC_OK,"Image document cancellation accepted");check(wait(request)==XC_OK,"Image cancellation outcome ready");xc_search_progress progress{};
 check(xc_request_search_progress(request,&progress)==XC_OK,"Image export progress available before observer release");auto *race=take();
 check(xc_search_html_export_get_info(race,&info)==XC_OK&&
  ((info.disposition==XC_SEARCH_EXPORT_OK&&progress.phase==XC_SEARCH_COMPLETE)||(info.disposition==XC_SEARCH_EXPORT_CANCELLED&&progress.phase==XC_SEARCH_CANCELLED)),"Cancellation/completion outcome agrees with owner control");
 if(info.disposition==XC_SEARCH_EXPORT_CANCELLED)check(xc_search_html_export_get_images(race,&stats)==XC_OK&&!stats.count&&!xc_search_html_export_size(race),"Cancelled image document is empty");xc_search_html_export_release(race);
 check(xc_worker_export_search_html_with_images(worker,tab,&input,&images,&request)==XC_OK,"Detached observer accepted");xc_request_release(request);
 check(xc_worker_export_search_html_with_images(worker,tab,&input,&images,&request)==XC_OK,"Image export accepted before shutdown");xc_worker_close(worker);auto *drained=take();
 check(xc_search_html_export_get_info(drained,&info)==XC_OK&&info.disposition==XC_SEARCH_EXPORT_OK&&xc_search_html_export_get_images(drained,&stats)==XC_OK&&stats.count==2&&std::string(xc_search_html_export_html(owned))==html,"Owned counts/document survive worker and request teardown");
 xc_search_html_export_release(drained);xc_search_html_export_release(owned);xc_search_html_export_release(nullptr);
 std::cout<<"ABI-safe C image policy, copied options, bounded owned document/counts, cancellation and lifetime passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
