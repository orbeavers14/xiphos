// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
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
xc_status wait(xc_request *request) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  xc_status status;
  while ((status = xc_request_poll(request)) == XC_PENDING) {
    check(std::chrono::steady_clock::now() < deadline, "request timeout");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return status;
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
    xc_worker *bridge = nullptr;
    xc_request *request = nullptr;
    check(xc_worker_open(root.string().c_str(), &bridge) == XC_OK, "C worker opened");
    check(xc_worker_create_tab(bridge, &request) == XC_OK && wait(request) == XC_OK, "C tab created");
    const auto ctab = xc_request_tab(request); xc_request_release(request);
    check(xc_worker_visit_passage(bridge, ctab, "Text", nullptr, "John 3:16", &request) == XC_INVALID_ARGUMENT && !request,
          "null commentary rejected before submission");
    check(xc_worker_visit_passage(bridge, ctab, "Text", "Notes", "John 3:16", &request) == XC_OK, "C pair accepted");
    check(xc_request_take_passage(request) == nullptr, "pending pair cannot transfer before poll");
    check(wait(request) == XC_OK && xc_request_take_entry(request) == nullptr, "pair ready with distinct type");
    auto *owned = xc_request_take_passage(request);
    check(owned && !xc_request_take_passage(request), "pair transfers exactly once");
    xc_request_release(request);
    check(xc_worker_read_passage(bridge, ctab, "Text", "", "John 3:17", &request) == XC_OK && wait(request) == XC_OK,
          "C optional commentary read");
    auto *hidden = xc_request_take_passage(request); xc_request_release(request);
    check(!xc_passage_has_commentary(hidden) && !xc_passage_commentary_reference(hidden) &&
          !xc_passage_commentary_html(hidden) && std::string(xc_passage_commentary_module(hidden)).empty(), "absence is explicit");
    xc_passage_release(hidden);
    xc_worker_close(bridge);
    check(std::string(xc_passage_module(owned)) == "Text" && std::string(xc_passage_reference(owned)) == "John 3:16" &&
          std::string(xc_passage_commentary_html(owned)).find("café") != std::string::npos, "pair survives worker/request release");
    xc_passage_release(owned);
    check(!xc_passage_has_commentary(nullptr) && !xc_passage_html(nullptr) && !xc_request_take_passage(nullptr), "null snapshot guards");
    xc_passage_release(nullptr);
    std::cout << "Synchronized study passage checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
