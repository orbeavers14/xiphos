// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include "reader_document.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::linux_preview;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
void settle(ReaderModel &m, bool success = true) {
  const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(20);
  do { m.tick(); check(std::chrono::steady_clock::now() < end, "Nonblocking model deadline"); std::this_thread::sleep_for(std::chrono::milliseconds(1)); } while (m.busy());
  if (success && !m.error().empty()) throw std::runtime_error(m.error());
}
int main(int argc, char **argv) { try {
  check(argc == 5, "Book, attribute search, output and image fixtures required");
  {
    ReaderModel m(argv[1]); settle(m); check(m.ready(), "Book model ready");
    check(m.module("StyledBook") && m.module("StyledBook")->book, "Copied runtime book capability");
    check(m.choose_book("StyledBook"), "Select general book"); settle(m);
    check(m.book().module == "StyledBook" && !m.book_contents().nodes.empty(), "Selection publishes document and root contents");
    check(m.read_book_key("/Styled"), "Read canonical path"); settle(m);
    check(m.book().html.find("Original") != std::string::npos && m.book().nodes[0].path == "/Styled", "Configured DisplayLevel document owns path/text");
    const auto held = m.book();
    check(m.book_option(0, false), "Disable book images"); settle(m);
    check(m.book_option(1, false), "Disable book fonts"); settle(m);
    check(m.book_option(2, true), "Enable book spacing"); settle(m);
    check(m.book().images == 0 && m.book().fonts == 0 && m.book().spacing == 1 && m.book().html.find("<img ") == std::string::npos && m.book().css.find("line-height") != std::string::npos, "Core refresh applies book options and matching CSS");
    check(m.browse_book(m.book().nodes[0].offset), "Browse committed section"); settle(m);
    check(!m.book_contents().nodes.empty() && m.book_contents().nodes[0].path == "/Styled/Child", "Core child order/source copied");
    check(m.navigate_book(XC_BOOK_FIRST_CHILD_DIRECTION), "Navigate child"); settle(m);
    check(m.book().nodes[0].path == "/Styled/Child", "Core owns book navigation");
    check(m.navigate_book(XC_BOOK_PARENT_DIRECTION), "Navigate parent"); settle(m);
    check(m.read_book_key("/Missing"), "Queue missing source"); settle(m, false);
    check(!m.error().empty() && m.book().nodes[0].path == "/Styled", "Failed book read retains committed document");
    check(!m.can_back() && !m.dictionary_can_back(), "Books do not add Bible/dictionary history");
    check(document("x").find("img-src 'none'") != std::string::npos && document("x", "", true).find("img-src data:") != std::string::npos, "Only resource-prepared book documents allow data images");
    check(m.close_book() && m.book().module.empty() && held.html.find("Original") != std::string::npos, "Close releases presentation; copied values survive");
  }
  {
    xc_worker *worker = nullptr; check(xc_worker_open(argv[2], &worker) == XC_OK, "Open shared process worker");
    {
      ReaderModel m(*worker), other(*worker); settle(m); settle(other);
      check(m.visit("Text", "Commentary", "John1:1"), "Seed paired reading"); settle(m);
      const auto before = m.passage();
      SearchInput q; q.modules = {"Text", "Missing", "Commentary", "Text"}; q.expression = "Original";
      check(m.search(q), "Submit ordered multi-module request"); settle(m);
      const auto first = m.search_results();
      check(first.groups.size() == 4 && first.groups[1].status != XC_OK && first.hits.size() == 12, "Failure does not lose later groups or duplicate requests");
      check(first.hits[4].module == "Commentary" && first.hits[4].group == 2 && first.hits[8].group == 3, "Authoritative group/row/module identity retained");
      check(m.passage().reference == before.reference && other.search_results().generation.id == 0, "Search is history neutral and per-window");
      check(m.preview_search_hit(5), "Preview commentary hit"); settle(m);
      check(m.search_preview().module == "Commentary" && m.passage().reference == before.reference, "Preview preserves source without reading visit");
      check(m.activate_search_hit(5), "Activate commentary hit"); settle(m);
      check(m.passage().bible == "Text" && m.passage().commentary == "Commentary" && m.passage().reference == first.hits[5].reference, "Hit activation follows shared commentary routing");
      check(m.export_search(false, false, {4, 0, 4}, true, true, false, false), "Export explicit rows with duplicates"); settle(m);
      check(m.search_export().disposition == XC_SEARCH_EXPORT_OK && !m.search_export().text.empty(), "Owned selected export complete");
      check(m.export_search(false, true, {}, true, true, false, false), "Request all from incomplete batch"); settle(m, false);
      check(m.search_export().disposition == XC_SEARCH_EXPORT_SOURCE_INCOMPLETE && m.search_export().text.empty(), "Incomplete source refusal never publishes partial payload");
      check(m.export_search(true, true, {}, true, true, false, true), "Explicit partial HTML export with disclosure"); settle(m);
      check(m.search_export().disposition == XC_SEARCH_EXPORT_OK && m.search_export().html.find("<!DOCTYPE") != std::string::npos && !m.search_export().text.empty(), "HTML and rich clipboard companion come from same core job");
      q.modules = {"Text"}; q.expression = "G140"; q.mode = XC_SEARCH_LEMMA;
      check(m.search(q), "Attribute search"); settle(m);
      check(m.search_results().hits.size() == 2, "Exact lemma semantics from shared engine");
      check(m.reset_search_annotations(), "Reset only search options"); settle(m);
      check(m.search_results().hits.size() == 2, "Reset retains prior results");
      q.mode = XC_SEARCH_PHRASE; q.expression = "Original"; q.scope = XC_SEARCH_SCOPE_LAST;
      check(m.search(q), "Search within complete last scope"); settle(m);
      check(m.search_results().hits.size() == 2, "Last scope comes from core tab state");
      check(m.clear_search_scope(), "Clear last scope"); settle(m); check(!m.search_results().scope.present, "Copied clear result");
      auto output = std::filesystem::path(argv[3]); std::filesystem::create_directories(output);
      auto profile = output / "lists.xml";
      { std::ofstream f(profile); f << "<Xiphos><modlists><modlist label=\"Pair\" list=\"Text,Commentary,Text\"/><modlist label=\"Pair\" list=\"Shadow\"/><modlist label=\"Missing\"/><modlist label=\"Empty\" list=\"\"/></modlists><ranges><range label=\"Small\" list=\"John1:1-2\"/></ranges></Xiphos>"; }
      check(m.import_search_lists(profile.string()), "Import explicit saved lists"); settle(m);
      check(m.saved_lists(false).size() == 4 && m.saved_lists(false)[1].effective == 0 && !m.saved_lists(false)[2].value && m.saved_lists(false)[3].value == std::make_optional(std::string()) && m.saved_lists(true).size() == 1, "Shared first-label and absent/empty values preserved");
      check(m.parse_search_modules("Text, Commentary, Text"), "Shared module-list tokenization"); settle(m);
      check(m.parsed_search_modules() == std::vector<std::string>({"Text", " Commentary", " Text"}), "Literal spaces and duplicates retained by core tokenizer");
      check(m.search(q), "Pending work before model close");
      m.detach(); check(!m.search(q), "Closed model rejects submissions");
      check(other.search(q), "Second window remains usable"); settle(other);
    }
    std::thread([worker] { xc_worker_close(worker); }).join();
  }
  {
    const auto root = std::filesystem::path(argv[4]) / std::filesystem::u8path("library %20 café");
    ReaderModel m(root.string().c_str()); settle(m);
    check(m.visit("OSISText", "", "John1:2"), "Read image-bearing passage"); settle(m);
    const auto html = m.passage().bible_html;
    const auto begin = html.find("href=\""); check(begin != std::string::npos, "Generated typed image command");
    const auto end = html.find('"', begin + 6); check(end != std::string::npos, "Complete image command attribute");
    const auto uri = html.substr(begin + 6, end - begin - 6);
    check(m.follow("OSISText", uri, m.passage().reference), "Follow core image command"); settle(m);
    check(m.image().module == "OSISText" && std::string(m.image().bytes.begin(), m.image().bytes.end()) == std::string("Original\0image", 14) + char(0xff), "Linux copies actual-source binary bytes, including embedded NUL, before releasing snapshot");
    const auto held = m.image();
    check(m.follow("OSISText", "passagestudy.jsp?action=showImage&value=..%2Foutside.png", m.passage().reference), "Submit invalid image source"); settle(m, false);
    check(!m.error().empty() && m.image().revision == held.revision && m.image().bytes == held.bytes, "Rejected resource preserves prior image");
  }
  std::cout << "Linux books, images, ordered search, source activation, generation export and shared-window lifecycle passed\n";
} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; } }
