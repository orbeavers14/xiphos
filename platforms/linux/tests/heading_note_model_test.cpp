// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::linux_preview;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
void settle(ReaderModel &model) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (model.busy() || model.preview_busy()) {
    model.tick(); check(std::chrono::steady_clock::now() < deadline, "heading model deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
std::string heading_uri(const std::string &html) {
  const auto at = html.find("&value=xiphos-heading-"); check(at != std::string::npos, "Linux receives scoped core heading link");
  const auto start = html.rfind("href=\"", at), end = html.find('"', at);
  check(start != std::string::npos && end != std::string::npos, "bounded heading URI");
  return html.substr(start + 6, end - start - 6);
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "private heading snapshot fixture required");
    ReaderModel model(argv[1]); settle(model);
    check(model.visit("Text", "Notes", "John4:1"), "visit paired heading fixture"); settle(model);
    const auto uri = heading_uri(model.passage().commentary_html); const auto back = model.can_back();
    check(model.follow("Notes", uri, model.passage().reference), "follow published heading link"); settle(model);
    check(model.note().module == "Notes" && model.note().reference == "John 4:1" &&
          model.note().html.find("Original heading note.") != std::string::npos &&
          model.note().html.find("Original body note.") == std::string::npos && model.can_back() == back,
          "Linux heading click resolves shared source body without history visit");
    check(model.hover(uri), "hover published heading link"); settle(model);
    check(model.information().html.find("Original heading note.") != std::string::npos, "Linux hover shares heading note lookup");
    check(model.set_note_numbers(true, false), "hide commentary labels"); settle(model);
    check(model.presentation(true).note_numbers == 0 && heading_uri(model.passage().commentary_html) == uri,
          "Linux display label changes retain heading identity");
    check(model.visit("Text", "Notes", "John4:2"), "move selected passage"); settle(model);
    check(model.follow("Notes", uri, model.passage().reference), "follow retained heading link"); settle(model);
    check(model.passage().reference == "John 4:2" && model.note().reference == "John 4:1" &&
          model.note().html.find("Original heading note.") != std::string::npos,
          "retained Linux link keeps original source passage after later navigation");
    check(model.visit("Text", "Notes", "John4:7"), "visit linked heading entry"); settle(model);
    check(model.follow("Notes", heading_uri(model.passage().commentary_html), model.passage().reference), "follow linked source"); settle(model);
    check(model.note().reference == "John 4:7", "Linux linked source retains selected passage context");
    check(model.visit("Text", "Notes", "John6:1"), "unsupported mixed heading fixture"); settle(model);
    check(model.error().empty() && model.passage().commentary_html.find("xiphos-heading-") == std::string::npos &&
          model.passage().commentary_html.find("Original mixed") != std::string::npos,
          "unsupported source stays readable without guessed heading identity");
    const auto retained = model.passage(); auto *worker = model.detach();
    check(!model.follow("Notes", uri), "closed model rejects heading action");
    std::thread([worker] { xc_worker_close(worker); }).join();
    check(retained.commentary_html.find("Original mixed") != std::string::npos, "Linux copied HTML survives worker shutdown");
    std::cout << "Linux heading click/hover, linked source identity and conservative fallback passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
