// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "book_tree_source.hpp"
#include <rawgenbook.h>
#include <rawld.h>
#include <swmgr.h>
#include <treekeyidx.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <map>
#include <stdexcept>
#include <thread>
using namespace xiphos::core;
namespace fs=std::filesystem;
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
template<class Error,class F> void rejects(F call) {
  try { call(); } catch (const Error &) { return; }
  throw std::runtime_error("Expected book child-page error");
}
std::map<std::string,std::string> bytes(const fs::path &root) {
  std::map<std::string,std::string> result;
  for (const auto &item:fs::recursive_directory_iterator(root)) if (item.is_regular_file()) {
    std::ifstream input(item.path(),std::ios::binary); check(bool(input),"Read fixture bytes");
    result.emplace(fs::relative(item.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(input),{}));
  }
  return result;
}
xc_status wait(xc_request *request) {
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(10);
  xc_status status;
  while ((status=xc_request_poll(request))==XC_PENDING) {
    check(std::chrono::steady_clock::now()<end,"Child page request deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return status;
}
std::vector<BookTreeNode> frozen(sword::SWModule &module,unsigned long parent) {
  std::unique_ptr<sword::SWKey> owned(module.createKey()); auto &key=dynamic_cast<sword::TreeKeyIdx &>(*owned);
  key.setOffset(parent); std::vector<BookTreeNode> result;
  if (key.firstChild()) do {
    check(result.size()<1000,"Frozen fixture traversal bound");
    result.push_back({key.getOffset(),key.getText(),key.getLocalName(),key.hasChildren()});
  } while (key.nextSibling());
  return result;
}
void same(const BookTreeNode &a,const BookTreeNode &b) {
  check(a.offset==b.offset && a.path==b.path && a.title==b.title && a.has_children==b.has_children,"Copied child matches frozen SDK order/content");
}
int main(int argc,char **argv) {
  try {
    check(argc==2,"Generated child-page fixture root required"); const fs::path root(argv[1]);
    fs::create_directories(root/"mods.d");fs::create_directories(root/"data");
    const auto prefix=(root/"data/book").string();
    check(sword::RawGenBook::createModule(prefix.c_str())==0,"Create original child fixture");
    {
      sword::RawGenBook writer(prefix.c_str());
      for (const auto *path:{"/Part One","/Part One/Leaf","/123 Numeric Title","/Empty Branch/Leaf"}) {
        writer.setKey(path);writer.setEntry("Original tree prose.");
      }
      for (unsigned i=0;i<600;++i) {
        const auto title=i==1?std::string("123 Numeric"):i==2?std::string("café α"):"Node "+std::to_string(i);
        writer.setKey(("/Wide/"+title).c_str());writer.setEntry("Original wide-branch prose.");
      }
    }
    {std::ofstream conf(root/"mods.d/book.conf");conf<<"[Book]\nDataPath=./data/book\nModDrv=RawGenBook\nEncoding=UTF-8\nDescription=Original child-page fixture\n";}
    check(sword::RawLD::createModule((root/"data/dictionary").string().c_str())==0,"Create wrong module");
    {std::ofstream conf(root/"mods.d/dictionary.conf");conf<<"[Dictionary]\nDataPath=./data/dictionary\nModDrv=RawLD\n";}
    fs::remove(root/"mods.d/modules-conf.cache");const auto before=bytes(root/"data");
    std::vector<BookTreeNode> expected,roots;std::uint64_t wide=0;
    BookChildren retained;
    {
      Reader reader(root.string());
      wide=reader.book_tree_path("Book","/Wide").node.offset;
      sword::SWMgr legacy(root.string().c_str(),true,nullptr,false,false);auto &module=*legacy.getModule("Book");
      expected=frozen(module,static_cast<unsigned long>(wide));roots=frozen(module,0);
      check(expected.size()==600 && roots.size()==4,"Fixture exceeds one maximum page");
      for (auto limit:{1u,64u,127u,256u}) {
        std::optional<std::uint64_t> after;std::size_t index=0;
        do {
          const auto page=reader.book_children("Book",wide,after,limit);
          check(page.module=="Book" && page.parent.offset==wide && page.parent.path=="/Wide" && page.parent.has_children,"Owned page parent source");
          check(!page.children.empty() && page.children.size()<=limit,"Nonempty bounded continuation");
          for(const auto &child:page.children) {check(index<expected.size(),"No extra children");same(child,expected[index++]);}
          check(bool(page.next_after)==(index<expected.size()),"Lookahead continuation agrees with actual remaining siblings");
          if (page.next_after) check(*page.next_after==page.children.back().offset,"Continuation is the last returned child");
          after=page.next_after;
        } while(after);
        check(index==expected.size(),"All pages preserve complete legacy sibling sequence");
      }
      const auto top=reader.book_children("Book",0,std::nullopt,256);
      check(top.children.size()==roots.size() && !top.next_after && top.parent.offset==0 && top.parent.title=="Book","Root caption/source and exact final page");
      for(std::size_t i=0;i<roots.size();++i)same(top.children[i],roots[i]);
      const auto empty=reader.book_children("Book",8);
      check(empty.children.empty() && !empty.next_after && !empty.parent.has_children,"Leaf has no synthetic children");
      check(reader.book_children("Book",wide,expected.back().offset).children.empty(),"Cursor after final child returns empty final page");
      check(reader.book_children("Book",wide,expected[598].offset,1).children.size()==1 && !reader.book_children("Book",wide,expected[598].offset,1).next_after,"Exact one-child final page needs no extra request");
      rejects<std::invalid_argument>([&]{reader.book_children("Book",wide,std::nullopt,0);});
      rejects<std::invalid_argument>([&]{reader.book_children("Book",wide,std::nullopt,257);});
      rejects<std::invalid_argument>([&]{reader.book_children("Book",wide,8);});
      rejects<std::invalid_argument>([&]{reader.book_children("Book",0,0);});
      rejects<std::invalid_argument>([&]{reader.book_children("Book",wide,wide);});
      rejects<std::invalid_argument>([&]{reader.book_children("Book",1);});
      rejects<std::invalid_argument>([&]{reader.book_children("Book",wide,1);});
      rejects<std::out_of_range>([&]{reader.book_children("Book",40000);});
      rejects<std::out_of_range>([&]{reader.book_children("Book",wide,40000);});
      rejects<std::out_of_range>([&]{reader.book_children("Missing",0);});
      rejects<std::invalid_argument>([&]{reader.book_children("Dictionary",0);});
      rejects<std::invalid_argument>([&]{reader.book_children(std::string("Book\0bad",8),0);});
      auto &live=dynamic_cast<sword::TreeKeyIdx &>(*module.getKey());live.setOffset(8);const auto *identity=module.getKey();
      const auto attributes=module.getEntryAttributes();const bool processing=module.isProcessEntryAttributes();
      retained=detail::capture_book_children(module,wide,std::nullopt,2);
      rejects<std::invalid_argument>([&]{detail::capture_book_children(module,wide,8,2);});
      check(module.getKey()==identity && live.getOffset()==8 && module.getEntryAttributes()==attributes && module.isProcessEntryAttributes()==processing,"Page queries/failures preserve key identity/selection and render state");
    }
    check(retained.children.size()==2 && retained.children[1].title=="123 Numeric" && retained.next_after,"Page ownership survives Reader/SDK destruction");
    std::future<BookChildren> drained;
    {
      StudyWorker worker(root.string());auto bad=worker.book_children("Book",wide,8,2);auto good=worker.book_children("Book",wide,std::nullopt,3);
      rejects<std::invalid_argument>([&]{(void)bad.get();});check(good.get().children[2].title=="café α","Failed page does not stop worker or lose Unicode");
      drained=worker.book_children("Book",wide,expected[255].offset,256);
    }
    check(drained.get().children.front().offset==expected[256].offset,"Page close drains copied continuation");
    xc_worker *worker=nullptr;check(xc_worker_open(root.string().c_str(),&worker)==XC_OK,"Open child-page C worker");
    xc_request *request=reinterpret_cast<xc_request *>(1);
    check(xc_worker_book_children(worker,"Book",wide,2,0,1,&request)==XC_INVALID_ARGUMENT && !request,"Strict cursor flag");
    check(xc_worker_book_children(worker,"Book",wide,0,0,0,&request)==XC_INVALID_ARGUMENT && !request,"Zero page limit rejects immediately");
    check(xc_worker_book_children(worker,"Book",wide,0,0,257,&request)==XC_INVALID_ARGUMENT && !request,"Oversized page limit rejects immediately");
    check(xc_worker_book_children(worker,nullptr,wide,0,0,1,&request)==XC_INVALID_ARGUMENT && !request,"NULL module rejects");
    check(xc_worker_book_children(worker,"Book",wide,0,0,1,nullptr)==XC_INVALID_ARGUMENT,"NULL request output rejects");
    uint64_t after=0;int has_after=0;std::size_t index=0;xc_book_children *held=nullptr;
    do {
      char id[]="Book";
      check(xc_worker_book_children(worker,id,wide,has_after,after,127,&request)==XC_OK,"Queue copied C child-page request");id[0]='X';
      check(!xc_request_take_book_children(request) && wait(request)==XC_OK && !xc_request_take_book_tree(request),"Page take/poll and wrong-kind contract");
      auto *page=xc_request_take_book_children(request);check(page && !xc_request_take_book_children(request),"Take child page once");xc_request_release(request);
      check(std::string(xc_book_children_module(page))=="Book" && xc_book_children_count(page)<=127,"Copied module and bounded C count");
      xc_book_node node{};check(xc_book_children_parent(page,&node)==XC_OK && node.present && node.offset==wide && std::string(node.path)=="/Wide","C parent source");
      for(std::size_t i=0;i<xc_book_children_count(page);++i) {
        check(xc_book_children_get_node(page,i,&node)==XC_OK && node.present,"Borrow C child");
        check(index<expected.size(),"Bound C collected children");const auto &value=expected[index++];
        check(node.offset==value.offset && node.path==value.path && node.title==value.title && bool(node.has_children)==value.has_children,"C page reproduces every frozen child");
      }
      check(xc_book_children_get_node(page,xc_book_children_count(page),&node)==XC_INVALID_ARGUMENT && !node.present && !node.path && !node.offset,"Index bounds clear stale node");
      check(xc_book_children_get_node(page,0,nullptr)==XC_INVALID_ARGUMENT && xc_book_children_parent(page,nullptr)==XC_INVALID_ARGUMENT,"NULL view output rejects");
      has_after=xc_book_children_next_after(page,&after);check(bool(has_after)==(index<expected.size()),"C continuation matches remaining children");
      if(!held)held=page;else xc_book_children_release(page);
    }while(has_after);
    check(index==600 && !after,"C pages finish without duplicate/missing children");
    check(xc_worker_book_children(worker,"Book",wide,1,8,2,&request)==XC_OK && wait(request)==XC_INVALID_ARGUMENT,"C wrong-parent cursor fails at poll");xc_request_release(request);
    check(xc_worker_book_children(worker,"Book",wide,1,40000,2,&request)==XC_OK && wait(request)==XC_MODULE_NOT_FOUND,"C missing cursor fails at poll");xc_request_release(request);
    check(xc_worker_book_children(worker,"Book",0,0,0,1,&request)==XC_OK,"Queue root before C close");xc_worker_close(worker);
    check(wait(request)==XC_OK,"C close drains pages");auto *root_page=xc_request_take_book_children(request);xc_request_release(request);
    xc_book_node node{};check(xc_book_children_parent(root_page,&node)==XC_OK && node.present && !node.offset && !*node.path && std::string(node.title)=="Book","C root parent remains present at zero offset");
    check(xc_book_children_get_node(held,2,&node)==XC_OK && std::string(node.title)=="café α","Borrowed strings valid until owned page released after worker close");
    xc_book_children_release(root_page);xc_book_children_release(held);xc_book_children_release(nullptr);
    check(bytes(root/"data")==before,"Paged successes/failures preserve every module data byte and filename");
    std::cout<<"600-child frozen paging, cursor guards, C ownership and module-byte checks passed\n";return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
