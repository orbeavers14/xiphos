// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char **argv) {
  try {
    check(argc == 2, "heading fixture required"); const std::string root = argv[1];
    Reader reader(root); StudySession session(reader); session.enable_module_preferences();
    check(reader.read("Notes", "John 3:16").heading_html.empty(), "existing body-only API remains compatible");
    const auto entry = session.read_commentary_entry("Notes", "John 3:16");
    check(entry.heading_html.find("Original heading") != std::string::npos && entry.heading_html.find("Second heading") != std::string::npos &&
          entry.html.find("Original entry body") != std::string::npos && entry.html.find("Original heading") == std::string::npos,
          "commentary owns headings separately from body");
    const auto chapter = session.read_commentary_chapter("Notes", "John 3:16");
    check(chapter.html.find("Original heading") < chapter.html.find("id=\"commentary-verse-16\"") &&
          chapter.html.find("id=\"commentary-verse-16\"") < chapter.html.find("Original entry body"), "header precedes selection separator/body in source order");
    check(chapter.html.find("<tr class=\"xc-heading\"><td colspan=\"2\">") != std::string::npos,
          "numbered headings use a complete native table row");
    const auto pair = session.read_passage("Text", "Notes", "John 3:16");
    check(pair.commentary->heading_html == entry.heading_html && session.history().entries().empty(), "paired no-record commentary carries headings");
    const auto heading_only = session.read_commentary_entry("Notes", "John 3:19");
    check(heading_only.html.empty() && heading_only.heading_html.find("Original heading-only") != std::string::npos, "heading-only entry retains presentation content");
    session.set_headings("Notes", false);
    const auto hidden = session.read_commentary_chapter("Notes", "John 3:16");
    check(hidden.html.find("Original heading") == std::string::npos && hidden.html.find("xc-heading") == std::string::npos &&
          hidden.html.find("Original entry body") != std::string::npos, "heading preference hides titles and retains bodies");
    check(session.read_commentary_entry("Notes", "John 3:16").heading_html.empty(), "verse presentation obeys same heading preference");
    const auto profile = std::filesystem::path(root) / "heading-modops.conf";
    { std::ofstream file(profile); file << "[Notes]\nHeadings=On\nCommentary by Chapter=Off\nCustom=café\n[Uninstalled]\nHeadings=Off\n"; }
    session.save_module_preferences(profile.string());
    check(read_module_option_values(profile.string()).at("Notes").at("Headings") == "Off" &&
          read_module_option_values(profile.string()).at("Uninstalled").at("Headings") == "Off", "known headings saved while unavailable settings retained");
    session.set_headings("Notes", true); session.import_module_preferences(profile.string());
    check(!session.module_presentation_options("Notes").headings && !session.module_presentation_options("Text").commentary_chapter_available,
          "combined import restores headings with copied capability metadata");
    StudyWorker worker(root); const auto tab = worker.create_tab().get(), other = worker.create_tab().get();
    worker.enable_module_preferences(tab).get();
    worker.set_headings(tab, "Notes", false).get();
    check(!worker.module_presentation_options(tab, "Notes").get().headings && worker.module_presentation_options(other, "Notes").get().headings,
          "heading settings isolated across tabs");
    const auto queued = worker.read_commentary_chapter(tab, "Notes", "John 3:16").get();
    check(queued.html.find("Original heading") == std::string::npos && worker.history(tab).get().entries.empty(), "queued heading preference used without Bible visit");
    std::cout << "Commentary heading snapshot checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
