// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/word_blocks.hpp"
#include <rawtext.h>
#include <rawcom.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
std::string annotation() {
  return "<sync type=\"Strongs\" value=\"G3056\"/>";
}
std::string expected(const std::string &raw) {
  auto layout = layout_lexical_content(raw, true, false, false, false);
  check(layout.applied, "fixture must exercise supported lexical grammar");
  return cleanup_html_content(layout.html, ContentPreference::off, ContentPreference::off).html;
}
int main(int argc, char **argv) {
  try {
    check(argc == 3, "source headings and private lexical fixture roots required");
    const std::filesystem::path source(argv[1]), root(argv[2]);
    std::filesystem::create_directories(root);
    std::filesystem::copy(source, root, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    const auto styled = "<font face=\"Original Lexical Font\">Original styled word</font> " + annotation() + " <img src=\"original.png\" />";
    {
      sword::RawText writer((root / "text").string().c_str());
      writer.setKey("John 4:1"); writer.setEntry(styled.c_str());
      writer.setKey("John 4:2"); writer.setEntry("Original <scripRef passage=\"John 4:1\">passage link</scripRef>.");
      writer.setKey("John 5:1"); writer.setEntry(styled.c_str());
      writer.setKey("John 5:2"); writer.setEntry("Original unsupported <small>ordinary note</small>");
    }
    {
      sword::RawCom writer((root / "thml").string().c_str());
      writer.setKey("John 4:1"); writer.setEntry(("<div class=\"sechead\">" + styled + "</div>" + styled).c_str());
      writer.setKey("John 4:2"); writer.setEntry("Original sparse commentary body.");
      writer.setKey("John 5:1"); writer.setEntry(("<div class=\"sechead\"><small>Unsupported heading</small></div>" + styled).c_str());
      writer.setKey("John 5:2"); writer.setEntry(styled.c_str());
    }
    const auto on = [] { EngineOptions options; options.strongs = true; return options; }();
    Entry held;
    {
      Reader reader(root.string());
      StudySession first(reader), other(reader), unknown(reader);
      first.enable_module_preferences(); other.enable_module_preferences();
      first.set_module_engine_options("Text", on); first.set_module_engine_options("ThMLNotes", on);
      other.set_module_engine_options("Text", on);
      const auto raw = reader.read("Text", "John 4:1", on);
      const auto linked = reader.read("Text", "John 4:2", on).html;
      const auto anchor = linked.find("<a "), anchor_end = linked.find("</a>", anchor);
      check(anchor != std::string::npos && anchor_end != std::string::npos, "SDK fixture emits an actual passage link");
      const auto marker = linked.substr(anchor, anchor_end + 4 - anchor);
      const auto compatible = first.read_entry("Text", "John 4:1");
      check(compatible.html == raw.html && !compatible.lexical.requested && compatible.lexical.css.empty(),
            "existing sessions remain compatible until the consumer opts into matching HTML/CSS");
      unknown.set_lexical_layout_enabled(true);
      check(!unknown.read_entry("Text", "John 4:1").lexical.requested,
            "unconfigured engine state never guesses annotation settings");
      first.set_lexical_layout_enabled(true);
      first.set_image_content("Text", false); first.set_respect_font_faces("Text", false);
      first.set_image_content("ThMLNotes", false); first.set_respect_font_faces("ThMLNotes", false);
      held = first.read_entry("Text", "John 4:1");
      if (!held.lexical.applied) std::cerr << "Layout fallback for SDK fixture: " << raw.html << '\n';
      check(held.lexical.requested && held.lexical.applied && held.lexical.css.find("line-height: 2.7em") != std::string::npos &&
            held.html == expected(raw.html) && held.html.find("Original Lexical Font") == std::string::npos && held.html.find("<img ") == std::string::npos,
            "entry carries matching owned layout before image/font cleanup");
      const auto cleaned_first = cleanup_html_content(raw.html, ContentPreference::off, ContentPreference::off).html;
      const auto chapter = first.read_chapter("Text", "John 4:2");
      check(chapter.lexical.applied && chapter.lexical.css == held.lexical.css && chapter.verses.at(0).html == held.html &&
            chapter.verses.at(1).html.find(marker) != std::string::npos && chapter.selected_anchor == "verse-2" &&
            chapter.verses.at(1).lexical.css == chapter.lexical.css && chapter.html.find("id=\"verse-2\"") != std::string::npos,
            "dense chapter carries one CSS decision with preserved verse anchors and opaque SDK links");
      const auto pair = first.read_passage("Text", "ThMLNotes", "John 4:1");
      const auto raw_notes = reader.read_commentary_entry("ThMLNotes", "John 4:1", true, on);
      check(pair.text.html == held.html && pair.commentary && pair.commentary->lexical.applied &&
            pair.commentary->heading_html == expected(raw_notes.heading_html) && pair.commentary->html == expected(raw_notes.html),
            "paired panes format and clean heading/body with independently matching CSS");
      const auto sparse = first.read_commentary_chapter("ThMLNotes", "John 4:2");
      check(sparse.lexical.applied && sparse.verse_ordinals == std::vector<unsigned>({1, 2}) &&
            sparse.verses.front().heading_html == pair.commentary->heading_html && sparse.selected_anchor == "commentary-verse-2",
            "sparse commentary preserves SDK ordinals and heading layout");
      const auto fallback = first.read_chapter("Text", "John 5:1");
      check(fallback.lexical.requested && !fallback.lexical.applied && fallback.lexical.css.empty() &&
            fallback.verses.at(0).html == cleanup_html_content(reader.read("Text", "John 5:1", on).html, ContentPreference::off, ContentPreference::off).html &&
            !fallback.verses.at(0).lexical.applied && fallback.verses.at(1).html.find("<small>ordinary note</small>") != std::string::npos,
            "unsupported neighbor rolls back the whole document before any annotation CSS can leak");
      const auto fallback_header = first.read_commentary_entry("ThMLNotes", "John 5:1");
      check(fallback_header.lexical.requested && !fallback_header.lexical.applied && fallback_header.lexical.css.empty() &&
            fallback_header.html.find("class=\"word\"") == std::string::npos && fallback_header.heading_html.find("Unsupported heading") != std::string::npos,
            "unsupported heading rolls body and heading back together");
      const auto fallback_sparse = first.read_commentary_chapter("ThMLNotes", "John 5:2");
      check(!fallback_sparse.lexical.applied && fallback_sparse.lexical.css.empty() &&
            fallback_sparse.verses.back().html.find("class=\"word\"") == std::string::npos,
            "commentary chapter fallback also covers supported neighboring bodies");
      check(!other.read_entry("Text", "John 4:1").lexical.requested && first.history().entries().empty(),
            "opt-in and formatting are tab-local and do not visit history");
      const auto visited = first.visit_passage("Text", "ThMLNotes", "John 4:1");
      first.visit_passage("Text", "ThMLNotes", "John 4:2");
      const auto back = first.navigate_passage(-1);
      check(back.text.html == visited.text.html && back.commentary->lexical.applied && first.history().entries().size() == 2,
            "paired history still records only navigation and re-renders current display policy");
      first.set_lexical_layout_enabled(false);
      const auto plain = first.read_entry("Text", "John 4:1");
      check(!plain.lexical.requested && plain.lexical.css.empty() && plain.html == cleaned_first,
            "turning off consumer layout retains current cleanup settings without stale CSS");
      first.set_lexical_layout_enabled(true);
      first.set_module_engine_options("Text", EngineOptions{});
      check(!first.read_entry("Text", "John 4:1").lexical.requested, "disabled annotation options keep plain HTML and no annotation CSS");
      first.set_module_engine_options("Text", on);
      const auto profile = (root / "layout-profile.conf").string();
      first.save_module_preferences(profile);
      StudySession restored(reader); restored.import_module_preferences(profile);
      check(!restored.read_entry("Text", "John 4:1").lexical.requested, "consumer capability is not a saved user preference");
      restored.set_lexical_layout_enabled(true);
      check(restored.read_entry("Text", "John 4:1").html == held.html, "saved engine/content options restore compatible formatted output after consumer opt-in");
    }
    std::future<Entry> accepted;
    {
      StudyWorker worker(root.string());
      const auto first = worker.create_tab().get(), second = worker.create_tab().get();
      worker.set_module_engine_options(first, "Text", on).get();
      worker.set_module_engine_options(second, "Text", on).get();
      worker.set_image_content(first, "Text", false).get(); worker.set_respect_font_faces(first, "Text", false).get();
      worker.set_lexical_layout_enabled(first, true).get();
      check(worker.read_entry(first, "Text", "John 4:1").get().html == held.html &&
            !worker.read_entry(second, "Text", "John 4:1").get().lexical.requested && worker.history(first).get().entries.empty(),
            "FIFO worker layout selection isolates tabs and histories");
      worker.close_tab(second).get();
      bool closed = false;
      try { worker.set_lexical_layout_enabled(second, true).get(); } catch (const std::invalid_argument &) { closed = true; }
      check(closed, "closed-tab layout command fails without disrupting later work");
      accepted = worker.read_entry(first, "Text", "John 4:1");
    }
    const auto drained = accepted.get();
    check(drained.html == held.html && drained.lexical.css == held.lexical.css && drained.lexical.applied,
          "copied HTML and matching CSS survive worker drain and reader destruction");
    std::cout << "Shared lexical entry/chapter/paired snapshots, cleanup order, fallback and worker ownership passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
