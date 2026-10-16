// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/profile.hpp"
#include <libxml/parser.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace xiphos::core;
#define _(value) (value)
static xmlDocPtr xml_settings_doc;
#include "legacy_xml_profile_actual.inc"
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::optional<std::string> original(const char *section,const char *key){auto *text=xml_get_value(section,key);if(!text)return std::nullopt;std::string result(text);xmlFree(text);return result;}
SavedBookSelection compare(const std::string &xml){
  xml_settings_doc=xmlReadMemory(xml.data(),static_cast<int>(xml.size()),nullptr,nullptr,XML_PARSE_NONET);check(xml_settings_doc,"Legacy XML fixture parses");
  const auto result=parse_saved_book_selection(xml);
  check(result.module==original("modules","book") && result.key==original("keys","book") && result.offset_text==original("keys","offset"),"Saved book values match actual legacy getter without normalization");
  xmlFreeDoc(xml_settings_doc);xml_settings_doc=nullptr;return result;
}
template<class F>void rejects(F call){try{call();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected profile rejection");}
int main(int argc,char **argv){
 try{
  check(argc==2,"Private generated profile root required");const std::filesystem::path root(argv[1]);std::filesystem::create_directories(root);
  for(const auto &source:{std::string("<Xiphos/>"),std::string("<Xiphos><modules><book/></modules><keys><book/><offset/></keys></Xiphos>"),
    std::string("<Xiphos><modules><book><![CDATA[]]></book></modules><keys><book>0</book><offset>0</offset></keys></Xiphos>"),
    std::string("<Xiphos><modules/><modules><book>First</book><book>Ignored</book></modules><modules><book>Also ignored</book></modules><keys><book>/Part One/café α</book><offset> 0012junk </offset></keys></Xiphos>"),
    std::string("<Xiphos><modules><book>Book &amp; More</book></modules><keys><book><nested>Ignored</nested>direct</book><offset>-999999999999999999999999999999999</offset></keys></Xiphos>"),
    std::string("<Xiphos><modules><book/></modules><modules><book>Shadowed</book></modules><keys><book>123 Numeric Title</book><offset>4</offset></keys><keys><book>Ignored</book></keys></Xiphos>")})compare(source);
  const auto absent=compare("<Xiphos/>");check(!absent.module && !absent.key && !absent.offset_text,"Missing values stay absent rather than applying startup defaults");
  const auto raw=compare("<Xiphos><modules><book>MissingModule</book></modules><keys><book>12junk</book><offset>not a number</offset></keys></Xiphos>");
  check(raw.module=="MissingModule" && raw.key=="12junk" && raw.offset_text=="not a number","No implicit installed-module validation, numeric parse, clamp or path resolution");
  std::string encoded="\xff\xfe";
  for(char16_t c:std::u16string(u"<?xml version='1.0' encoding='UTF-16'?><Xiphos><modules><book>OriginalBook</book></modules><keys><book>/café α</book><offset>24</offset></keys></Xiphos>")){encoded.push_back(static_cast<char>(c&255));encoded.push_back(static_cast<char>(c>>8));}
  const auto expected=compare(encoded);check(expected.key=="/café α","Declared Unicode encoding becomes owned UTF-8");
  const auto file=root/std::filesystem::u8path("book-café.xml");{std::ofstream out(file,std::ios::binary);out.write(encoded.data(),static_cast<std::streamsize>(encoded.size()));}
  const auto permissions=std::filesystem::status(file).permissions();
  const auto loaded=read_saved_book_selection(file.u8string());check(loaded.module==expected.module && loaded.key==expected.key && loaded.offset_text==expected.offset_text,"Read explicit selected profile");
  {std::ifstream input(file,std::ios::binary);check(std::string(std::istreambuf_iterator<char>(input),{})==encoded && std::filesystem::status(file).permissions()==permissions,"Read preserves complete profile bytes and permissions");}
  std::filesystem::remove(file);encoded.clear();check(loaded.key=="/café α" && loaded.offset_text=="24","Imported values outlive source bytes and file");
  for(const auto &path:{std::string(),root.u8string(),file.u8string(),std::string("bad\0path",8)})rejects([&]{read_saved_book_selection(path);});
  for(const auto &bad:{std::string(),std::string("<Other/>"),std::string("<Xiphos>"),std::string("<!DOCTYPE Xiphos SYSTEM 'https://example.test/profile'><Xiphos/>"),std::string("<Xiphos/>\0hidden",16),std::string(16*1024*1024+1,'x')})rejects([&]{parse_saved_book_selection(bad);});
  std::future<SavedBookSelection> jobs[4];
  for(unsigned i=0;i<4;++i)jobs[i]=std::async(std::launch::async,[i]{return parse_saved_book_selection("<Xiphos><keys><book>"+std::to_string(i)+"</book></keys></Xiphos>");});
  for(unsigned i=0;i<4;++i)check(jobs[i].get().key==std::to_string(i),"Independent parser contexts own separate results");
  std::cout<<"Legacy saved book-selection values, encoding, read-only ownership and XML guards passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
