// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char **argv) {
  try {
    check(argc == 2, "heading fixture required"); const std::string root = argv[1];
    Reader reader(root); StudySession session(reader);
    const auto original_body = reader.read("Notes", "John 3:16").html;
    check(reader.read_commentary_entry("Notes", "John 3:16").heading_html.find("Original heading") != std::string::npos,
          "direct commentary headings work without an engine preference profile");
    check(reader.read("Notes", "John 3:16").html == original_body, "direct heading scope restores SDK defaults");
    session.enable_module_preferences();
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
    const auto thml = session.read_commentary_entry("ThMLNotes", "John 3:16");
    check(thml.heading_html.find("Original ThML heading") != std::string::npos && thml.heading_html.find("Second ThML heading") != std::string::npos &&
          thml.html.find("Original ThML entry body") != std::string::npos && thml.html.find("Original ThML heading") == std::string::npos,
          "ThML commentary owns two headings separately from body");
    check(session.read_commentary_entry("ThMLNotes", "John 3:17").heading_html == thml.heading_html, "ThML linked verse headings agree");
    const auto sparse_thml = session.read_commentary_chapter("ThMLNotes", "John 3:17");
    unsigned shared_sections = 0;
    for (const auto &verse : sparse_thml.verses) if (verse.heading_html == thml.heading_html) ++shared_sections;
    check(shared_sections == 1 && sparse_thml.selected_anchor == "commentary-verse-17", "ThML linked chapter header rendered once with missing-selection anchor");
    session.set_headings("ThMLNotes", false);
    check(session.read_commentary_entry("ThMLNotes", "John 3:16").heading_html.empty() &&
          session.read_commentary_entry("ThMLNotes", "John 3:16").html == thml.html,
          "ThML heading controls preserve body");
    session.set_headings("ThMLNotes", true);
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
    const auto linked_header = worker.read_passage(tab, "Text", "ThMLNotes", "John 3:20").get().commentary->heading_html;
    const auto href = linked_header.find("href=\""); check(href != std::string::npos, "owned heading contains SDK passage link");
    const auto href_end = linked_header.find('"', href + 6);
    const auto uri = linked_header.substr(href + 6, href_end - href - 6);
    const auto link = worker.follow_pane_link_at(tab, "Text", "ThMLNotes", "John 3:20", uri).get();
    check(std::holds_alternative<ReferenceList>(link), "SDK heading link is a reference preview command");
    const auto &references = std::get<ReferenceList>(link);
    check(references.module == "Text" && references.entries.size() == 1 && references.entries.front().reference == "John 3:17" &&
          references.entries.front().html == reader.read("Text", "John 3:17").html &&
          worker.history(tab).get().entries.empty(), "module-less heading reference uses retained main Bible default without recording a visit");
    const auto queued = worker.read_commentary_chapter(tab, "Notes", "John 3:16").get();
    check(queued.html.find("Original heading") == std::string::npos && worker.history(tab).get().entries.empty(), "queued heading preference used without Bible visit");
    auto wait = [](xc_request *r) {
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      xc_status status;
      while ((status = xc_request_poll(r)) == XC_PENDING) {
        check(std::chrono::steady_clock::now() < deadline, "heading request timeout");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      return status;
    };
    xc_worker *bridge = nullptr; xc_request *request = nullptr;
    check(xc_worker_open(root.c_str(), &bridge) == XC_OK, "C heading worker opens");
    check(xc_worker_create_tab(bridge, &request) == XC_OK && wait(request) == XC_OK, "C heading tab ready");
    check(xc_request_headings(request) == -1, "heading scalar rejects wrong result kind");
    const auto ctab = xc_request_tab(request); xc_request_release(request);
    check(xc_worker_module_presentation_options(bridge, ctab, "Notes", &request) == XC_OK &&
          xc_request_headings(request) == -1 && wait(request) == XC_OK, "heading scalar waits for polling");
    check(xc_request_headings(request) == 1 && xc_request_headings(request) == 1, "ready heading scalar can repeat");
    xc_request_release(request);
    check(xc_worker_enable_module_preferences(bridge, ctab, &request) == XC_OK && wait(request) == XC_OK, "C module profile enabled");
    xc_request_release(request);
    check(xc_worker_read_passage(bridge, ctab, "Text", "Notes", "John 3:16", &request) == XC_OK && wait(request) == XC_OK, "C pair has owned headings");
    auto *owned = xc_request_take_passage(request);
    check(owned && !xc_request_take_passage(request) && std::string(xc_passage_commentary_heading_html(owned)) == entry.heading_html,
          "pair transfers headings once"); xc_request_release(request);
    check(xc_worker_read_tab_commentary_chapter(bridge, ctab, "Notes", "John 3:16", 0, &request) == XC_OK && wait(request) == XC_OK, "C chapter ready");
    auto *owned_chapter = xc_request_take_chapter(request); xc_request_release(request);
    bool found = false;
    for (size_t i = 0; i < xc_chapter_count(owned_chapter); ++i)
      found |= std::string(xc_chapter_verse_heading_html(owned_chapter, i)).find("Original heading") != std::string::npos;
    check(found && !xc_chapter_verse_heading_html(owned_chapter, xc_chapter_count(owned_chapter)), "chapter heading bounds");
    check(xc_worker_set_headings(bridge, ctab, "Notes", 2, &request) == XC_INVALID_ARGUMENT && !request, "invalid heading boolean rejected");
    check(xc_worker_set_headings(bridge, ctab, "Notes", 0, &request) == XC_OK, "heading mutation queued");
    xc_request_release(request);
    check(xc_worker_read_passage(bridge, ctab, "Text", "Notes", "John 3:16", &request) == XC_OK, "FIFO refreshed headings");
    xc_worker_close(bridge);
    check(wait(request) == XC_OK, "worker drains dropped heading edit");
    auto *off = xc_request_take_passage(request); xc_request_release(request);
    check(std::string(xc_passage_commentary_heading_html(off)).empty() &&
          std::string(xc_passage_commentary_html(off)) == entry.html &&
          std::string(xc_passage_commentary_heading_html(owned)) == entry.heading_html, "owned headings survive worker close and later preference changes");
    xc_passage_release(off); xc_passage_release(owned); xc_chapter_release(owned_chapter);
    check(!xc_entry_heading_html(nullptr) && !xc_passage_commentary_heading_html(nullptr) &&
          !xc_chapter_verse_heading_html(nullptr, 0) && xc_request_headings(nullptr) == -1, "null heading guards");
    std::cout << "Commentary heading snapshot checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
