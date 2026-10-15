// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::linux_preview;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
void settle(ReaderModel &model) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (model.busy()) {
    model.tick(); check(std::chrono::steady_clock::now() < deadline, "note model deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "original-prose note fixture required");
    ReaderModel model(argv[1]); settle(model);
    check(model.visit("Text", "ThMLNotes", "John4:5"), "visit paired note fixture"); settle(model);
    check(model.presentation(false).note_numbers == 1 && model.presentation(true).note_numbers == 1 &&
          model.passage().bible_html.find("*n2") != std::string::npos, "Linux model opts into shared default On");
    const auto back = model.can_back();
    check(model.set_note_numbers(false, false), "submit Bible Off");
    check(!model.set_note_numbers(false, true), "busy control cannot submit overlapping UI refresh"); settle(model);
    check(model.presentation(false).note_numbers == 0 && model.passage().bible_html.find("*n1") == std::string::npos &&
          model.presentation(true).note_numbers == 1 && model.can_back() == back, "Linux Off refresh isolates commentary and history");
    check(model.set_note_numbers(true, false), "submit commentary Off"); settle(model);
    check(model.set_note_numbers(false, true), "submit Bible On"); settle(model);
    check(model.set_display(true, XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED), "chapter view"); settle(model);
    check(model.bible_chapter().html.find("*n3") != std::string::npos && model.bible_chapter().anchor == "verse-5" &&
          model.commentary_chapter().html.find("*na") == std::string::npos, "Linux chapter counters and separate commentary policy come from core");
    const auto profile = (std::filesystem::path(argv[1]) / "linux-note-controls.conf").string();
    check(model.reading_profile(profile, true), "save note preferences"); settle(model);
    check(model.set_note_numbers(false, false), "change Bible setting"); settle(model);
    check(model.set_note_numbers(true, true), "change commentary setting"); settle(model);
    check(model.reading_profile(profile, false), "restore note preferences"); settle(model);
    check(model.presentation(false).note_numbers == 1 && model.presentation(true).note_numbers == 0 && model.can_back() == back &&
          model.bible_chapter().html.find("*n3") != std::string::npos, "Linux profile restores applied per-module policy without navigation");
    const auto retained = model.bible_chapter();
    check(model.reading_profile((std::filesystem::path(argv[1]) / "missing/linux-note.conf").string(), false), "submit failed import"); settle(model);
    check(!model.error().empty() && model.presentation(false).note_numbers == 1 && model.bible_chapter().html == retained.html,
          "failed Linux import retains copied policy and snapshot");
    auto *worker = model.detach();
    check(!model.set_note_numbers(false, false), "closed model rejects control");
    std::thread([worker] { xc_worker_close(worker); }).join();
    check(retained.html.find("*n3") != std::string::npos, "copied Linux chapter outlives worker");
    std::cout << "Linux shared note controls, paired policy, profiles and ownership passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
