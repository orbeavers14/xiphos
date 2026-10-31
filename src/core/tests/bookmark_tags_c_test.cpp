// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
static thread_local bool watching=false;
static thread_local std::size_t allocations=0;
static thread_local int fail_after=-1;
void *operator new(std::size_t n){if(watching){++allocations;if(fail_after==0)throw std::bad_alloc();if(fail_after>0)--fail_after;}if(auto *p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void *operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void *p)noexcept{std::free(p);}void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p,std::size_t)noexcept{std::free(p);}void operator delete[](void *p,std::size_t)noexcept{std::free(p);}
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static xc_status wait(xc_request *r){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(30);xc_status s;while((s=xc_request_poll(r))==XC_PENDING){check(std::chrono::steady_clock::now()<end,"Tabs deadline");std::this_thread::sleep_for(std::chrono::milliseconds(1));}return s;}
template<class T>static void rejected(std::future<T> value,bool immediate=false){if(immediate)check(value.wait_for(std::chrono::seconds(0))==std::future_status::ready,"Admission rejection before FIFO");try{(void)value.get();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected tab profile rejection");}
int main(int argc,char **argv){try{
 check(argc==2,"Original installed fixture required");const auto root=std::filesystem::u8path(argv[1]);const auto file=root/"annotation-c.xml";
 const std::string source="<SwordBookmarks syntaxVersion='1.0'><Folder caption='Tags' color='#123456'><Bookmark modulename='Text' key='John3:16-18;16' description='Original &amp; 文字'/><Bookmark key='John3:16' description='Generic'/><Bookmark modulename='T' key='John3:16'/><Bookmark modulename='Missing' key='John3:16'/><Folder caption='Nested' color='#AA2233'><Bookmark modulename='Dictionary' key='1234'/></Folder></Folder><Unknown/></SwordBookmarks>";{std::ofstream out(file);out<<source;}
 xc_worker *worker=nullptr;xc_request *request=nullptr;check(xc_worker_open(root.u8string().c_str(),&worker)==XC_OK&&xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C annotation owner/tab");const auto tab=xc_request_tab(request);xc_request_release(request);
 xc_bookmark_tag_limits limits;xc_bookmark_tag_limits_default(&limits);xc_bookmark_tag_limits_default(nullptr);check(limits.max_verses==8192&&limits.max_occurrences==32768&&limits.max_snapshot_bytes==XC_BOOKMARK_TAG_MAX_SNAPSHOT_BYTES,"C default annotation bounds");
 const auto path=file.u8string();
 const std::string huge(32769,'x');allocations=0;watching=true;auto status=xc_worker_read_bookmark_tags(worker,tab,path.c_str(),"Text",huge.c_str(),&limits,&request);watching=false;check(status==XC_INVALID_ARGUMENT&&!request&&!allocations,"C oversized annotation context rejects without allocation");
 auto bad=limits;bad.max_occurrences=XC_BOOKMARK_TAG_MAX_OCCURRENCES+1;allocations=0;watching=true;status=xc_worker_read_bookmark_tags(worker,tab,path.c_str(),"Text","John3:16",&bad,&request);watching=false;check(status==XC_INVALID_ARGUMENT&&!request&&!allocations,"C occurrence cap rejects before observer/copy allocation");
 for(int failure:{0,1}){watching=true;fail_after=failure;status=xc_worker_read_bookmark_tags(worker,tab,path.c_str(),"Text","John3:16",&limits,&request);watching=false;fail_after=-1;check(status==XC_ENGINE_ERROR&&!request,"C annotation observer/envelope allocation failure safely rejects");}
 std::string copied_path=path,module="Text",context="John3:16";check(xc_worker_read_bookmark_tags(worker,tab,copied_path.c_str(),module.c_str(),context.c_str(),&limits,&request)==XC_OK,"C annotation read accepted");copied_path.clear();module.clear();context.clear();limits={};
 allocations=0;watching=true;status=wait(request);watching=false;check(status==XC_OK&&!allocations,"Owned annotation publication allocates nothing");auto *owned=xc_request_take_bookmark_tags(request);check(owned&&!xc_request_take_bookmark_tags(request)&&!xc_request_take_bookmarks(request),"Distinct take-once annotation handle");xc_request_release(request);
 check(std::string(xc_bookmark_tags_module(owned))=="Text"&&std::string(xc_bookmark_tags_context(owned))=="John3:16"&&std::string(xc_bookmark_tags_revision(owned)).size()==64,"Copied module/context and source revision");
 check(xc_bookmark_tags_candidate_count(owned)==5&&xc_bookmark_tags_verse_count(owned)==3&&xc_bookmark_tags_occurrences(owned)==5&&!xc_bookmark_tags_limited(owned)&&xc_bookmark_tags_skipped_modules(owned)==3&&xc_bookmark_tags_ignored_subtrees(owned)==1,"Owned annotation shape and eligibility/ignored diagnostics");
 check(xc_bookmark_tags_candidate_state(owned,0)==XC_BOOKMARK_TAG_READY&&xc_bookmark_tags_candidate_state(owned,1)==XC_BOOKMARK_TAG_GENERIC&&xc_bookmark_tags_candidate_state(owned,2)==XC_BOOKMARK_TAG_MISSING&&xc_bookmark_tags_candidate_state(owned,3)==XC_BOOKMARK_TAG_MISSING&&xc_bookmark_tags_candidate_state(owned,4)==XC_BOOKMARK_TAG_UNSUPPORTED,"Exact installed aliases/missing/dictionary/generic C states");
 check(xc_bookmark_tags_candidate_source(owned,0)==1&&xc_bookmark_tags_candidate_color_source(owned,0)==0&&std::string(xc_bookmark_tags_candidate_color(owned,0))=="#123456"&&std::string(xc_bookmark_tags_candidate_foreground(owned,0))=="#FFFFFF"&&std::string(xc_bookmark_tags_candidate_key(owned,0))=="John3:16-18;16"&&std::string(xc_bookmark_tags_candidate_module(owned,0))=="Text"&&std::string(xc_bookmark_tags_candidate_description(owned,0))=="Original & 文字","Copied candidate fields and source/color provenance");
 check(!xc_bookmark_tags_candidate_module(owned,1)&&!xc_bookmark_tags_candidate_description(owned,2)&&std::string(xc_bookmark_tags_verse_reference(owned,0))=="John.3.16"&&xc_bookmark_tags_verse_tag_count(owned,0)==3,"Absent metadata and ordered duplicate overlaps");
 size_t index=99;check(xc_bookmark_tags_verse_tag_index(owned,0,0,&index)==XC_OK&&index==0&&xc_bookmark_tags_verse_tag_index(owned,0,1,&index)==XC_OK&&index==0&&xc_bookmark_tags_verse_tag_index(owned,0,2,&index)==XC_OK&&index==1,"First color and all duplicate candidate occurrences retained");
 check(xc_bookmark_tags_verse_tag_index(owned,0,99,&index)==XC_INVALID_ARGUMENT&&!index&&!xc_bookmark_tags_verse_reference(owned,99)&&!xc_bookmark_tags_candidate_key(owned,99)&&xc_bookmark_tags_candidate_state(owned,99)==-1&&!xc_bookmark_tags_module(nullptr)&&!xc_bookmark_tags_candidate_count(nullptr)&&!xc_bookmark_tags_occurrences(nullptr)&&!xc_bookmark_tags_skipped_references(nullptr)&&!xc_bookmark_tags_snapshot_bytes(nullptr),"Safe invalid and null C accessors");
 xc_bookmark_tag_limits_default(&limits);limits.max_occurrences=3;check(xc_worker_read_bookmark_tags(worker,tab,path.c_str(),"Text","John3:16",&limits,&request)==XC_OK&&wait(request)==XC_OK,"C bounded annotation prefix");auto *prefix=xc_request_take_bookmark_tags(request);xc_request_release(request);check(xc_bookmark_tags_limited(prefix)&&xc_bookmark_tags_occurrences(prefix)==3&&xc_bookmark_tags_candidate_count(prefix)==5,"Limited C result retains complete candidate eligibility metadata");xc_bookmark_tags_release(prefix);
 limits.max_occurrences=5;check(xc_worker_read_bookmark_tags(worker,tab,path.c_str(),"Text","John3:16",&limits,&request)==XC_OK&&wait(request)==XC_OK,"C exact occurrence bound");prefix=xc_request_take_bookmark_tags(request);xc_request_release(request);check(!xc_bookmark_tags_limited(prefix)&&xc_bookmark_tags_occurrences(prefix)==5,"Exact complete C result is not marked limited");xc_bookmark_tags_release(prefix);
 limits.max_snapshot_bytes=1;check(xc_worker_read_bookmark_tags(worker,tab,path.c_str(),"Text","John3:16",&limits,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT&&!xc_request_take_bookmark_tags(request),"Metadata cannot silently truncate to fit a byte cap");xc_request_release(request);
 const std::string bad_context(1,static_cast<char>(0xff));check(xc_worker_read_bookmark_tags(worker,tab,path.c_str(),"Text",bad_context.c_str(),nullptr,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT,"Invalid annotation UTF-8 arrives as semantic failure");xc_request_release(request);
 check(xc_worker_read_bookmark_tags(worker,tab,path.c_str(),"Dictionary","John3:16",nullptr,&request)==XC_OK&&wait(request)==XC_INVALID_ARGUMENT,"Unsupported control module fails without false scripture highlights");xc_request_release(request);
 check(xc_worker_read_bookmark_tags(worker,tab,path.c_str(),"Text","John3:16",nullptr,&request)==XC_OK,"C annotation request queued for drain");xc_worker_close(worker);check(wait(request)==XC_OK,"Shutdown drains copied annotation read");auto *drained=xc_request_take_bookmark_tags(request);xc_request_release(request);xc_bookmark_tags_release(drained);
 std::ifstream in(file,std::ios::binary);const std::string after(std::istreambuf_iterator<char>(in),{});check(after==source,"Annotation reads preserve exact selected-file bytes");in.close();std::filesystem::remove(file);
 check(std::string(xc_bookmark_tags_verse_reference(owned,0))=="John.3.16"&&xc_bookmark_tags_snapshot_bytes(owned)>0,"Owned annotation groups survive source/request/worker teardown");xc_bookmark_tags_release(owned);xc_bookmark_tags_release(nullptr);
 std::cout<<"C owned annotations: bounded copied admission, exact eligibility, ordered overlaps, limits, allocation-free publication and draining passed\n";return 0;
}catch(const std::exception &e){watching=false;std::cerr<<e.what()<<'\n';return 1;}}
