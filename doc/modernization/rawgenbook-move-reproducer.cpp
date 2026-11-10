// SPDX-License-Identifier: GPL-2.0-or-later
#include <rawgenbook.h>
#include <swmgr.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <treekeyidx.h>

int main(int argc, char **argv) {
  namespace fs = std::filesystem;
  if (argc != 2 || fs::exists(argv[1])) return 2;
  const fs::path root = argv[1];
  fs::create_directories(root / "data");
  fs::create_directories(root / "mods.d");
  const auto prefix = (root / "data" / "entries").string();
  if (sword::RawGenBook::createModule(prefix.c_str())) return 3;
  {
    std::ofstream conf(root / "mods.d" / "probe.conf");
    conf << "[Probe]\nDataPath=./data/entries\nModDrv=RawGenBook\nEncoding=UTF-8\n";
    sword::RawGenBook book(prefix.c_str());
    auto *key = dynamic_cast<sword::TreeKeyIdx *>(book.getKey());
    if (!key) return 4;
    for (const auto *path : {"/Retained", "/Parent/Moved", "/Parent/Moved/Child", "/Destination/Existing"}) {
      key->assureKeyPath(path);
      key->save();
      book.setEntry(std::string(path).find("Child") != std::string::npos ? "childword" : "sectionword");
    }
    std::unique_ptr<sword::SWKey> linked(book.createKey());
    linked->setText("/Parent/Moved");
    key->assureKeyPath("/RetainedAlias");
    key->save();
    book.linkEntry(linked.get());
  }
  unsigned long source = 0, child = 0;
  {
    sword::SWMgr manager(root.string().c_str());
    auto *book = manager.getModule("Probe");
    if (!book || book->createSearchFramework()) return 5;
    book->deleteSearchFramework();
    auto *tree = dynamic_cast<sword::TreeKeyIdx *>(book->getKey());
    if (!tree) return 6;
    auto offset = [&](const char *path) {
      tree->setText(path);
      return tree->getError() ? 0UL : tree->getOffset();
    };
    child = offset("/Parent/Moved/Child");
    const auto destination = offset("/Destination");
    source = offset("/Parent/Moved");
    if (!source || !child || !destination || tree->reparent(child) ||
        !tree->reparent(destination)) return 7;
    book->flush();
  }
  sword::SWMgr manager(root.string().c_str());
  auto *book = manager.getModule("Probe");
  if (!book) return 8;
  auto *tree = dynamic_cast<sword::TreeKeyIdx *>(book->getKey());
  tree->setText("/Parent/Moved");
  if (!tree->popError()) return 9;
  tree->setOffset(source);
  if (tree->getError() || std::string(tree->getText()) != "/Destination/Moved") return 10;
  tree->setOffset(child);
  if (tree->getError() || std::string(tree->getText()) != "/Destination/Moved/Child" ||
      std::string(book->getRawEntryBuf().c_str()) != "childword") return 11;
  book->setKey("/RetainedAlias");
  if (book->popError() || std::string(book->getRawEntryBuf().c_str()) != "sectionword") return 14;
  if (book->createSearchFramework()) return 12;
  auto &hits = book->search("childword", -4);
  if (hits.getCount() != 1 || std::string(hits.getElement(0)->getText()) != "/Destination/Moved/Child")
    return 13;
  std::cout << "Moved subtree preserved offsets, content and indexed result path\n";
}
