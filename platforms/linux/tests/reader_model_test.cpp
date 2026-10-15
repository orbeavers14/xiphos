// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>
using xiphos::linux_preview::ReaderModel;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
void settle(ReaderModel &model) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (model.busy()) {
    model.tick();
    check(std::chrono::steady_clock::now() < deadline, "request timed out");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
void close(ReaderModel &model) {
  auto *worker = model.detach();
  std::thread([worker] { xc_worker_close(worker); }).join();
}
int main(int argc, char **argv) {
  try {
    check(argc == 3, "fixture and empty root required");
    {
      ReaderModel model(argv[1]); settle(model);
      check(model.ready() && !model.modules().empty(), "catalog and tab ready");
      check(model.visit("Text", "Notes", "Jn 3:16"), "submit paired passage");
      check(!model.visit("Text", "Notes", "John 3:17"), "busy action rejected");
      settle(model);
      check(model.error().empty() && model.passage().reference == "John 3:16", "normalized passage");
      check(model.passage().bible_html.find("Original Bible fixture") != std::string::npos, "copied Bible HTML");
      check(model.passage().commentary_html.find("Notes original commentary") != std::string::npos, "copied commentary HTML");
      check(model.visit("Text", "OtherNotes", "John 3:17"), "second pair"); settle(model);
      check(model.can_back() && !model.can_forward(), "core back state");
      check(model.navigate(-1), "back"); settle(model);
      check(model.passage().commentary == "Notes" && model.passage().reference == "John 3:16", "back restores pair");
      check(model.can_forward(), "forward state");
      check(model.navigate(1), "forward"); settle(model);
      check(model.passage().commentary == "OtherNotes", "forward restores commentary");
      const auto html = model.passage().bible_html;
      check(model.visit("Missing", "Notes", "John 3:16"), "missing module accepted for worker validation"); settle(model);
      check(!model.error().empty() && model.passage().bible_html == html, "failed visit preserves display");
      check(model.navigate(0), "invalid navigation submitted"); settle(model);
      check(!model.error().empty() && model.passage().bible_html == html, "failed navigation preserves display");
      check(model.follow("Text", "https://example.invalid/"), "external URI submitted to core"); settle(model);
      check(!model.error().empty() && model.passage().bible_html == html, "external link rejected");
      check(model.follow("Text", "sword://Text/John%203%3A16"), "passage link submitted"); settle(model);
      check(model.error().empty() && model.passage().reference == "John 3:16", "core routes passage link");
      check(!model.choices(XC_BOOK).empty() && !model.choices(XC_CHAPTER).empty() && !model.choices(XC_VERSE).empty(), "core selector targets copied");
      check(model.set_display(true, XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED), "chapter mode"); settle(model);
      check(model.bible_chapter().html.find("Original Bible fixture") != std::string::npos && !model.bible_chapter().anchor.empty(), "owned chapter and selected anchor");
      check(model.set_commentary_chapter(false), "commentary verse mode"); settle(model);
      check(!model.show_commentary_chapter(), "core commentary mode retained");
      check(model.set_commentary_chapter(true), "commentary chapter mode"); settle(model);
      check(model.commentary_chapter().html.find("commentary") != std::string::npos, "sparse commentary chapter");
      check(model.set_display(true, 0), "display flags"); settle(model);
      check(model.display_flags() == 0, "no display flags");
      check(model.navigate_chapter(1), "next chapter"); settle(model);
      check(model.error().empty() && model.passage().reference == "John 4:1", "chapter advance owned by core");
      check(model.navigate(-1), "back after chapter"); settle(model);
      check(model.passage().reference == "John 3:17", "chapter history retains legacy forward entry");
      check(model.clear_history(), "clear history"); settle(model);
      check(!model.can_back() && !model.can_forward() && model.passage().reference == "John 3:17", "clear leaves display");
      const auto committed_bible = model.passage().reference;
      check(model.lookup_dictionary("Dictionary", "ALPHA"), "dictionary lookup"); settle(model);
      check(model.dictionary().html.find("dictionary pane fixture") != std::string::npos && model.dictionary_keys().size() == 9, "owned dictionary and nearby keys");
      check(model.dictionary_history_count() == 0 && model.passage().reference == committed_bible, "direct lookup changes neither history");
      check(model.step_dictionary(1), "dictionary step"); settle(model);
      check(model.dictionary_history_count() == 0, "key step is not recorded");
      check(model.lookup_dictionary("Dictionary", "ALPHA", true), "recorded dictionary visit"); settle(model);
      check(model.lookup_dictionary("Dictionary", "BETA", true), "second recorded dictionary visit"); settle(model);
      check(model.dictionary_can_back() && model.dictionary_history_count() == 2, "separate dictionary history");
      check(model.navigate_dictionary_history(-1), "dictionary back"); settle(model);
      check(model.dictionary_can_forward(), "dictionary forward available");
      check(model.navigate_dictionary_history(1), "dictionary forward"); settle(model);
      const auto dictionary_html = model.dictionary().html;
      check(model.lookup_dictionary("Missing", "ALPHA"), "missing dictionary worker validation"); settle(model);
      check(!model.error().empty() && model.dictionary().html == dictionary_html, "failed dictionary lookup preserves content");
      check(model.clear_dictionary_history(), "clear dictionary history"); settle(model);
      check(model.dictionary_history_count() == 0 && model.dictionary().html == dictionary_html, "clear dictionary keeps displayed text");
      check(model.close_dictionary() && model.dictionary().module.empty(), "close dictionary presentation");
      check(model.choose_commentary("OtherNotes"), "change commentary without visit"); settle(model);
      check(model.error().empty() && model.passage().commentary == "OtherNotes" && !model.can_back(), "commentary selection preserves history");
      check(model.visit("Notes", "", "John 3:16"), "standalone commentary"); settle(model);
      check(model.error().empty() && model.passage().bible == "Notes" && model.passage().bible_html.find("Notes original commentary") != std::string::npos, "standalone commentary snapshot");
      check(model.follow("Notes", "sword://Notes/John%203%3A17"), "standalone commentary passage link"); settle(model);
      check(model.error().empty() && model.passage().bible == "Notes" && model.passage().reference == "John 3:17", "standalone links remain standalone");
      check(model.navigate_chapter(1), "standalone commentary next chapter"); settle(model);
      check(model.error().empty() && model.passage().reference == "John 4:1", "standalone chapter navigation uses core");
      check(model.visit("Text", "Notes", "John 3:17"), "pending action before close");
      close(model); model.tick();
      check(!model.visit("Text", "Notes", "John 3:16") && !model.ready(), "closed model cannot publish or submit");
    }
    std::filesystem::create_directories(std::filesystem::path(argv[2]) / "mods.d");
    {
      ReaderModel empty(argv[2]); settle(empty);
      check(empty.ready() && empty.modules().empty(), "empty library starts without home discovery");
      check(empty.visit("Missing", "", "John 3:16"), "empty library action"); settle(empty);
      check(!empty.error().empty() && empty.passage().bible.empty(), "empty library error"); close(empty);
    }
    {
      ReaderModel pending(argv[1]); close(pending); pending.tick();
      check(!pending.ready() && !pending.busy(), "close during startup drains safely");
    }
    std::cout << "Linux adapter: catalog, pairs, history, links, failures, busy and close checks passed\n";
    return 0;
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
