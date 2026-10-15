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
