// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include <rawtext.h>
#include <rawcom.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
template<class Error, class F> void fails(F action, const char *message) {
  try { action(); } catch (const Error &) { return; }
  throw std::runtime_error(message);
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture path required");
    const std::filesystem::path root(argv[1]);
    std::filesystem::create_directories(root / "mods.d");
    for (const auto &id : {"Text", "Notes", "OtherNotes"}) {
      const auto path = root / id;
      std::filesystem::create_directories(path);
      const bool text = std::string(id) == "Text";
      check((text ? sword::RawText::createModule(path.string().c_str())
                  : sword::RawCom::createModule(path.string().c_str())) == 0, "create original fixture");
      std::ofstream config(root / "mods.d" / (std::string(id) + ".conf"));
      config << '[' << id << "]\nDataPath=./" << id << "/\nModDrv=" << (text ? "RawText" : "RawCom")
             << "\nSourceType=OSIS\nEncoding=UTF-8\nVersification=KJV\nDescription=Original study pane fixture\n";
      if (text) {
        sword::RawText writer(path.string().c_str());
        writer.setKey("John 3:16"); writer.setEntry("Original Bible fixture café.");
        writer.setKey("John 3:17"); writer.setEntry("Original second Bible entry.");
      } else {
        sword::RawCom writer(path.string().c_str());
        writer.setKey("John 3:16"); writer.setEntry((std::string(id) + " original commentary café.").c_str());
        writer.setKey("John 3:17"); writer.setEntry((std::string(id) + " second commentary.").c_str());
      }
    }
    Reader reader(root.string());
    StudySession session(reader);
    const auto first = session.visit_passage("Text", "Notes", "Jn 3:16");
    check(first.text.reference == "John 3:16" && first.commentary->reference == first.text.reference,
          "commentary follows normalized Bible passage");
    check(first.commentary->html.find("Notes original commentary café") != std::string::npos,
          "real RawCom rendered snapshot");
    session.visit_passage("Text", "OtherNotes", "John 3:17");
    const auto back = session.navigate_passage(-1);
    check(back.commentary_module == "Notes" && back.text.reference == "John 3:16", "back restores recorded pair");
    const auto forward = session.navigate_passage(1);
    check(forward.commentary_module == "OtherNotes" && forward.commentary->html.find("second commentary") != std::string::npos,
          "forward restores commentary and passage");
    // The old application deduplicates on reference, preserving the first pair.
    session.visit_passage("Text", "OtherNotes", "John 3:16");
    check(session.history().entries().size() == 2 && session.history().entries()[0].commentary == "Notes",
          "legacy reference-only deduplication retains original pair");
    const auto cursor = session.history().cursor();
    fails<std::out_of_range>([&] { session.visit_passage("Text", "Missing", "John 3:18"); }, "missing commentary rejected");
    fails<std::invalid_argument>([&] { session.visit_passage("Text", "Text", "John 3:18"); }, "Bible cannot be commentary pane");
    fails<std::invalid_argument>([&] { session.visit_passage("Notes", "", "John 3:18"); }, "commentary cannot be Bible pane");
    check(session.history().cursor() == cursor && session.history().entries().size() == 2, "failed pair leaves history unchanged");
    const auto no_notes = session.read_passage("Text", "", "John 3:16");
    check(!no_notes.commentary && no_notes.commentary_module.empty(), "hidden commentary requires no module");
    check(session.history().cursor() == cursor, "read-only refresh leaves history unchanged");
    session.visit("Text", "John 3:18");
    check(session.history().entries().back().commentary.empty(), "existing visit remains compatible");
    StudyWorker worker(root.string());
    const auto tab = worker.create_tab().get(), other = worker.create_tab().get();
    std::string notes = "Notes";
    auto queued = worker.visit_passage(tab, "Text", notes, "John 3:16");
    notes = "OtherNotes";
    auto second = worker.visit_passage(tab, "Text", notes, "John 3:17");
    auto restored = worker.navigate_passage(tab, -1);
    check(queued.get().commentary_module == "Notes" && second.get().commentary_module == "OtherNotes", "queued inputs owned");
    check(restored.get().commentary_module == "Notes", "FIFO paired navigation");
    check(worker.history(other).get().entries.empty(), "paired tab history isolation");
    auto failed = worker.visit_passage(tab, "Text", "Missing", "John 3:18");
    fails<std::out_of_range>([&] { failed.get(); }, "queued failure propagated");
    check(worker.navigate_passage(tab, 1).get().commentary_module == "OtherNotes", "queue recovers after failed pair");
    const auto next = worker.navigate_passage_chapter(tab, "Text", "Notes", "John 3:17", 1).get();
    check(next.text.reference == "John 4:1" && next.commentary->reference == "John 4:1", "both panes follow chapter navigation");
    std::cout << "Synchronized study passage checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
