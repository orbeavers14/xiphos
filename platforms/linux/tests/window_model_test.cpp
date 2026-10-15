// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using xiphos::linux_preview::ReaderModel;
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
void settle(ReaderModel &first, ReaderModel &second) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (first.busy() || second.busy() || first.preview_busy() || second.preview_busy()) {
    first.tick(); second.tick(); check(std::chrono::steady_clock::now() < deadline, "shared worker timeout"); std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
int main(int argc, char **argv) {
  xc_worker *worker = nullptr;
  try {
    check(argc == 2 && xc_worker_open(argv[1], &worker) == XC_OK, "one process worker");
    {
      ReaderModel first(*worker), second(*worker); settle(first, second);
      check(first.ready() && second.ready(), "two independent initialized tabs");
      check(first.visit("OptionOther", "", "John 3:16") && second.visit("OptionOther", "", "John 3:16"), "two queued visits"); settle(first, second);
      const auto flags = second.engine_flags(); const auto second_html = second.passage().bible_html;
      check(first.set_filter(XC_ENGINE_STRONGS, !(flags & XC_ENGINE_STRONGS)), "first window filter"); settle(first, second);
      check(second.engine_flags() == flags && second.passage().bible_html == second_html, "filter isolation");
      check(first.visit("OptionOther", "", "John 3:17"), "first window history"); settle(first, second);
      check(first.can_back() && !second.can_back() && second.passage().reference == "John 3:16", "history and passage isolation");
      check(first.set_display(true, 0), "first chapter display"); settle(first, second);
      check(!second.show_chapter(), "display state isolation");
      check(first.set_cross_reference_list(false), "first hover preference"); settle(first, second);
      check(!first.cross_reference_list() && second.cross_reference_list(), "preview preference isolation");
      auto names = first.lexicons(); names[0] = "Unavailable first window";
      check(first.set_lexicons(names), "first lexicons"); settle(first, second);
      check(first.lexicons() != second.lexicons(), "lexicon isolation");
      check(first.visit("OptionOther", "", "John 3:18"), "accepted work before first close");
      check(first.detach() == nullptr, "borrowed worker not transferred or closed");
      check(second.visit("OptionOther", "", "John 3:17"), "second window remains live"); settle(first, second);
      check(second.error().empty() && second.passage().reference == "John 3:17", "remaining window unaffected by close");
      check(second.detach() == nullptr, "last model still leaves worker to host");
    }
    std::thread([worker] { xc_worker_close(worker); }).join(); worker = nullptr;
    std::cout << "Linux shared-worker window isolation checks passed\n"; return 0;
  } catch (const std::exception &error) {
    if (worker) std::thread([worker] { xc_worker_close(worker); }).join();
    std::cerr << error.what() << '\n'; return 1;
  }
}
