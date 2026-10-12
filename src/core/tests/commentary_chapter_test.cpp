// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include <swbuf.h>
#include <glib.h>
#include "legacy_commentary_anchors.inc"
#include "legacy_commentary_anchors_actual.inc"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char **argv) {
  try {
    check(argc == 2, "commentary fixture required");
    Reader reader(argv[1]); StudySession session(reader);
    EngineOptions options; options.footnotes = true; options.words_in_red = true;
    session.set_module_engine_options("Notes", options);
    for (const auto verse : {1, 16, 17, 18, 19, 20, 25, 36}) {
      const auto reference = "John 3:" + std::to_string(verse);
      const auto chapter = session.read_commentary_chapter("Notes", reference);
      check(chapter.reference == reference && chapter.title == "John 3" && chapter.selected_verse == static_cast<size_t>(verse), "owned selected commentary chapter context");
      check(chapter.verses.size() == 3 && chapter.verses[1].reference == "John 3:16" && chapter.verses[2].reference == "John 3:19", "sparse/linked entries rendered once");
      const auto anchor = "id=\"commentary-verse-" + std::to_string(verse) + "\"";
      check(chapter.html.find(anchor) != std::string::npos && chapter.html.find(anchor, chapter.html.find(anchor) + 1) == std::string::npos,
            "selected/missing/trailing anchor placed exactly once");
      const auto frozen = frozen_commentary_anchors({1, 16, 19}, verse);
      const auto actual = actual_commentary_anchors({1, 16, 19}, verse);
      check(frozen == actual, "current selected/missing anchor statements agree with frozen source");
      const auto old_anchor = frozen.find("name=\"" + std::to_string(verse) + "\"");
      const auto new_anchor = chapter.html.find(anchor);
      check(old_anchor != std::string::npos, "legacy selected anchor exists");
      for (const auto boundary : {1, 16, 19}) {
        const auto old_entry = frozen.find("<entry verse=\"" + std::to_string(boundary) + "\"");
        const auto new_entry = chapter.html.find("<tr id=\"commentary-verse-" + std::to_string(boundary) + "\"");
        check((old_anchor < old_entry) == (new_anchor <= new_entry), "selected/missing anchor retains legacy section boundary");
      }
      check(chapter.html.find("<table") != std::string::npos && chapter.html.find("sword:///John%203:19") != std::string::npos,
            "numbered commentary section links");
      check(chapter.html.find("wordsOfJesus") != std::string::npos && chapter.html.find("action=showNote") != std::string::npos,
            "module engine options apply to each commentary section");
      const auto plain = session.read_commentary_chapter("Notes", reference, {false, false});
      check(plain.html.find("<table") == std::string::npos && plain.html.find("xc-current") == std::string::npos &&
            plain.html.find(anchor) != std::string::npos, "unnumbered commentary retains selected/missing anchor");
    }
    check(session.history().entries().empty(), "commentary chapter read does not record a Bible visit");
    options.footnotes = false; options.words_in_red = false;
    session.set_module_engine_options("Notes", options);
    const auto off = session.read_commentary_chapter("Notes", "John 3:19");
    check(off.html.find("action=showNote") == std::string::npos && off.html.find("wordsOfJesus") == std::string::npos,
          "commentary preference refresh changes filters without history");
    const auto next = session.read_commentary_chapter("Notes", "John 4:1");
    check(next.verses.size() == 2 && next.verses.back().reference == "John 4:2" && next.html.find("Original linked") == std::string::npos,
          "iterator stays within chapter boundary");
    Chapter invalid; invalid.verses.push_back({"<&\"", "Original body"});
    try { display_commentary_chapter(invalid, {}, {}); throw std::runtime_error("mismatched positions accepted"); }
    catch (const std::invalid_argument &) {}
    invalid.selected_verse = 1; invalid.selected_anchor = "commentary-verse-1";
    check(display_commentary_chapter(invalid, {1}, {}).find("&lt;&amp;&quot;") != std::string::npos, "section context attributes escaped");
    StudyWorker worker(argv[1]); const auto tab = worker.create_tab().get();
    worker.set_module_engine_options(tab, "Notes", options).get();
    auto queued = worker.read_commentary_chapter(tab, "Notes", "John 3:17");
    auto after = worker.history(tab);
    check(queued.get().selected_anchor == "commentary-verse-17" && after.get().entries.empty(), "FIFO no-record commentary chapter snapshot");
    std::cout << "Commentary chapter service checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
