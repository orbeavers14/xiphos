// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/image_size.h"
#include <glib.h>
#include <cstring>
#include <string>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <map>
#include "legacy_image_size_frozen.inc"
extern "C" int image_size_c_header_test(void);
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
struct GdkWindow { int width,height; };
int gdk_window_get_width(GdkWindow *w){return w->width;}
int gdk_window_get_height(GdkWindow *w){return w->height;}
std::map<std::string,xc_image_size> images;
int ImageDimensions(const char *path,int *x,int *y){auto f=images.find(path);if(f==images.end())return -1;*x=f->second.width;*y=f->second.height;return 0;}
int warnings=0;
void gui_generic_warning(const char *){++warnings;}
#define USE_GTK_3
#define HAVE_STRCASESTR
#define _(text) text
#include "legacy_image_size_actual.inc"
int main(){try{
 check(!image_size_c_header_test(),"Standalone strict C geometry transport");
 const int sizes[]={1,23,24,100,199,200,201,300,800,1024,1920,4096,10000};
 for(int w:sizes)for(int h:sizes)for(int columns:{1,2,3,8}){
  const auto expected=legacy_bounds(w,h,columns);xc_image_size actual{9,9};
  const auto status=xc_image_viewport_size(w,h,columns,&actual);
  if(expected.width<=0 || expected.height<=0){check(status==1&&!actual.width&&!actual.height,"Unusable viewport clears output");continue;}
  check(!status&&actual.width==expected.width&&actual.height==expected.height,"Viewport preserves legacy single/multi-column margins and threshold");
  for(int iw:sizes)for(int ih:sizes){
   const auto fit=legacy_fit(iw,ih,actual.width,actual.height);xc_image_size out{};
   check(!xc_fit_image_size(iw,ih,actual.width,actual.height,&out)&&out.width==fit.width&&out.height==fit.height,"Shared width-then-height float/truncation equals frozen legacy arithmetic");
  }
 }
 for(int bad:{0,-1,std::numeric_limits<int>::min()}){xc_image_size out{9,9};check(xc_image_viewport_size(100,100,bad,&out)&&!out.width&&!out.height,"Invalid columns reject");out={9,9};check(xc_fit_image_size(bad,100,100,100,&out)&&!out.width&&!out.height,"Invalid image dimensions reject");}
 {xc_image_size out{9,9};check(xc_fit_image_size(std::numeric_limits<int>::max(),std::numeric_limits<int>::max(),std::numeric_limits<int>::max()-1,std::numeric_limits<int>::max(),&out)&&!out.width&&!out.height,"Float rounding outside int representation rejects instead of undefined conversion");}
 check(xc_image_viewport_size(100,100,1,nullptr)&&xc_fit_image_size(1,1,1,1,nullptr),"NULL output rejects without a crash");
 GdkWindow window{1000,800};images["/original.png"]={1600,1200};images["/small.png"]={8,8};
 std::string input="Original <img src=\"file:/original.png\" alt=\"Original\"> then <img src=\"file:///small.png\"> tail";
 const auto original=input;const std::string output=AnalyzeForImageSize(&input[0],2,&window);
 check(input==original&&output.find("width=\"402\" height=\"301\"")!=std::string::npos&&output.find("width=\"8\" height=\"8\"")!=std::string::npos&&output.find("tail")!=std::string::npos&&!warnings,"Actual legacy adapter uses shared geometry, keeps small image, URI spelling, input bytes and tail");
 images["/huge.png"]={1000000000,1000000000};GdkWindow huge{std::numeric_limits<int>::max(),std::numeric_limits<int>::max()};
 input="<img src=\"file:/huge.png\">";const std::string huge_output=AnalyzeForImageSize(&input[0],1,&huge);
 check(huge_output.find("width=\"1000000000\" height=\"1000000000\"")!=std::string::npos,"Actual attribute formatting accommodates two ten-digit dimensions without overflow");
 input="Original no images";check(AnalyzeForImageSize(&input[0],1,&window)==&input[0],"Actual no-image fast path retains source pointer");
 input="<img src=\"missing.png\"><img src=\"missing2.png\">";(void)AnalyzeForImageSize(&input[0],1,&window);check(warnings==1,"Actual missing dimensions warn once");
 window={1,800};input="<img src=\"file:/original.png\">";check(AnalyzeForImageSize(&input[0],1,&window)==&input[0],"Actual unusable viewport returns unresized source");
 std::cout<<"Shared legacy image geometry, frozen arithmetic and actual GTK adapter passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
