// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <chrono>
#include <thread>
#include <map>
#include <stdexcept>
#include <iostream>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected bounded resource error");}
std::string encode(const std::string &text){const char *hex="0123456789ABCDEF";std::string out;for(unsigned char c:text){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-')out+=static_cast<char>(c);else{out+='%';out+=hex[c>>4];out+=hex[c&15];}}return out;}
void write(const fs::path &path,const std::string &data){std::ofstream file(path,std::ios::binary);file.write(data.data(),static_cast<std::streamsize>(data.size()));check(bool(file),"Write original resource fixture");}
std::map<std::string,std::string> bytes(const fs::path &root){std::map<std::string,std::string> result;for(const auto &file:fs::recursive_directory_iterator(root))if(file.is_regular_file()&&!file.is_symlink()){std::ifstream in(file.path(),std::ios::binary);result.emplace(fs::relative(file.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(in),{}));}return result;}
xc_status wait(xc_request *request){const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<deadline,"Image read deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Original image fixture required");const fs::path root(argv[1]);const std::string base="passagestudy.jsp?action=showImage&value=";
 const auto file=root/"data"/fs::u8path("café.svg"),binary=root/"data/binary.png",outside=root/"outside.svg",link=root/"data/escape.svg",inside_link=root/"data/inside.svg";
 const std::string svg="<svg xmlns='http://www.w3.org/2000/svg' width='8' height='8'><title>Original resource fixture</title><rect width='8' height='8' fill='#2468ac'/></svg>";
 const std::string binary_bytes("Original\0binary\xff",16);write(file,svg);write(binary,binary_bytes);write(root/"data/empty.png","");write(outside,"Original outside resource");
 fs::create_directories(root/"data/sub");fs::create_directories(root/"data/folder.svg");
 std::error_code error;fs::remove(link,error);fs::create_symlink(outside,link,error);const bool outside_symlink=!error;
 fs::remove(inside_link,error);fs::create_symlink(file,inside_link,error);const bool inside_symlink=!error;
 const std::string original_png("\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52\x00\x00\x00\x08\x00\x00\x00\x08\x08\x06\x00\x00\x00\xc4\x0f\xbe\x8b\x00\x00\x00\x12\x49\x44\x41\x54\x78\x9c\x63\x50\xc9\x58\xf3\x1f\x1f\x66\x18\x19\x0a\x00\xc1\xa0\x8d\xc1\xf4\xd5\x38\xd1\x00\x00\x00\x00\x49\x45\x4e\x44\xae\x42\x60\x82",75);write(root/"data"/fs::u8path("café.png"),original_png);
 const auto before=bytes(root);BookImageResource held;
 {
  Reader reader(root.string());
  for(const auto &value:std::vector<std::string>{"café.svg","file:café.svg",file.u8string(),"file:"+file.u8string(),"file://"+file.u8string(),"sub/../café.svg"}){
   auto resource=reader.read_book_image("StyledBook",base+encode(value));check(resource.data==svg && resource.target.context.module=="StyledBook" && resource.target.command.media_type=="image/svg+xml","Relative/contained absolute/file URI paths use SDK resource directory with exact owned bytes");held=std::move(resource);
  }
  check(reader.read_book_image("StyledBook",base+"empty.png").data.empty(),"Empty files remain empty byte resources, not validated image content");
  check(reader.read_book_image("StyledBook",base+"binary.png").data==binary_bytes,"Binary and NUL bytes remain intact; extension MIME does not claim decoding");
  check(reader.read_book_image("StyledBook",base+encode("café.svg"),svg.size()).data==svg,"Exact byte limit succeeds");
  rejects<std::invalid_argument>([&]{reader.read_book_image("StyledBook",base+encode("café.svg"),svg.size()-1);});
  rejects<std::invalid_argument>([&]{reader.read_book_image("StyledBook",base+encode("café.svg"),0);});
  rejects<std::invalid_argument>([&]{reader.read_book_image("StyledBook",base+encode("café.svg"),maximum_book_image_bytes+1);});
  for(const auto &value:std::vector<std::string>{"../outside.svg",outside.u8string(),"missing.svg","folder.svg","file://host/share/image.svg","\\\\host\\share\\image.svg","café.SVG"})rejects<std::invalid_argument>([&]{reader.read_book_image("StyledBook",base+encode(value));});
  if(outside_symlink)rejects<std::invalid_argument>([&]{reader.read_book_image("StyledBook",base+"escape.svg");});
  if(inside_symlink)check(reader.read_book_image("StyledBook",base+"inside.svg").data==svg,"Contained symlink resolves inside declared resource directory");
  for(const auto *id:{"Dictionary","VerseBook"})rejects<std::invalid_argument>([&]{reader.read_book_image(id,base+"binary.png");});
  rejects<std::out_of_range>([&]{reader.read_book_image("Missing",base+"binary.png");});
  StudySession a(reader),b(reader);rejects<std::invalid_argument>([&]{a.read_book_image_link(base+"binary.png");});auto selected=a.select_book_document({"StyledBook",std::uint64_t(4)});
  auto resource=a.read_book_image_link(base+encode("café.svg")+"&module=OtherBook");check(resource.data==svg && resource.target.context.module=="StyledBook" && a.book_selection()->document->html==selected.html && !b.book_selection(),"Image read uses committed source without publishing or following declared module");
  check(a.history().entries().empty()&&a.dictionary_history().entries().empty(),"Image reads do not create history visits");
 }
 std::future<BookImageResource> drained;
 {StudyWorker worker(root.string());const auto tab=worker.create_tab().get();auto selected=worker.select_book_document(tab,{"StyledBook",std::uint64_t(4)});drained=worker.read_book_image_link(tab,base+encode("café.svg"));check(selected.get().tree.module=="StyledBook","FIFO source selection precedes image read");}
 check(drained.get().data==svg && held.data==svg,"Owned image bytes survive Reader/worker shutdown");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.string().c_str(),&worker)==XC_OK,"Open C resource worker");
 check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create C image tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_select_book_document(worker,tab,"StyledBook",4,&request)==XC_OK&&wait(request)==XC_OK,"Select C image source");xc_request_release(request);
 std::string copied=base+encode("café.svg");check(xc_worker_read_book_image_link(worker,tab,copied.c_str(),&request)==XC_OK&&!xc_request_take_book_image_data(request),"Pending data unavailable");copied.assign("unrelated");
 check(wait(request)==XC_OK&&!xc_request_take_book_image(request),"Image data is a separate owned result kind");auto *data=xc_request_take_book_image_data(request);check(data&&!xc_request_take_book_image_data(request),"Image data take-once");xc_request_release(request);
 check(xc_book_image_data_size(data)==svg.size()&&std::string(reinterpret_cast<const char *>(xc_book_image_data_bytes(data)),xc_book_image_data_size(data))==svg&&std::string(xc_book_image_media_type(xc_book_image_data_source(data)))=="image/svg+xml","C borrowed data/source copies match original bytes and MIME hint");
 request=reinterpret_cast<xc_request *>(1);check(xc_worker_read_book_image_link(worker,tab,nullptr,&request)==XC_INVALID_ARGUMENT&&!request,"NULL resource URI clears output");
 check(!xc_book_image_data_source(nullptr)&&!xc_book_image_data_bytes(nullptr)&&!xc_book_image_data_size(nullptr)&&!xc_book_image_media_type(nullptr),"NULL data getter contracts");xc_book_image_data_release(nullptr);
 check(xc_worker_read_book_image_link(worker,tab,(base+"..%2Foutside.svg").c_str(),&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_book_image_data(request),"Outside path rejects without partial result");xc_request_release(request);
 check(xc_worker_follow_book_link(worker,tab,(base+"caf%C3%A9.png").c_str(),&request)==XC_OK&&wait(request)==XC_OK&&!xc_request_take_book_note(request),"Core click dispatch selects image resource result");auto *followed=xc_request_take_book_image_data(request);xc_request_release(request);check(followed&&std::string(reinterpret_cast<const char *>(xc_book_image_data_bytes(followed)),xc_book_image_data_size(followed))==original_png,"Dispatched original PNG bytes match");xc_book_image_data_release(followed);
 check(xc_worker_read_book_image_link(worker,tab,(base+"empty.png").c_str(),&request)==XC_OK&&wait(request)==XC_OK,"Read empty C resource");auto *empty=xc_request_take_book_image_data(request);xc_request_release(request);check(empty&&!xc_book_image_data_size(empty)&&!xc_book_image_data_bytes(empty),"Zero-size bytes use NULL without implying missing owned result");xc_book_image_data_release(empty);
 check(xc_worker_read_book_image_link(worker,tab,(base+"binary.png").c_str(),&request)==XC_OK,"Queue binary image before close");xc_worker_close(worker);check(wait(request)==XC_OK,"Close drains bounded read");auto *last=xc_request_take_book_image_data(request);xc_request_release(request);
 check(last&&std::string(reinterpret_cast<const char *>(xc_book_image_data_bytes(last)),xc_book_image_data_size(last))==binary_bytes&&std::string(reinterpret_cast<const char *>(xc_book_image_data_bytes(data)),xc_book_image_data_size(data))==svg,"Binary/source copies outlive later reads and shutdown");xc_book_image_data_release(data);xc_book_image_data_release(last);
 check(bytes(root)==before,"Resource reads do not modify data/config/profile bytes");
 std::cout<<"Bounded module-directory image bytes, path constraints, state neutrality and owned C transport passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
