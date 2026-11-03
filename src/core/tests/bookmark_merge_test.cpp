// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/andbible.hpp"
#include "xiphos/core/c_api.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>
using namespace xiphos::core;namespace fs=std::filesystem;
static void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
static void reject(const std::function<void()> &f){try{f();}catch(const std::exception &){return;}throw std::runtime_error("Expected merge rejection");}
static std::string bytes(const fs::path &p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
static void write(const fs::path &p,const std::string &s){std::ofstream out(p,std::ios::binary);out<<s;check(bool(out),"Fixture write");}
static xc_status wait(xc_request *request){auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);xc_status status;while((status=xc_request_poll(request))==XC_PENDING){check(std::chrono::steady_clock::now()<deadline,"C merge deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return status;}
int main(int argc,char **argv)try{
 check(argc==2,"Named disposable directory required");const fs::path root=argv[1];check(root.filename()=="bookmark-merge-fixture","Named scratch only");fs::remove_all(root);fs::create_directories(root/"modules/mods.d");
 const auto file=root/"bookmarks.xml";
 const std::string xml="<SwordBookmarks syntaxVersion='1.0' andbible-source='old'><Extension version='1'/><!--retained--><Folder caption='First' color='#ff0000'><Bookmark key='John3:16' description='Original Ω note' andbible-entry='entry:label' andbible-note='Original Ω note' future='keep'><Future value='original'/></Bookmark></Folder></SwordBookmarks>";
 auto first=merge_bookmark_document(file.string(),std::nullopt,xml,"First import",BookmarkImportPolicy::reject_collisions);const auto original=bytes(file);
 auto reordered=xml;auto at=reordered.find("key='John3:16' description='Original Ω note'");reordered.replace(at,std::string("key='John3:16' description='Original Ω note'").size(),"description='Original Ω note' key='John3:16'");
 auto repeat=review_bookmark_import(file.string(),first.revision,reordered);check(repeat.identical==1&&repeat.collisions[0].kind==BookmarkImportCollisionKind::identical,"XML attribute ordering is not a conflict");
 check(merge_bookmark_document(file.string(),first.revision,reordered,"Do not duplicate",BookmarkImportPolicy::skip_identical).revision==first.revision&&bytes(file)==original,"Identical XML stays exact and idempotent");
 reject([&]{merge_bookmark_document(file.string(),first.revision,xml,"Default",BookmarkImportPolicy::reject_collisions);});
 auto changed=xml;at=changed.find("<Future value='original'/>");changed.replace(at,std::string("<Future value='original'/>").size(),"<Future value='changed'/>");check(review_bookmark_import(file.string(),first.revision,changed).conflicting==1,"Unknown leaf metadata changes require review");
 changed=xml;at=changed.find("<Extension version='1'/>");changed.replace(at,std::string("<Extension version='1'/>").size(),"<Extension version='2'/>");check(review_bookmark_import(file.string(),first.revision,changed).conflicting==1,"Unknown inherited metadata changes require review");
 auto mixed=xml;at=mixed.find("</Folder>");mixed.insert(at,"<Bookmark key='John3:17' description='Original new note' andbible-entry='second:label' future='new'/>");
 auto plan=review_bookmark_import(file.string(),first.revision,mixed);check(plan.new_items==1&&plan.identical==1,"Mixed new and repeated membership planning");
 auto saved=merge_bookmark_document(file.string(),first.revision,mixed,"Second import",BookmarkImportPolicy::skip_identical);const auto savedBytes=bytes(file);
 check(savedBytes.find("future=\"keep\"")!=savedBytes.npos&&savedBytes.find("future=\"new\"")!=savedBytes.npos&&savedBytes.find("<!--retained-->")!=savedBytes.npos,"Mixed merge preserves source/destination metadata");
 auto conflict=mixed;at=conflict.find("Original Ω note");conflict.replace(at,std::string("Original Ω note").size(),"Conflicting incoming note");
 plan=review_bookmark_import(file.string(),saved.revision,conflict);check(plan.conflicting>=1,"Edited imported note conflicts");
 reject([&]{merge_bookmark_document(file.string(),saved.revision,conflict,"Skip",BookmarkImportPolicy::skip_identical);});
 check(merge_bookmark_document(file.string(),saved.revision,conflict,"Keep",BookmarkImportPolicy::keep_existing).revision==saved.revision&&bytes(file)==savedBytes,"Keep-existing never overwrites conflicting descriptions or extension metadata");
 reject([&]{merge_bookmark_document(file.string(),first.revision,mixed,"Stale",BookmarkImportPolicy::keep_existing);});check(bytes(file)==savedBytes,"Stale merge preserves exact source bytes");
 auto duplicate=xml;at=duplicate.find("</Folder>");const auto leaf=duplicate.substr(duplicate.find("<Bookmark"),duplicate.find("</Bookmark>")+11-duplicate.find("<Bookmark"));duplicate.insert(at,leaf);reject([&]{review_bookmark_import(file.string(),saved.revision,duplicate);});
 const auto ambiguous=root/"ambiguous.xml";write(ambiguous,duplicate);auto amb=read_bookmarks(ambiguous.string());auto ambiguousPlan=review_bookmark_import(ambiguous.string(),amb.revision,xml);check(ambiguousPlan.ambiguous==1,"Ambiguous destination identities disclosed");reject([&]{merge_bookmark_document(ambiguous.string(),amb.revision,xml,"Keep",BookmarkImportPolicy::keep_existing);});check(bytes(ambiguous)==duplicate,"Ambiguous merge never changes source");
 auto utf16=root/"utf16.xml";std::string little="\xff\xfe";for(unsigned char c:std::string("<SwordBookmarks><!--UTF16--><Bookmark key='John3:18' future='yes'/></SwordBookmarks>")){little+=char(c);little+='\0';}write(utf16,little);auto old=read_bookmarks(utf16.string());auto xmlMerge=merge_bookmark_document(utf16.string(),old.revision,"<SwordBookmarks><Extension value='imported'/></SwordBookmarks>","Metadata",BookmarkImportPolicy::skip_identical);check(bytes(utf16).find("<!--UTF16-->")!=std::string::npos&&bytes(utf16).find("value=\"imported\"")!=std::string::npos,"UTF16 destination and metadata-only import survive verified conversion");
 xc_worker *worker=nullptr;check(xc_worker_open((root/"modules").string().c_str(),&worker)==XC_OK,"C merge worker");xc_request *request=nullptr;std::string incoming=mixed;
 check(xc_worker_review_bookmark_import(worker,file.string().c_str(),saved.revision.c_str(),incoming.c_str(),&request)==XC_OK,"Copied C review admission");incoming="changed";check(wait(request)==XC_OK,"C review execution");auto *owned=xc_request_take_bookmark_import_review(request);check(owned&&!xc_request_take_bookmark_import_review(request),"C review take once");xc_request_release(request);
 check(xc_worker_merge_bookmark_document(worker,file.string().c_str(),saved.revision.c_str(),mixed.c_str(),"C repeated",XC_BOOKMARK_IMPORT_SKIP_IDENTICAL,&request)==XC_OK&&wait(request)==XC_OK,"C idempotent merge");auto *snapshot=xc_request_take_bookmarks(request);xc_request_release(request);xc_bookmarks_release(snapshot);xc_worker_close(worker);
 xc_bookmark_import_counts counts{};check(xc_bookmark_import_review_counts(owned,&counts)==XC_OK&&counts.identical==2&&!counts.conflicting,"Owned C review survives input/request/worker teardown");xc_bookmark_import_collision collision{};check(xc_bookmark_import_review_collision(owned,0,&collision)==XC_OK&&collision.kind==1&&collision.identity&&xc_bookmark_import_review_source_revision(owned)&&std::string(xc_bookmark_import_review_destination_revision(owned))==saved.revision,"Owned collision rows and revision provenance");
 check(xc_bookmark_import_review_collision(owned,999,&collision)==XC_INVALID_ARGUMENT&&!collision.identity&&!collision.kind,"Invalid borrowed collision clears output");xc_bookmark_import_review_release(owned);check(bytes(file)==savedBytes,"C repeat writes no duplicate folder/content");
 std::cout<<"Bookmark merge policies, stable identity/context review, unknown/UTF16 metadata, stale guards and C ownership passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
