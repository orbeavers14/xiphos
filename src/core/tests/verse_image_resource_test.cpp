// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "image_source_test_support.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <iterator>
#include <stdexcept>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class E,class F>void rejects(F call){try{call();}catch(const E &){return;}throw std::runtime_error("Expected verse image refusal");}
std::map<std::string,std::string> bytes(const fs::path &root){
 std::map<std::string,std::string> out;for(const auto &file:fs::recursive_directory_iterator(root))if(file.is_regular_file()&&!file.is_symlink()){
  std::ifstream in(file.path(),std::ios::binary);out.emplace(fs::relative(file.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(in),{}));
 }return out;
}
std::string escaped_path(const std::string &path){const char *hex="0123456789ABCDEF";std::string out;
 for(unsigned char c:path){if(c=='%'||c==' '||c>=128){out+='%';out+=hex[c>>4];out+=hex[c&15];}else out+=char(c);}return out;
}
int main(int argc,char **argv){try{
 check(argc==2,"Original verse image fixture required");const auto root=fs::path(argv[1])/fs::u8path("library %20 café");
 const std::vector<std::string> ids{"ThMLText","ThMLNotes","OSISText","OSISNotes"};
 const auto before=bytes(root);VerseImageResource held;std::string worker_source,worker_link;
 {
  Reader reader(root.string());StudySession session(reader);session.enable_module_preferences();
  for(const auto &id:ids){
   session.visit(id,"John1:2");const auto history=session.history().entries().size();const auto prefs=session.module_presentation_options(id);
   for(unsigned verse=1;verse<=7;++verse){
    const auto source=image_test::images(session.capture_entry_html(id,"John1:"+std::to_string(verse)).html).at(0);
    const auto link=image_test::attributes(session.capture_entry_html(id,"John1:"+std::to_string(verse)).html,"a","href").at(0);
    if(verse>4)rejects<std::invalid_argument>([&]{reader.read_verse_image_link(id,link);});
    if(verse>4){rejects<std::invalid_argument>([&]{reader.read_verse_inline_image(id,source);});continue;}
    auto result=reader.read_verse_inline_image(id,source,image_test::bytes().size());
    check(result.module==id&&result.source==source&&result.media_type=="image/png"&&result.data==image_test::bytes(),"Exact owned binary bytes from rendered source preserve Unicode/literal-percent native roots and URL filename decoding");
    rejects<std::invalid_argument>([&]{reader.read_verse_inline_image(id,source,image_test::bytes().size()-1);});
    auto clicked=reader.read_verse_image_link(id,link);
    check(clicked.module==id&&clicked.source==link&&clicked.data==result.data,"SDK showImage query and inline URI lead to the same exact row-owned bytes");
    const auto tag=link.rfind("&module=");check(tag!=std::string::npos,"Fixture command carries declared source module");
    auto foreign=link.substr(0,tag)+"&module=DifferentInstalledOrMissing";
    check(reader.read_verse_image_link(id,foreign).data==result.data,"Declared command module metadata cannot redirect resource origin");
    held=std::move(result);if(id=="OSISText"&&verse==2){worker_source=source;worker_link=link;}
   }
   if(fs::is_symlink(root/"data"/id/"inside.png"))check(reader.read_verse_inline_image(id,"inside.png").data==image_test::bytes(),"Contained image symlink resolves to the exact module resource");
   if(fs::is_symlink(root/"data"/id/"outside.png"))rejects<std::invalid_argument>([&]{reader.read_verse_inline_image(id,"outside.png");});
   rejects<std::invalid_argument>([&]{reader.read_verse_inline_image(id,"directory.png");});
   const auto dir=root/"data"/id;
   // These explicit URI cases are independent of SDK image serialization.
   check(reader.read_verse_inline_image(id,"file:"+escaped_path((dir/"original.png").u8string())).data==image_test::bytes(),"Encoded contained absolute file URI resolves exactly once");
   check(reader.read_verse_inline_image(id,"empty.png").data.empty(),"Empty regular file is an owned empty byte result");
   for(const auto &bad:std::vector<std::string>{"https://example.invalid/original.png","file://host/original.png","bad%00.png",std::string("bad\0tail.png",12),std::string("\xff.png",5),std::string(32769,'x')})
    rejects<std::invalid_argument>([&]{reader.read_verse_inline_image(id,bad);});
   // A relative sibling may be identical for one origin, so test cross-module
   // containment with an absolute path to a different actual module.
   const auto other=id=="ThMLText"?"OSISText":"ThMLText";
   rejects<std::invalid_argument>([&]{reader.read_verse_inline_image(id,"file:"+escaped_path((root/"data"/other/"original.png").u8string()));});
   check(session.history().entries().size()==history&&!session.book_selection()&&session.module_presentation_options(id).image_content==prefs.image_content,"Resource success/failure does not publish navigation or presentation state");
  }
  rejects<std::out_of_range>([&]{reader.read_verse_inline_image("Missing","original.png");});
  for(auto limit:{std::size_t{0},maximum_verse_image_bytes+1})rejects<std::invalid_argument>([&]{reader.read_verse_inline_image("ThMLText","original.png",limit);});
 }
 std::future<VerseImageResource> drained,clicked;
 {StudyWorker worker(root.string());auto submitted=worker_source;drained=worker.read_verse_inline_image("OSISText",submitted);clicked=worker.read_verse_image_link("OSISText",worker_link);submitted="unrelated.png";}
 check(clicked.get().source==worker_link&&drained.get().data==image_test::bytes()&&held.data==image_test::bytes(),"Accepted copied image sources drain and results outlive SDK owner");
 check(bytes(root)==before,"Resource reads leave module/config/profile bytes unchanged");
 std::cout<<"Owned Bible/commentary image bytes, source context, path bounds and FIFO lifetime passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
