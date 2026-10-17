// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/engine_manager.hpp"
#include <rawgenbook.h>
#include <markupfiltmgr.h>
#include <glib.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
#include <chrono>
#include <stdexcept>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected stylesheet error");}
void write(const fs::path &path,const std::string &bytes){std::ofstream out(path,std::ios::binary);out.write(bytes.data(),bytes.size());check(bool(out),"Write original stylesheet fixture");}
sword::SWMgr *legacy_manager=nullptr;
char *main_get_mod_config_entry(const char *id,const char *key){return const_cast<char *>(legacy_manager->getModule(id)->getConfigEntry(key));}
struct { const char *gSwordDir; } settings;
using sword::SWModule;
using std::string;
using gchar=char;
#include "legacy_book_css_actual.inc"
xc_status wait(xc_request *request){const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<deadline,"Stylesheet worker deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv){try{
 check(argc==2,"Original stylesheet fixture required");const fs::path root(argv[1]);fs::create_directories(root/"mods.d");fs::create_directories(root/"data");fs::create_directories(root/"profile");
 const std::map<std::string,std::optional<std::string>> choices={{"Default",{}},{"Preferred","style café.css"},{"Empty",""},{"Escape","../outside.css"},{"Percent","literal%20+style.css"},{"Absolute","/nested/style.css"},{"InsideLink","inside.css"},{"OutsideLink","outside-link.css"}};
 for(const auto &choice:choices){const auto prefix=(root/"data"/choice.first).string();check(sword::RawGenBook::createModule(prefix.c_str())==0,"Create original CSS book");{sword::RawGenBook writer(prefix.c_str());writer.setKey("/Entry");writer.setEntry("<p class='original'>Original stylesheet prose</p>");}std::ofstream conf(root/"mods.d"/(choice.first+".conf"));conf<<'['<<choice.first<<"]\nDataPath=./data/"<<choice.first<<"\nModDrv=RawGenBook\nSourceType=ThML\nEncoding=UTF-8\n";if(choice.second)conf<<"PreferredCSSXHTML="<<*choice.second<<'\n';}
 fs::remove(root/"mods.d/modules-conf.cache");fs::remove(root/"data/style.css");fs::remove(root/"profile/default-style.css");
 const std::string preferred="/* Original café style </style> */\n.original { color: rgb(36, 104, 172); }\n.original::before { content: '<Original>'; }\n";
 write(root/"data"/fs::u8path("style café.css"),preferred);write(root/"data/literal%20+style.css","/* Original literal path */");write(root/"outside.css","/* Original outside */");
 fs::create_directories(root/"data/nested");write(root/"data/nested/style.css","/* Original nested */");
 std::error_code error;fs::remove(root/"data/inside.css",error);fs::create_symlink(root/"data"/fs::u8path("style café.css"),root/"data/inside.css",error);const bool inside_link=!error;
 fs::remove(root/"data/outside-link.css",error);fs::create_symlink(root/"outside.css",root/"data/outside-link.css",error);const bool outside_link=!error;
 std::optional<BookStylesheetResource> held;
 {
  Reader reader(root.string());EngineManager legacy(root.string().c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);legacy_manager=&legacy;
  const auto profile=(root/"profile").string();settings.gSwordDir=profile.c_str();
  check(!reader.read_book_stylesheet("Default") && std::string(get_css_references(*legacy.getModule("Default"))).empty(),"Missing default stylesheet is ordinary absence in both routes");
  held=reader.read_book_stylesheet("Preferred");check(held&&held->css==preferred&&held->filename=="style café.css"&&held->context.module=="Preferred","Owned preferred Unicode stylesheet uses exact SDK context");
  check(held->inline_css.find("</style")==std::string::npos && held->inline_css.find("<\\/style>")!=std::string::npos && held->inline_css.find("<Original>")!=std::string::npos,"Core escapes HTML style termination while preserving CSS character semantics");
  StudySession session(reader);const auto selected=session.select_book_document({"Preferred",std::uint64_t(4)});
  check(selected.presentation && selected.presentation->stylesheet && selected.presentation->stylesheet->css==preferred && !selected.presentation->stylesheet_issue && selected.html.find("color:")==std::string::npos,"Selection atomically owns CSS separately from raw document HTML");
  const auto empty=session.select_book_document({"Empty",std::uint64_t(4)});check(empty.presentation->stylesheet_issue && !empty.presentation->stylesheet && !empty.html.empty() && session.history().entries().empty(),"Unusable stylesheet reports issue while retaining text/history neutrality");
  const auto actual=std::string(get_css_references(*legacy.getModule("Preferred")));
  check(actual.find((root/"data"/fs::u8path("style café.css")).u8string())!=std::string::npos,"Actual legacy CSS branch selects matching preferred file");
  write(root/"data/style.css","/* Original fallback */");check(reader.read_book_stylesheet("Default")->css=="/* Original fallback */","Missing preference selects style.css, not preferred sibling");
  write(root/"data/style.css","");check(reader.read_book_stylesheet("Default")->css.empty(),"Empty file is a present owned stylesheet");
  check(reader.read_book_stylesheet("Percent")->css=="/* Original literal path */","Config filenames do not decode percent or plus as URLs");
  auto absolute=reader.read_book_stylesheet("Absolute");check(absolute&&absolute->css=="/* Original nested */"&&std::string(get_css_references(*legacy.getModule("Absolute"))).find((root/"data/nested/style.css").string())!=std::string::npos,"GLib absolute-looking later components use legacy joined directory semantics");
  rejects<std::invalid_argument>([&]{reader.read_book_stylesheet("Empty");});rejects<std::invalid_argument>([&]{reader.read_book_stylesheet("Escape");});
  if(inside_link)check(reader.read_book_stylesheet("InsideLink")->css==preferred,"Contained CSS symlink resolves inside data directory");if(outside_link)rejects<std::invalid_argument>([&]{reader.read_book_stylesheet("OutsideLink");});
  check(reader.read_book_stylesheet("Preferred",preferred.size())->css==preferred,"Exact CSS byte bound succeeds");rejects<std::invalid_argument>([&]{reader.read_book_stylesheet("Preferred",preferred.size()-1);});rejects<std::invalid_argument>([&]{reader.read_book_stylesheet("Preferred",0);});rejects<std::invalid_argument>([&]{reader.read_book_stylesheet("Preferred",maximum_book_stylesheet_bytes+1);});
  write(root/"data"/fs::u8path("style café.css"),std::string("bad\0css",7));rejects<std::invalid_argument>([&]{reader.read_book_stylesheet("Preferred");});write(root/"data"/fs::u8path("style café.css"),std::string("\xff",1));rejects<std::invalid_argument>([&]{reader.read_book_stylesheet("Preferred");});write(root/"data"/fs::u8path("style café.css"),preferred);
  const std::string unusual="<!-- .original::before { content: '\\<Original> </StYlE>'; } -->";
  write(root/"data"/fs::u8path("style café.css"),unusual);
  const auto escaped=reader.read_book_stylesheet("Preferred");
  check(escaped->inline_css=="<!-- .original::before { content: '\\<Original> <\\/StYlE>'; } -->","Style embedding preserves CDO, existing CSS escapes and case while escaping only the HTML end-tag opener");
  write(root/"data"/fs::u8path("style café.css"),preferred);
  write(root/"profile/default-style.css","/* Original global CSS is not implicitly imported */");check(reader.read_book_stylesheet("Preferred")->css==preferred,"Global profile CSS is outside this explicit module resource operation");
  rejects<std::out_of_range>([&]{reader.read_book_stylesheet("Missing");});
 }
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.string().c_str(),&worker)==XC_OK,"Open CSS worker");
 check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"Create CSS tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 check(xc_worker_select_book_document(worker,tab,"Preferred",4,&request)==XC_OK&&wait(request)==XC_OK,"Select queued owned CSS");auto *document=xc_request_take_book_document(request);xc_request_release(request);
 check(document && std::string(xc_book_document_stylesheet_css(document))==preferred && std::string(xc_book_document_stylesheet_filename(document))=="style café.css" && std::string(xc_book_document_stylesheet_inline_css(document)).find("</style")==std::string::npos && !xc_book_document_stylesheet_issue(document),"C document borrows independent CSS, filename and safe style text");
 check(!xc_book_document_stylesheet_css(nullptr)&&!xc_book_document_stylesheet_filename(nullptr)&&!xc_book_document_stylesheet_inline_css(nullptr)&&!xc_book_document_stylesheet_issue(nullptr),"NULL stylesheet getters");
 check(xc_worker_select_book_document(worker,tab,"Default",4,&request)==XC_OK&&wait(request)==XC_OK,"Select empty stylesheet file");auto *empty=xc_request_take_book_document(request);xc_request_release(request);check(xc_book_document_stylesheet_css(empty)&&!*xc_book_document_stylesheet_css(empty),"C empty stylesheet is present, distinct from missing");xc_book_document_release(empty);
 check(xc_worker_select_book_document(worker,tab,"Empty",4,&request)==XC_OK&&wait(request)==XC_OK,"Select unusable stylesheet source");auto *issue=xc_request_take_book_document(request);xc_request_release(request);check(!xc_book_document_stylesheet_css(issue)&&xc_book_document_stylesheet_issue(issue)&&*xc_book_document_html(issue),"C stylesheet issue does not lose reading text");xc_book_document_release(issue);
 check(xc_worker_select_book_document(worker,tab,"Preferred",4,&request)==XC_OK,"Queue CSS read before shutdown");xc_worker_close(worker);check(wait(request)==XC_OK,"Close drains CSS publication");auto *last=xc_request_take_book_document(request);xc_request_release(request);check(last&&std::string(xc_book_document_stylesheet_css(last))==preferred&&std::string(xc_book_document_stylesheet_css(document))==preferred,"CSS snapshots own bytes after later publications and worker shutdown");xc_book_document_release(last);xc_book_document_release(document);
 check(held&&held->css==preferred,"Owned CSS bytes survive SDK and Reader destruction");
 std::cout<<"Bounded module stylesheets, actual legacy selection, path/UTF-8 policy and ownership passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
