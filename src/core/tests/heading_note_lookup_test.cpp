// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include "xiphos/core/session.hpp"
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
std::string note_uri(const std::string &html, const std::string &id) {
  const auto value = html.find("&value=" + id + "&");
  check(value != std::string::npos, "capture emits the requested scoped note URI");
  const auto start = html.rfind("href=\"", value), end = html.find('"', value);
  check(start != std::string::npos && end != std::string::npos, "SDK note anchor remains bounded");
  return html.substr(start + 6, end - start - 6);
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "private heading capture fixture required");
    StudyNote held; std::string uri, id;
    {
      Reader reader(argv[1]); EngineOptions options; options.footnotes = true;
      for (const auto *module : {"Notes", "ThMLNotes", "OSISBefore", "ThMLBefore"}) {
        const auto baseline = reader.read_commentary_entry(module, "John4:1", true, options);
        for (bool labels : {false, true}) for (std::size_t fragment = 0; fragment < (std::string(module).find("OSIS") != std::string::npos || std::string(module) == "Notes" ? 1u : 2u); ++fragment) {
          reader.read_commentary_entry(module, "John4:4", true, options);
          const auto source = reader.heading_notes(module, "John4:1", fragment, options, labels);
          check(source.notes.size() == 1 && source.html.find("value=&") == std::string::npos,
                "Reader source API selects requested verse and emits scoped heading links");
          const auto &expected = source.notes.begin()->second;
          HeadingNoteTarget target;
          check(parse_heading_note_identifier(expected.identifier, target) && target.fragment == fragment,
                "Reader source identity retains the exact SDK fragment");
          const auto command = study_note_link(note_uri(source.html, expected.identifier), module);
          reader.read_commentary_entry(module, "John4:4", true, options);
          const auto note = reader.study_note(command.module, command.reference, command.identifier, options);
          check(note.identifier == expected.identifier && note.module == module && note.reference == "John 4:1" &&
                note.html == expected.html && note.label == expected.label && note.type == expected.type,
                "heading lookup uses scoped source context after later reads, never the body's colliding SDK ID");
          const auto metadata = reader.study_note_metadata(module, "John4:1", expected.identifier, options);
          check(metadata.html.empty() && metadata.label == note.label && metadata.identifier == note.identifier,
                "heading metadata lookup does not return another note or render its body");
          auto hidden = options; hidden.footnotes = false;
          check(reader.study_note(module, "John4:1", expected.identifier, hidden).html == expected.html,
                "issued scoped heading links still resolve after display footnotes are hidden");
          const auto off = reader.heading_notes(module, "John4:1", fragment, hidden, false, false);
          check(off.html.find("action=showNote") == std::string::npos,
                "temporary lookup Footnotes On is restored for subsequent configured display");
          bool failed = false;
          try { reader.study_note(module, "John4:1", expected.identifier); } catch (const std::invalid_argument &) { failed = true; }
          check(failed, "unconfigured heading lookup never guesses engine state");
          const auto after = reader.read_commentary_entry(module, "John4:1", true, options);
          check(after.html == baseline.html && after.heading_html == baseline.heading_html,
                "new source/lookup APIs leave old Reader HTML and SDK label baselines unchanged");
          held = note;
        }
      }
      const auto source = reader.heading_notes("Notes", "John5:1", 0, options, true);
      const auto cross_id = heading_note_identifier(0, "2");
      const auto cross = reader.study_note("Notes", "John5:1", cross_id, options);
      check(cross.type == "crossReference" && cross.reference_list.find("John.4.1") != std::string::npos &&
            source.notes.at(heading_note_identifier(0, "3")).html.empty(), "scoped cross-reference lists and real empty notes stay distinct");
      for (const auto &bad : {heading_note_identifier(0, "999"), heading_note_identifier(999, "1"), std::string("xiphos-heading-01-31")}) {
        bool failed = false;
        try { reader.study_note("Notes", "John5:1", bad, options); } catch (const std::invalid_argument &) { failed = true; }
        check(failed, "missing or malformed scoped identifiers reject rather than falling through to body lookup");
      }
      bool bad_input = false;
      try { reader.heading_notes(std::string("Notes\0suffix", 12), "John4:1", 0, options, true); } catch (const std::invalid_argument &) { bad_input = true; }
      check(bad_input, "Reader heading source rejects NUL module input");
      StudySession session(reader); session.set_engine_options(options);
      id = heading_note_identifier(0, "1");
      const auto snapshot = reader.heading_notes("Notes", "John4:1", 0, options, true);
      uri = note_uri(snapshot.html, id);
      held = session.study_note("Notes", "John4:1", id);
      check(held.html.find("Original heading note.") != std::string::npos && session.history().entries().empty(),
            "session shared lookup resolves heading notes without navigation history");
    }
    std::future<StudyNote> drained;
    {
      StudyWorker worker(argv[1]); const auto tab = worker.create_tab().get();
      worker.enable_module_preferences(tab).get();
      drained = worker.study_note_link(tab, "Notes", uri);
    }
    check(drained.get().html == held.html, "existing worker dispatch carries scoped heading identity through shutdown");
    xc_worker *opened = nullptr; check(xc_worker_open(argv[1], &opened) == XC_OK, "open C heading worker");
    std::unique_ptr<xc_worker, decltype(&xc_worker_close)> worker(opened, xc_worker_close);
    xc_request *request = nullptr;
    auto complete = [&](xc_status status) {
      std::unique_ptr<xc_request, decltype(&xc_request_release)> done(std::exchange(request, nullptr), xc_request_release);
      check(status == XC_OK && done, "accept C heading lookup request");
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (xc_request_poll(done.get()) == XC_PENDING) {
        check(std::chrono::steady_clock::now() < deadline, "C heading lookup deadline");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      check(xc_request_poll(done.get()) == XC_OK, "complete C heading lookup request"); return done;
    };
    auto done = complete(xc_worker_create_tab(worker.get(), &request)); const auto tab = xc_request_tab(done.get());
    done = complete(xc_worker_enable_module_preferences(worker.get(), tab, &request));
    done = complete(xc_worker_study_note_link(worker.get(), tab, "Notes", uri.c_str(), &request));
    std::unique_ptr<xc_note, decltype(&xc_note_release)> copied(xc_request_take_note(done.get()), xc_note_release);
    check(copied && std::string(xc_note_identifier(copied.get())) == id && std::string(xc_note_html(copied.get())) == held.html,
          "existing C note snapshots copy scoped heading identity and body");
    worker.reset(); done.reset();
    check(std::string(xc_note_html(copied.get())) == held.html, "C heading body survives request and worker destruction");
    std::cout << "Scoped heading source/lookup, filter restoration and worker/C ownership passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
