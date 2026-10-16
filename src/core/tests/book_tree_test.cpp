// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include "book_tree_source.hpp"
#include <swmgr.h>
#include <rawgenbook.h>
#include <rawld.h>
#include <treekeyidx.h>
#include <versekey.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <stdexcept>
using namespace xiphos::core;
namespace fs = std::filesystem;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
template<class Error, class F> void rejects(F call) {
  try { call(); } catch (const Error &) { return; }
  throw std::runtime_error("Expected typed book-tree error");
}
std::map<std::string,std::string> bytes(const fs::path &root) {
  std::map<std::string,std::string> result;
  for (const auto &item : fs::recursive_directory_iterator(root)) if (item.is_regular_file()) {
    std::ifstream input(item.path(), std::ios::binary);
    check(bool(input), "Open book data for byte comparison");
    result.emplace(fs::relative(item.path(),root).generic_string(),
                   std::string(std::istreambuf_iterator<char>(input), {}));
  }
  return result;
}
void same(const BookTreeNode &actual, sword::TreeKeyIdx &expected, const std::string &module) {
  check(actual.offset == expected.getOffset(), "Frozen SDK offset");
  check(actual.path == expected.getText(), "Frozen SDK canonical path");
  check(actual.title == (actual.offset ? expected.getLocalName() : module), "Legacy root/module caption and local title");
  check(actual.has_children == expected.hasChildren(), "Frozen SDK child presence");
}
void frozen(const BookTree &actual, sword::SWModule &module) {
  std::unique_ptr<sword::SWKey> owned(module.createKey());
  auto &key = dynamic_cast<sword::TreeKeyIdx &>(*owned);
  key.setOffset(static_cast<unsigned long>(actual.node.offset));
  same(actual.node,key,module.getName());
  const auto relative = [&](const std::optional<BookTreeNode> &value, bool (sword::TreeKeyIdx::*step)()) {
    // Same copy/setOffset/step sequence used by BackEnd's tree helpers.
    sword::TreeKeyIdx legacy(key);
    legacy.setOffset(static_cast<unsigned long>(actual.node.offset));
    const bool exists = (legacy.*step)();
    check(bool(value) == exists, "Frozen legacy relationship availability");
    if (exists) same(*value,legacy,module.getName());
  };
  relative(actual.parent,&sword::TreeKeyIdx::parent);
  relative(actual.first_child,&sword::TreeKeyIdx::firstChild);
  relative(actual.previous_sibling,&sword::TreeKeyIdx::previousSibling);
  relative(actual.next_sibling,&sword::TreeKeyIdx::nextSibling);
}
int main(int argc,char **argv) {
  try {
    check(argc==2,"Private generated fixture root required");
    const fs::path root(argv[1]);
    fs::create_directories(root/"mods.d"); fs::create_directories(root/"data");
    const auto prefix=(root/"data/book").string(), empty=(root/"data/empty").string();
    check(sword::RawGenBook::createModule(prefix.c_str())==0,"Create original-prose book");
    check(sword::RawGenBook::createModule(empty.c_str())==0,"Create empty book");
    const std::vector<std::string> paths{"", "/Part One", "/Part One/Section α", "/Part One/Section Two",
      "/123 Numeric Title", "/Empty Branch", "/Empty Branch/Leaf"};
    {
      sword::RawGenBook writer(prefix.c_str());
      for (const auto &path : paths) if (!path.empty() && path!="/Empty Branch") {
        writer.setKey(path.c_str()); writer.setEntry(("Original prose for "+path).c_str());
      }
    }
    for (const auto &id : {"Book", "Empty"}) {
      std::ofstream conf(root/"mods.d"/(std::string(id)+".conf"));
      conf << '[' << id << "]\nDataPath=./data/" << (std::string(id)=="Book"?"book":"empty")
           << "\nModDrv=RawGenBook\nSourceType=ThML\nEncoding=UTF-8\nDescription=Original prose tree fixture\n";
    }
    check(sword::RawLD::createModule((root/"data/lexicon").string().c_str())==0,"Create wrong-type fixture");
    { std::ofstream conf(root/"mods.d/dictionary.conf"); conf << "[Dictionary]\nDataPath=./data/lexicon\nModDrv=RawLD\n"; }
    const auto deep=(root/"data/deep").string();
    check(sword::RawGenBook::createModule(deep.c_str())==0,"Create bounded-depth fixture");
    {
      sword::RawGenBook writer(deep.c_str());
      std::string path;
      for (unsigned i=0;i<258;++i) path+="/n";
      writer.setKey(path.c_str()); writer.setEntry("Original deep leaf prose.");
      std::ofstream conf(root/"mods.d/deep.conf"); conf << "[Deep]\nDataPath=./data/deep\nModDrv=RawGenBook\n";
    }
    { std::ofstream conf(root/"mods.d/verse-book.conf"); conf << "[VerseBook]\nDataPath=./data/book\nModDrv=RawGenBook\nKeyType=VerseKey\n"; }
    fs::remove(root/"mods.d/modules-conf.cache");
    const auto before=bytes(root/"data"); check(!before.empty(),"Book bytes captured");
    BookTree retained;
    {
      Reader reader(root.string());
      sword::SWMgr legacy(root.string().c_str(),true,nullptr,false,false);
      check(dynamic_cast<sword::VerseKey *>(legacy.getModule("VerseBook")->getKey()),"SDK non-indexed generic-book fixture is a verse key");
      unsigned indexed=0;
      for (const auto &item : reader.modules()) {
        const bool expected=item.id=="Book" || item.id=="Empty" || item.id=="Deep";
        check(item.indexed_book==expected,"Runtime indexed-book catalog capability");
        if(item.indexed_book) ++indexed;
      }
      check(indexed==3,"All eligible indexed books discovered");
      rejects<std::invalid_argument>([&]{reader.book_tree("VerseBook",0);});
      rejects<std::invalid_argument>([&]{reader.book_tree_path("VerseBook","");});
      rejects<std::invalid_argument>([&]{reader.book_children("VerseBook",0);});
      rejects<std::invalid_argument>([&]{reader.read_book("VerseBook",0,{});});
      auto *module=legacy.getModule("Book"); check(module,"Load frozen book");
      auto *live=dynamic_cast<sword::TreeKeyIdx *>(module->getKey()); check(live,"Book has live indexed tree");
      live->setOffset(12);
      auto *identity=module->getKey();
      const auto offset=live->getOffset(); const std::string live_path=live->getText();
      const auto attributes=module->getEntryAttributes(); const bool processing=module->isProcessEntryAttributes();
      for (std::size_t i=0;i<paths.size();++i) {
        const auto tree=reader.book_tree("Book",i*4);
        check(tree.module=="Book" && tree.node.path==paths[i],"Known fixture creation order/path");
        frozen(tree,*module);
        const auto from_path=reader.book_tree_path("Book",paths[i]);
        check(from_path.node.offset==i*4,"Path/offset identity, including numeric title");
        frozen(from_path,*module);
        frozen(detail::capture_book_tree(*module,i*4),*module);
        frozen(detail::capture_book_tree_path(*module,paths[i]),*module);
      }
      const auto root_tree=reader.book_tree("Book",0);
      check(root_tree.node.title=="Book" && !root_tree.parent && root_tree.first_child->offset==4,"Actual root distinct from first reading node");
      const auto top=reader.book_tree("Book",4);
      check(top.parent && top.parent->offset==0 && !top.previous_sibling && top.next_sibling->offset==16,"Top-level root parent retained");
      const auto branch=reader.book_tree("Book",20);
      check(branch.node.has_children && branch.first_child->offset==24,"Auto-created branch remains queryable");
      const auto leaf=reader.book_tree("Book",24);
      check(!leaf.first_child && !leaf.previous_sibling && !leaf.next_sibling,"Leaf relationship boundaries");
      const auto empty_tree=reader.book_tree("Empty",0);
      check(empty_tree.node.path.empty() && empty_tree.node.title=="Empty" && !empty_tree.node.has_children && !empty_tree.first_child,"Empty root does not synthesize a chapter");
      rejects<std::out_of_range>([&]{reader.book_tree("Empty",4);});
      rejects<std::out_of_range>([&]{reader.book_tree("Missing",0);});
      rejects<std::invalid_argument>([&]{reader.book_tree("Dictionary",0);});
      rejects<std::invalid_argument>([&]{reader.book_tree("",0);});
      rejects<std::invalid_argument>([&]{reader.book_tree(std::string("Book\0suffix",11),0);});
      for (auto bad : {std::uint64_t(1),std::uint64_t(6),std::uint64_t(2147483648ULL),std::numeric_limits<std::uint64_t>::max()})
        rejects<std::invalid_argument>([&]{reader.book_tree("Book",bad);});
      for (auto bad : {std::uint64_t(28),std::uint64_t(40000),std::uint64_t(2147483644)})
        rejects<std::out_of_range>([&]{reader.book_tree("Book",bad);});
      for (const auto &bad : {"/Missing/path","/Part One/Missing","/Part One/", "/"})
        rejects<std::out_of_range>([&]{reader.book_tree_path("Book",bad);});
      rejects<std::invalid_argument>([&]{reader.book_tree_path("Book","123 Numeric Title");});
      rejects<std::invalid_argument>([&]{reader.book_tree_path("Book",std::string("/Part One\0junk",14));});
      rejects<std::invalid_argument>([&]{reader.book_tree_path("Book","/"+std::string(4096,'x'));});
      rejects<std::out_of_range>([&]{detail::capture_book_tree(*module,40000);});
      rejects<std::out_of_range>([&]{detail::capture_book_tree_path(*module,"/Missing/path");});
      check(module->getKey()==identity && live->getOffset()==offset && live_path==live->getText(),"Queries/failures preserve live key identity/position");
      check(module->getEntryAttributes()==attributes && module->isProcessEntryAttributes()==processing,"Queries preserve render attributes/processing flag");
      rejects<std::runtime_error>([&]{reader.book_tree("Deep",258*4);});
      check(reader.book_tree("Deep",0).first_child->offset==4,"Depth limit does not block shallow queries");
      retained=reader.book_tree("Book",8);
      check(reader.book_tree("Book",16).node.title=="123 Numeric Title","Read after rejected query");
    }
    check(retained.node.path=="/Part One/Section α" && retained.parent->title=="Part One" && retained.next_sibling->offset==12,"Snapshots outlive reader and later queries");
    check(bytes(root/"data")==before,"Successful/failed queries preserve every module data byte and filename");
    std::cout << "Owned book tree, frozen relationships, rejection, key lifetime and module-byte checks passed\n";
    return 0;
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
