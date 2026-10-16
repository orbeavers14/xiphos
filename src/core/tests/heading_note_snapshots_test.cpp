// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "xiphos/core/link.hpp"
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <memory>
#include <utility>
#include <rawcom.h>
#include <versekey.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
std::vector<std::string> scoped_uris(const std::string &html) {
  std::vector<std::string> result; std::size_t at = 0;
  while ((at = html.find("href=\"", at)) != std::string::npos) {
    const auto start = at + 6, end = html.find('"', start); check(end != std::string::npos, "bounded snapshot href");
    const auto uri = html.substr(start, end - start); at = end + 1;
    if (uri.find("&value=xiphos-heading-") != std::string::npos) result.push_back(uri);
  }
  return result;
}
std::string legacy_ids(std::string html, bool filtered) {
  for (const auto &uri : scoped_uris(html)) {
    const auto link = study_note_link(uri, ""); HeadingNoteTarget target;
    check(parse_heading_note_identifier(link.identifier, target), "snapshot publishes canonical heading identity");
    const auto at = html.find("&value=" + link.identifier + "&");
    check(at != std::string::npos, "find exact scoped URI value");
    html.replace(at + 7, link.identifier.size(), filtered ? target.local_id : "");
  }
  return html;
}
void compare(const Entry &before, const Entry &after, bool filtered) {
  check(before.html == after.html && before.heading_html == legacy_ids(after.heading_html, filtered) &&
        before.lexical.css == after.lexical.css && before.lexical.applied == after.lexical.applied &&
        before.cleanup_diagnostics.size() == after.cleanup_diagnostics.size(),
        "opt-in snapshot changes only heading note IDs, never body/layout/cleanup or displayed labels/counters");
  check(before.note_source.reported_label == after.note_source.reported_label && before.note_source.headings.size() == after.note_source.headings.size(),
        "raw source metadata keeps its legacy meaning");
  for (std::size_t i = 0; i < before.note_source.headings.size(); ++i)
    check(before.note_source.headings[i].html == after.note_source.headings[i].html &&
          before.note_source.headings[i].reported_note_label == after.note_source.headings[i].reported_note_label,
          "raw copied Reader heading HTML remains unchanged by link presentation");
}
int main(int argc, char **argv) {
  try {
    check(argc == 3, "capture source and private session fixture required");
    const std::filesystem::path root(argv[2]); std::filesystem::create_directories(root);
    std::filesystem::copy(argv[1], root, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    { std::ofstream config(root / "mods.d/notes.conf", std::ios::app); config << "GlobalOptionFilter=OSISStrongs\n"; }
    {
      sword::RawCom writer((root / "data").string().c_str()); writer.setKey("John6:1");
      writer.setEntry("<div subType=\"x-preverse\"><title>Original mixed <note swordFootnote=\"999\" n=\"a\" type=\"study\">Original heading note.</note><note type=\"study\">Original second heading note.</note></title></div>Original body.");
      sword::VerseKey source("John4:1");
      writer.setKey("John4:6"); writer.linkEntry(&source);
      writer.setKey("John4:7"); writer.linkEntry(&source);
    }
    Entry held; unsigned cases = 0;
    {
      Reader reader(root.string());
      for (const auto *module : {"Notes", "ThMLNotes", "OSISBefore", "ThMLBefore"})
        for (bool numbers : {false, true}) for (bool notes : {false, true}) for (bool lexical : {false, true}) {
          EngineOptions engine; engine.footnotes = notes; engine.strongs = lexical;
          StudySession old(reader), enabled(reader);
          for (auto *session : {&old, &enabled}) {
            session->set_engine_options(engine); session->set_note_numbering_enabled(true);
            session->set_lexical_layout_enabled(lexical); session->set_note_numbers(module, numbers);
          }
          enabled.set_heading_note_links_enabled(true);
          const bool filtered = std::string(module).find("Before") != std::string::npos;
          for (const auto *reference : {"John4:1", "John4:2"}) {
            const auto before = old.read_commentary_entry(module, reference);
            const auto after = enabled.read_commentary_entry(module, reference);
            compare(before, after, filtered);
            const auto links = scoped_uris(after.heading_html);
            if (before.heading_html.find("action=showNote") != std::string::npos)
              check(!links.empty(), "supported note-bearing heading gains scoped links under the explicit capability");
            for (const auto &uri : links) {
              const auto target = study_note_link(uri, module);
              const auto note = enabled.study_note(target.module, target.reference, target.identifier);
              check(note.html.find("Original") != std::string::npos && note.html.find("Original body note.") == std::string::npos,
                    "published heading links resolve heading bodies, never a colliding verse-body note");
            }
            held = after; ++cases;
          }
          const auto before = old.read_commentary_chapter(module, "John4:2");
          const auto after = enabled.read_commentary_chapter(module, "John4:2");
          check(before.verse_ordinals == after.verse_ordinals && before.selected_anchor == after.selected_anchor && before.verses.size() == after.verses.size(),
                "sparse chapter keeps source ordinals and selected anchor");
          for (std::size_t i = 0; i < before.verses.size(); ++i) compare(before.verses[i], after.verses[i], filtered);
          check(enabled.history().entries().empty(), "heading presentation and lookup do not visit history");
        }
      EngineOptions engine; engine.footnotes = true;
      StudySession old(reader), enabled(reader), unknown(reader);
      for (auto *session : {&old, &enabled}) { session->set_engine_options(engine); session->set_note_numbering_enabled(true); }
      enabled.set_heading_note_links_enabled(true); unknown.set_heading_note_links_enabled(true);
      check(unknown.read_commentary_entry("Notes", "John4:1").heading_html.find("xiphos-heading-") == std::string::npos,
            "unknown engine/source capability never invents repaired links");
      const auto untouched = old.read_commentary_entry("Notes", "John6:1");
      const auto fallback = enabled.read_commentary_entry("Notes", "John6:1");
      check(untouched.heading_html == fallback.heading_html && fallback.heading_html.find("xiphos-heading-") == std::string::npos,
            "unsupported mixed source keeps exact legacy rendering instead of guessing a heading note association");
      enabled.visit_passage("Text", "Notes", "John4:1"); enabled.visit_passage("Text", "Notes", "John4:2");
      const auto back = enabled.navigate_passage(-1);
      check(back.commentary && !scoped_uris(back.commentary->heading_html).empty() && enabled.history().entries().size() == 2,
            "paired history re-renders current scoped links without extra visits");
      const auto linked = enabled.read_commentary_entry("Notes", "John4:7");
      const auto linked_uri = scoped_uris(linked.heading_html); check(!linked_uri.empty(), "linked commentary retains heading sources");
      const auto target = study_note_link(linked_uri.front(), "Notes");
      const auto linked_note = enabled.study_note(target.module, target.reference, target.identifier);
      check(target.reference == "John 4:7" && linked_note.reference == "John 4:7" &&
            linked_note.html.find("Original heading note.") != std::string::npos,
            "linked source data retains the selected passage in published URI and lookup metadata");
      const auto profile = (root / "heading-profile.conf").string(); enabled.save_module_preferences(profile);
      StudySession restored(reader); restored.import_module_preferences(profile); restored.set_note_numbering_enabled(true);
      check(scoped_uris(restored.read_commentary_entry("Notes", "John4:1").heading_html).empty(), "heading capability is not a saved preference");
      restored.set_heading_note_links_enabled(true);
      check(!scoped_uris(restored.read_commentary_entry("Notes", "John4:1").heading_html).empty(), "explicit restored consumer enables checked shared links");
      enabled.set_heading_note_links_enabled(false);
      compare(old.read_commentary_entry("Notes", "John4:1"), enabled.read_commentary_entry("Notes", "John4:1"), false);
    }
    std::future<StudyPassage> drained;
    {
      StudyWorker worker(root.string()); const auto first = worker.create_tab().get(), other = worker.create_tab().get();
      for (auto tab : {first, other}) { worker.enable_module_preferences(tab).get(); worker.set_note_numbering_enabled(tab, true).get(); }
      worker.set_heading_note_links_enabled(first, true).get();
      const auto first_html = worker.read_passage(first, "Text", "Notes", "John4:1").get().commentary->heading_html;
      check(!scoped_uris(first_html).empty() && scoped_uris(worker.read_passage(other, "Text", "Notes", "John4:1").get().commentary->heading_html).empty(),
            "worker heading capability isolates tabs");
      auto off = worker.set_heading_note_links_enabled(first, false);
      auto old = worker.read_passage(first, "Text", "Notes", "John4:1");
      auto on = worker.set_heading_note_links_enabled(first, true);
      auto repaired = worker.read_passage(first, "Text", "Notes", "John4:1"); off.get(); on.get();
      check(scoped_uris(old.get().commentary->heading_html).empty() && !scoped_uris(repaired.get().commentary->heading_html).empty() && worker.history(first).get().entries.empty(),
            "FIFO settings/reads retain independent heading policies without history visits");
      const auto note = worker.study_note_link(first, "Notes", scoped_uris(first_html).front()).get();
      check(note.html.find("Original heading note.") != std::string::npos, "worker resolves its published heading links");
      worker.close_tab(other).get(); bool closed = false;
      try { worker.set_heading_note_links_enabled(other, true).get(); } catch (const std::invalid_argument &) { closed = true; }
      check(closed, "closed worker tab rejects heading capability");
      drained = worker.read_passage(first, "Text", "Notes", "John4:1");
    }
    check(!scoped_uris(drained.get().commentary->heading_html).empty(), "accepted repaired snapshot survives worker drain");
    {
      xc_worker *opened = nullptr; check(xc_worker_open(root.string().c_str(), &opened) == XC_OK, "open C heading snapshot worker");
      std::unique_ptr<xc_worker, decltype(&xc_worker_close)> worker(opened, xc_worker_close);
      xc_request *request = nullptr;
      auto complete = [&](xc_status status, xc_status expected = XC_OK) {
        std::unique_ptr<xc_request, decltype(&xc_request_release)> done(std::exchange(request, nullptr), xc_request_release);
        check(status == XC_OK && done, "accept C heading capability request");
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (xc_request_poll(done.get()) == XC_PENDING) {
          check(std::chrono::steady_clock::now() < deadline, "C heading snapshot deadline");
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        check(xc_request_poll(done.get()) == expected, "C heading snapshot terminal status"); return done;
      };
      auto done = complete(xc_worker_create_tab(worker.get(), &request)); const auto tab = xc_request_tab(done.get());
      check(xc_worker_set_heading_note_links_enabled(worker.get(), tab, 2, &request) == XC_INVALID_ARGUMENT && !request &&
            xc_worker_set_heading_note_links_enabled(worker.get(), tab, 1, nullptr) == XC_INVALID_ARGUMENT &&
            xc_worker_set_heading_note_links_enabled(nullptr, tab, 1, &request) == XC_INVALID_ARGUMENT && !request,
            "C heading capability rejects strict boolean/null arguments before queueing");
      done = complete(xc_worker_enable_module_preferences(worker.get(), tab, &request));
      done = complete(xc_worker_set_note_numbering_enabled(worker.get(), tab, 1, &request));
      done = complete(xc_worker_set_heading_note_links_enabled(worker.get(), tab, 1, &request));
      done = complete(xc_worker_read_passage(worker.get(), tab, "Text", "Notes", "John4:1", &request));
      std::unique_ptr<xc_passage, decltype(&xc_passage_release)> passage(xc_request_take_passage(done.get()), xc_passage_release);
      check(passage != nullptr, "C repaired passage result");
      const std::string html(xc_passage_commentary_heading_html(passage.get()));
      const auto links = scoped_uris(html); check(!links.empty(), "C passage copies scoped heading HTML");
      done = complete(xc_worker_study_note_link(worker.get(), tab, "Notes", links.front().c_str(), &request));
      std::unique_ptr<xc_note, decltype(&xc_note_release)> note(xc_request_take_note(done.get()), xc_note_release);
      check(note && std::string(xc_note_html(note.get())).find("Original heading note.") != std::string::npos,
            "C note dispatch resolves the published heading URI");
      done = complete(xc_worker_close_tab(worker.get(), tab, &request));
      done = complete(xc_worker_set_heading_note_links_enabled(worker.get(), tab, 0, &request), XC_INVALID_ARGUMENT);
      worker.reset(); done.reset();
      check(std::string(xc_passage_commentary_heading_html(passage.get())) == html && std::string(xc_note_identifier(note.get())).find("xiphos-heading-") == 0,
            "C scoped HTML and note snapshots outlive tab, requests and worker");
    }
    check(!held.note_source.headings.empty(), "raw snapshot ownership survives all sessions and Reader destruction");
    std::cout << "Heading snapshots preserve exact legacy presentation outside scoped IDs: " << cases << " cases\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
