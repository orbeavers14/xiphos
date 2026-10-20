// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include "image_source_test_support.hpp"
#include <filesystem>
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
using namespace std::chrono_literals;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
xc_status wait(xc_request *r){const auto end=std::chrono::steady_clock::now()+10s;xc_status s;while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"C image request deadline");std::this_thread::sleep_for(1ms);}return s;}
std::string data(const xc_verse_image *image){return xc_verse_image_size(image)?std::string(reinterpret_cast<const char *>(xc_verse_image_bytes(image)),xc_verse_image_size(image)):std::string{};}
int main(int argc,char **argv){try{
 check(argc==2,"Original verse image fixture required");const auto root=std::filesystem::path(argv[1])/std::filesystem::u8path("library %20 café");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.u8string().c_str(),&worker)==XC_OK,"Open isolated C image worker");
 char module[]="ThMLText",source[]="caf%C3%A9.png";
 check(xc_worker_read_verse_inline_image(worker,module,source,image_test::bytes().size(),&request)==XC_OK&&!xc_request_take_verse_image(request),"Accepted source copies before typed result is available");
 module[0]='X';source[0]='X';
 check(wait(request)==XC_OK&&!xc_request_take_book_image_data(request),"Image result cannot be taken as book data");auto *owned=xc_request_take_verse_image(request);
 check(owned&&!xc_request_take_verse_image(request),"C image ownership transfers once");xc_request_release(request);
 check(std::string(xc_verse_image_module(owned))=="ThMLText"&&std::string(xc_verse_image_source(owned))=="caf%C3%A9.png"&&std::string(xc_verse_image_media_type(owned))=="image/png"&&data(owned)==image_test::bytes(),"Copied original request and binary source/data match");
 check(!xc_verse_image_module(nullptr)&&!xc_verse_image_source(nullptr)&&!xc_verse_image_media_type(nullptr)&&!xc_verse_image_bytes(nullptr)&&!xc_verse_image_size(nullptr)&&!xc_request_take_verse_image(nullptr),"NULL C getter contracts");xc_verse_image_release(nullptr);
 for(auto cap:{std::size_t{0},std::size_t{16*1024*1024+1}}){request=reinterpret_cast<xc_request *>(1);check(xc_worker_read_verse_inline_image(worker,"ThMLText","original.png",cap,&request)==XC_INVALID_ARGUMENT&&!request,"Invalid source capacity clears observer");}
 request=reinterpret_cast<xc_request *>(1);check(xc_worker_read_verse_inline_image(worker,nullptr,"original.png",1024,&request)==XC_INVALID_ARGUMENT&&!request,"NULL module clears observer");
 request=reinterpret_cast<xc_request *>(1);check(xc_worker_read_verse_inline_image(worker,"ThMLText",nullptr,1024,&request)==XC_INVALID_ARGUMENT&&!request,"NULL source clears observer");
 request=reinterpret_cast<xc_request *>(1);check(xc_worker_read_verse_inline_image(nullptr,"ThMLText","original.png",1024,&request)==XC_INVALID_ARGUMENT&&!request,"NULL worker clears observer");
 check(xc_worker_read_verse_inline_image(worker,"ThMLText","original.png",1024,nullptr)==XC_INVALID_ARGUMENT,"NULL output is invalid");
 for(const auto &large:std::vector<std::string>{std::string(4097,'x'),std::string(32769,'x')}){
  request=reinterpret_cast<xc_request *>(1);
  const auto status=large.size()==4097?xc_worker_read_verse_inline_image(worker,large.c_str(),"original.png",1024,&request):xc_worker_read_verse_inline_image(worker,"ThMLText",large.c_str(),1024,&request);
  check(status==XC_INVALID_ARGUMENT&&!request,"Overlong input refuses before enqueue/copy");
 }
 const char malformed[]={char(0xff),'.','p','n','g',0};
 check(xc_worker_read_verse_inline_image(worker,"ThMLText",malformed,1024,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_verse_image(request),"Malformed source UTF-8 cannot become resource identity");xc_request_release(request);
 for(const auto &bad:{"missing.png","../OSISText/original.png","https://example.invalid/original.png","file://host/original.png","bad%00.png"}){
  check(xc_worker_read_verse_inline_image(worker,"ThMLText",bad,1024,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_verse_image(request),"Semantic path failure has no owned bytes");xc_request_release(request);
 }
 check(xc_worker_read_verse_inline_image(worker,"Missing","original.png",1024,&request)==XC_OK&&wait(request)==XC_MODULE_NOT_FOUND&&!xc_request_take_verse_image(request),"Missing module has typed failure");xc_request_release(request);
 check(xc_worker_read_verse_inline_image(worker,"ThMLText","original.png",image_test::bytes().size()-1,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_verse_image(request),"One-byte-short data cap refuses truncation");xc_request_release(request);
 check(xc_worker_read_verse_inline_image(worker,"ThMLText","empty.png",1024,&request)==XC_OK&&wait(request)==XC_OK,"Empty C image bytes succeed");auto *empty=xc_request_take_verse_image(request);xc_request_release(request);check(empty&&!xc_verse_image_size(empty)&&!xc_verse_image_bytes(empty),"Empty resource is distinct from missing result");xc_verse_image_release(empty);
 check(xc_worker_read_verse_inline_image(worker,"ThMLText","original.png",1024,&request)==XC_OK,"Detached read accepted");xc_request_release(request);
 check(xc_worker_read_verse_inline_image(worker,"ThMLText","a+b.png",1024,&request)==XC_OK,"Read accepted before shutdown");xc_worker_close(worker);
 check(wait(request)==XC_OK,"Shutdown drains source read");auto *last=xc_request_take_verse_image(request);xc_request_release(request);
 check(last&&data(last)==image_test::bytes()&&data(owned)==image_test::bytes(),"Independent source/data survive observer and worker lifetime");xc_verse_image_release(last);xc_verse_image_release(owned);
 std::cout<<"C11 copied verse image requests, bounded owned bytes, null/error/take-once and shutdown passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
