// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include "xiphos/core/engine_manager.hpp"
#include "book_resource_source.hpp"
#include <glib.h>
#include <rawgenbook.h>
#include <rawld.h>
#include <markupfiltmgr.h>
#include <treekeyidx.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <iostream>
#include <stdexcept>
#include <cstring>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected image/context error");}
std::string shown;
static gint show_separate_image(const gchar *filename,gboolean){shown=filename;return 1;}
#define HAS_URL_PARAM(value) ((value) && *(value))
int actual_image(const char *svalue,bool clicked){const char *action="showImage";int retval=0;
#include "legacy_book_image_actual.inc"
return retval;}
std::string encode(const std::string &text){const char *hex="0123456789ABCDEF";std::string out;for(unsigned char c:text){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-')out+=static_cast<char>(c);else{out+='%';out+=hex[c>>4];out+=hex[c&15];}}return out;}
std::optional<std::string> config(sword::SWModule &module,const char *key){const auto *value=module.getConfigEntry(key);return value?std::make_optional(std::string(value)):std::nullopt;}
std::map<std::string,std::string> bytes(const fs::path &root){std::map<std::string,std::string> result;for(const auto &file:fs::recursive_directory_iterator(root))if(file.is_regular_file()){std::ifstream input(file.path(),std::ios::binary);result.emplace(fs::relative(file.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(input),{}));}return result;}
xc_status wait(xc_request *request){const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<deadline,"Image context deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
void done(xc_request *request,xc_status expected=XC_OK){check(wait(request)==expected,"Expected image context status");xc_request_release(request);}
int main(int argc,char **argv){try{
 check(argc==2,"Original image/context fixture required");const fs::path root(argv[1]);fs::create_directories(root/"mods.d");fs::create_directories(root/"data");
 for(const auto *id:{"PlainBook","StyledBook","EmptyCSS"}){
  const auto prefix=(root/"data"/id).string();check(sword::RawGenBook::createModule(prefix.c_str())==0,"Create original resource metadata book");
  {sword::RawGenBook writer(prefix.c_str());writer.setKey("/Entry");writer.setEntry("<p>Original image context prose <img src=\"café.png\" /></p>");}
  std::ofstream conf(root/"mods.d"/(std::string(id)+".conf"));conf<<'['<<id<<"]\nDataPath=./data/"<<id<<"\nModDrv=RawGenBook\nSourceType=ThML\nEncoding=UTF-8\n";
  if(std::string(id)=="StyledBook")conf<<"PreferredCSSXHTML=style café.css\n";
  if(std::string(id)=="EmptyCSS")conf<<"PreferredCSSXHTML=\n";
 }
 check(sword::RawLD::createModule((root/"data/dictionary").string().c_str())==0,"Create unsupported metadata driver");
 {std::ofstream conf(root/"mods.d/dictionary.conf");conf<<"[Dictionary]\nDataPath=./data/dictionary\nModDrv=RawLD\n";}
 {std::ofstream conf(root/"mods.d/verse-book.conf");conf<<"[VerseBook]\nDataPath=./data/PlainBook\nModDrv=RawGenBook\nKeyType=VerseKey\n";}
 fs::remove(root/"mods.d/modules-conf.cache");const auto before=bytes(root/"data");
 const std::string base="passagestudy.jsp?action=showImage&value=";
 for(const auto &value:std::vector<std::string>{"original.png","file:original.png","file:///original.png","file:file:original.png","café α +#&?!.png","/original/image.jpg","C:\\original\\image.jpeg","../original.gif","original.bmp","original.svg","original.tiff","original.webp",".png","", "file:","original.PNG","original.jpg?query","original.exe","original.tif","original.png/"}){
  for(bool clicked:{false,true}){
   shown.clear();const auto accepted=actual_image(value.c_str(),clicked);
   if(accepted){const auto parsed=book_image_link(base+encode(value));check(parsed.filename==shown && !parsed.declared_module,"Typed image filename matches actual legacy clicked/hover branch");}
   else rejects<std::invalid_argument>([&]{book_image_link(base+encode(value));});
  }
 }
 check(actual_image(nullptr,true)==0,"Missing legacy value is ignored");
 for(const auto &bad:{base+"original.png&action=showImage",base+"original%00.png",base+"%XX.png",std::string("https://example.invalid/?action=showImage&value=original.png"),std::string("passagestudy.jsp?action=showNote&type=n&value=original.png"),base+"original.png#fragment",base+"original.png&unknown=x"})rejects<std::invalid_argument>([&]{book_image_link(bad);});
 check(book_image_link(base+"original.png&module=").declared_module==std::make_optional(std::string()),"Explicit empty declared module stays distinct from missing");
 check(book_image_link("xiphos-preview://study/"+base+"file%3Aoriginal.png&module=OtherBook&type=ignored&passage=ignored").declared_module==std::make_optional(std::string("OtherBook")),"Declared module and ignored legacy fields do not rebase filename");
 BookResourceContext held;
 {
  Reader reader(root.string());EngineManager legacy(root.string().c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);
  for(const auto *id:{"PlainBook","StyledBook","EmptyCSS"}){
   auto &module=*legacy.getModule(id);auto &key=dynamic_cast<sword::TreeKeyIdx &>(*module.getKey());key.setOffset(4);module.renderText();module.setProcessEntryAttributes(false);
   const auto *identity=module.getKey();const auto attributes=module.getEntryAttributes();
   const auto actual=detail::capture_book_resource_context(module),captured=reader.book_resource_context(id);
   check(captured.module==id && captured.data_path==config(module,"DataPath") && captured.absolute_data_path==config(module,"AbsoluteDataPath") && captured.prefix_path==config(module,"PrefixPath") && captured.preferred_css_xhtml==config(module,"PreferredCSSXHTML"),"Context copies exact SDK virtual/config values, including missing versus empty");
   check(actual.data_path==captured.data_path && actual.absolute_data_path==captured.absolute_data_path && actual.prefix_path==captured.prefix_path && actual.preferred_css_xhtml==captured.preferred_css_xhtml,"SDK helper and serialized Reader copy the same context");
   check(module.getKey()==identity && key.getOffset()==4 && module.getEntryAttributes()==attributes && !module.isProcessEntryAttributes(),"Resource metadata leaves cursor, attributes and processing unchanged");
   if(std::string(id)=="StyledBook"){check(captured.preferred_css_xhtml==std::make_optional(std::string("style café.css")),"Unicode preferred CSS is raw copied metadata");held=captured;}
  }
  for(const auto *id:{"Dictionary","VerseBook"})rejects<std::invalid_argument>([&]{reader.book_resource_context(id);});
  rejects<std::out_of_range>([&]{reader.book_resource_context("Missing");});rejects<std::invalid_argument>([&]{reader.book_resource_context(std::string("PlainBook\0ignored",17));});
  StudySession a(reader),b(reader);rejects<std::invalid_argument>([&]{a.book_image_link(base+"original.png");});
  auto selected=a.select_book_document({"PlainBook",std::uint64_t(4)});check(selected.html.find("showImage")!=std::string::npos,"Actual SDK fixture emits a book image command");
  auto target=a.book_image_link(base+"file%3Acaf%C3%A9.png&module=OtherBook");check(target.context.module=="PlainBook" && target.command.filename=="café.png" && target.command.declared_module==std::make_optional(std::string("OtherBook")),"Document owns resource context; declared image module is metadata, as in legacy");
  check(a.book_selection()->document->html==selected.html && !b.book_selection() && a.history().entries().empty() && a.dictionary_history().entries().empty(),"Image candidate lookup is read-only and tab/history-neutral");
 }
 check(held.absolute_data_path && fs::is_directory(fs::u8path(*held.absolute_data_path)) && fs::u8path(*held.absolute_data_path).filename()=="data","Installed SDK supplies the resource directory even when DataPath is a RawGenBook file prefix");
 check(held.module=="StyledBook" && held.preferred_css_xhtml==std::make_optional(std::string("style café.css")) && held.absolute_data_path && held.prefix_path,"Copied context outlives SDK/Reader");
 std::future<BookImageTarget> drained;
 {
  StudyWorker worker(root.string());const auto a=worker.create_tab().get(),b=worker.create_tab().get();
  auto selected=worker.select_book_document(a,{"StyledBook",std::uint64_t(4)});auto image=worker.book_image_link(a,base+"file%3Acaf%C3%A9.png&module=OtherBook");auto snapshot=worker.book_document_selection(a);
  check(selected.get().tree.module=="StyledBook" && image.get().context.preferred_css_xhtml==held.preferred_css_xhtml && snapshot.get()->tree.node.offset==4,"FIFO source publication precedes copied image context");
  rejects<std::invalid_argument>([&]{(void)worker.book_image_link(b,base+"original.png").get();});
  worker.close_tab(b).get();rejects<std::invalid_argument>([&]{(void)worker.book_image_link(b,base+"original.png").get();});
  drained=worker.book_image_link(a,base+"original.png");
 }
 check(drained.get().context.preferred_css_xhtml==held.preferred_css_xhtml,"Worker close drains owned image metadata");
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.string().c_str(),&worker)==XC_OK,"Open C image context worker");
 check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Create C image tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_book_image_link(worker,tab,(base+"original.png").c_str(),&request)==XC_OK,"Queue image without document");done(request,XC_INVALID_ARGUMENT);
 check(xc_worker_select_book_document(worker,tab,"StyledBook",4,&request)==XC_OK,"Select C source document");done(request);
 std::string copied=base+"file%3Acaf%C3%A9.png&module=OtherBook";
 check(xc_worker_book_image_link(worker,tab,copied.c_str(),&request)==XC_OK && !xc_request_take_book_image(request),"Pending image result cannot be taken");copied.assign("unrelated");
 check(wait(request)==XC_OK && !xc_request_take_book_document(request) && xc_request_book_selection_present(request)==-1,"Image metadata is a distinct non-selection result");auto *image=xc_request_take_book_image(request);check(image && !xc_request_take_book_image(request),"Image target take-once");xc_request_release(request);
 check(std::string(xc_book_image_module(image))=="StyledBook" && std::string(xc_book_image_filename(image))=="café.png" && std::string(xc_book_image_declared_module(image))=="OtherBook","C source and declared module stay distinct with copied Unicode filename");
 check(std::string(xc_book_image_resource_field(image,XC_BOOK_RESOURCE_DATA_PATH))==*held.data_path && std::string(xc_book_image_resource_field(image,XC_BOOK_RESOURCE_ABSOLUTE_DATA_PATH))==*held.absolute_data_path && std::string(xc_book_image_resource_field(image,XC_BOOK_RESOURCE_PREFIX_PATH))==*held.prefix_path && std::string(xc_book_image_resource_field(image,XC_BOOK_RESOURCE_PREFERRED_CSS_XHTML))==*held.preferred_css_xhtml,"C owns exact copied SDK resource fields");
 check(!xc_book_image_resource_field(image,99) && !xc_book_image_module(nullptr) && !xc_book_image_filename(nullptr) && !xc_book_image_declared_module(nullptr) && !xc_book_image_resource_field(nullptr,0),"Invalid image getter contracts");xc_book_image_release(nullptr);
 request=reinterpret_cast<xc_request *>(1);check(xc_worker_book_image_link(worker,tab,nullptr,&request)==XC_INVALID_ARGUMENT && !request,"NULL URI resets output");
 check(xc_worker_book_image_link(worker,tab,(base+"original.PNG").c_str(),&request)==XC_OK,"Queue unsupported uppercase suffix");done(request,XC_INVALID_ARGUMENT);
 check(xc_worker_book_image_link(worker,tab,(base+"original.png").c_str(),&request)==XC_OK,"Queue image before worker close");xc_worker_close(worker);
 check(wait(request)==XC_OK,"Close drains accepted image request");auto *last=xc_request_take_book_image(request);xc_request_release(request);
 check(last && !xc_book_image_declared_module(last) && std::string(xc_book_image_filename(last))=="original.png" && std::string(xc_book_image_resource_field(image,XC_BOOK_RESOURCE_PREFERRED_CSS_XHTML))=="style café.css","Owned resource/image copies outlive worker and later commands");xc_book_image_release(last);xc_book_image_release(image);
 check(bytes(root/"data")==before,"Image command/context queries leave all module data bytes unchanged");
 std::cout<<"Legacy image-command filenames and exact copied SDK resource context passed; no resource I/O or viewer launched\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
