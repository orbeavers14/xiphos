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
    for (const auto *path : {"/Retained", "/Parent/Deleted"}) {
      key->assureKeyPath(path);
      key->save();
      book.setEntry(path[1] == 'R' ? "retainedword" : "deletedword");
    }
  }
  {
    sword::SWMgr manager(root.string().c_str());
    auto *book = manager.getModule("Probe");
    if (!book || book->createSearchFramework()) return 5;
    book->deleteSearchFramework();
    book->setKey("/Parent/Deleted");
    if (book->popError()) return 6;
    book->deleteEntry();
    book->flush();
  }
  sword::SWMgr manager(root.string().c_str());
  auto *book = manager.getModule("Probe");
  if (!book) return 7;
  book->setKey("/Parent");
  if (book->popError()) return 8;
  std::cerr << "Rebuilding after deletion\n";
  if (book->createSearchFramework()) return 9;
  const auto retained = book->search("retainedword", -4).getCount();
  const auto deleted = book->search("deletedword", -4).getCount();
  std::cerr << "Indexed counts retained=" << retained << " deleted=" << deleted << '\n';
  return retained && !deleted ? 0 : 10;
}
