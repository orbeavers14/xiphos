// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char **argv) {
  try {
    check(argc == 2, "original heading/content fixture required"); const std::string root(argv[1]);
    Reader reader(root); StudySession first(reader), other(reader);
    first.enable_module_preferences(); other.enable_module_preferences();
    const auto raw = reader.read("Text", "John 3:21");
    check(raw.html.find("<img ") != std::string::npos && raw.html.find("OriginalBibleFont") != std::string::npos, "SDK fixture exposes image/font content");
    const auto discovered = first.read_entry("Text", "John 3:21");
    check(discovered.html == raw.html && first.module_presentation_options("Text").image_content == ContentPreference::on &&
          first.module_presentation_options("Text").respect_font_faces == ContentPreference::on,
          "unknown content is retained and discovered in copied per-module settings");
    first.set_image_content("Text", false); first.set_respect_font_faces("Text", false);
    const auto hidden = first.read_entry("Text", "John 3:21");
    check(hidden.html == cleanup_html_content(raw.html, ContentPreference::off, ContentPreference::off).html && hidden.html.size() == raw.html.size() &&
          hidden.html.find("Original styled Bible body") != std::string::npos && hidden.html.find("<img ") == std::string::npos &&
          hidden.html.find("OriginalBibleFont") == std::string::npos, "Bible snapshots use shared cleanup while preserving body and byte length");
    const auto bible_chapter = first.read_chapter("Text", "John 3:21");
    check(bible_chapter.verse_ordinals.size() == bible_chapter.verses.size() && bible_chapter.verse_ordinals.at(20) == 21 &&
          bible_chapter.verses.at(20).html == hidden.html && bible_chapter.html.find("id=\"verse-21\"") != std::string::npos,
          "dense chapter retains copied SDK positions and cleaned anchor content");
    check(other.module_presentation_options("Text").image_content == ContentPreference::unknown &&
          other.read_entry("Text", "John 3:21").html == raw.html && reader.read("Text", "John 3:21").html == raw.html,
          "discovery/settings are tab-local and ordinary Reader output remains unchanged");
    const auto styled = first.read_commentary_entry("ThMLNotes", "John 3:21");
    check(styled.heading_html.find("OriginalHeadingFont") != std::string::npos && styled.html.find("OriginalBodyFont") != std::string::npos &&
          styled.heading_html.find("<img ") != std::string::npos && first.module_presentation_options("ThMLNotes").respect_font_faces == ContentPreference::on,
          "commentary discovery sees headings before body");
    first.set_image_content("ThMLNotes", false); first.set_respect_font_faces("ThMLNotes", false);
    const auto pair = first.read_passage("Text", "ThMLNotes", "John 3:21");
    check(pair.text.html == hidden.html && pair.commentary->heading_html == cleanup_html_content(styled.heading_html, ContentPreference::off, ContentPreference::off).html &&
          pair.commentary->html == cleanup_html_content(styled.html, ContentPreference::off, ContentPreference::off).html,
          "paired snapshots clean both bodies and commentary headings consistently");
    const auto chapter = first.read_commentary_chapter("ThMLNotes", "John 3:17");
    check(chapter.verse_ordinals.size() == chapter.verses.size() && chapter.selected_anchor == "commentary-verse-17" &&
          chapter.html.find("id=\"commentary-verse-17\"") != std::string::npos && chapter.html.find("<img ") == std::string::npos &&
          chapter.html.find("OriginalHeadingFont") == std::string::npos && chapter.html.find("Original styled heading") != std::string::npos,
          "sparse cleaned chapter preserves selected/missing anchors and content");
    unsigned shared_headers = 0;
    for (const auto &entry : chapter.verses) if (entry.heading_html.find("Original ThML heading café") != std::string::npos) ++shared_headers;
    check(shared_headers == 1 && first.history().entries().empty(), "linked headings render once and content commands do not visit history");
    const auto malformed = first.read_entry("Text", "John 3:22");
    check(malformed.cleanup_diagnostics.size() == 1 && malformed.cleanup_diagnostics.front().kind == ContentCleanupKind::font_face &&
          malformed.cleanup_diagnostics.front().suffix.find("unfinished") != std::string::npos, "owned entry retains malformed cleanup diagnostics");
    const auto profile = std::filesystem::path(root) / "content-snapshots.conf";
    { std::ofstream file(profile); file << "[Text]\nCustom=keep\n[Uninstalled]\nImage Content=On\n"; }
    first.save_module_preferences(profile.string());
    StudySession restored(reader); restored.import_module_preferences(profile.string());
    check(restored.read_entry("Text", "John 3:21").html == hidden.html &&
          restored.read_commentary_entry("ThMLNotes", "John 3:21").heading_html == pair.commentary->heading_html &&
          read_module_option_values(profile.string()).at("Uninstalled").at("Image Content") == "On", "cleaned presentation round trips through explicit save/import");
    auto worker = std::make_unique<StudyWorker>(root); const auto tab = worker->create_tab().get(), second = worker->create_tab().get();
    worker->set_image_content(tab, "Text", false).get(); worker->set_respect_font_faces(tab, "Text", false).get();
    check(worker->read_entry(tab, "Text", "John 3:21").get().html == hidden.html &&
          worker->read_entry(second, "Text", "John 3:21").get().html == raw.html && worker->history(tab).get().entries.empty(), "FIFO content setters isolate tabs without history changes");
    auto accepted = worker->read_entry(tab, "Text", "John 3:22"); worker.reset();
    const auto owned = accepted.get();
    check(owned.cleanup_diagnostics.front().suffix == malformed.cleanup_diagnostics.front().suffix && owned.html == malformed.html,
          "owned cleaned result/diagnostics survive worker drain");
    std::cout << "Content snapshot/discovery checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
