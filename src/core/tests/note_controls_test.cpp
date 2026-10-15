// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char **argv) {
  try {
    check(argc == 2, "original-prose note fixture required");
    std::future<Entry> drained;
    {
      StudyWorker worker(argv[1]); const auto first = worker.create_tab().get(), other = worker.create_tab().get();
      EngineOptions options; options.footnotes = true;
      worker.set_engine_options(first, options).get(); worker.set_engine_options(other, options).get();
      worker.set_note_numbering_enabled(first, true).get();
      auto off = worker.set_note_numbers(first, "Text", false);
      auto before = worker.read_entry(first, "Text", "John4:5");
      auto on = worker.set_note_numbers(first, "Text", true);
      auto after = worker.read_entry(first, "Text", "John4:5");
      off.get(); on.get();
      check(before.get().html.find("*n1") == std::string::npos && after.get().html.find("*n2") != std::string::npos,
            "FIFO read/setting submissions observe their own numbering policy");
      check(!worker.read_entry(other, "Text", "John4:5").get().note_source.captured && worker.history(first).get().entries.empty(),
            "worker capability isolates tabs without recording settings as visits");
      worker.close_tab(other).get(); bool failed = false;
      try { worker.set_note_numbers(other, "Text", false).get(); } catch (const std::invalid_argument &) { failed = true; }
      check(failed, "closed worker tab rejects note setting");
      drained = worker.read_entry(first, "Text", "John4:5");
    }
    check(drained.get().html.find("*n2") != std::string::npos, "accepted numbered snapshot survives worker drain");
    xc_worker *opened = nullptr; check(xc_worker_open(argv[1], &opened) == XC_OK, "open C worker");
    std::unique_ptr<xc_worker, decltype(&xc_worker_close)> worker(opened, xc_worker_close);
    xc_request *request = nullptr;
    int step = 0;
    auto complete = [&](xc_status status, xc_status expected = XC_OK) {
      std::unique_ptr<xc_request, decltype(&xc_request_release)> result(std::exchange(request, nullptr), xc_request_release);
      ++step;
      check(status == XC_OK && result, "C accepted request owns a result");
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
      while (xc_request_poll(result.get()) == XC_PENDING) {
        check(std::chrono::steady_clock::now() < deadline, "C note command deadline");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      if (xc_request_poll(result.get()) != expected) throw std::runtime_error("C terminal note command step " + std::to_string(step) + " status " + std::to_string(xc_request_poll(result.get())) + " expected " + std::to_string(expected));
      return result;
    };
    auto done = complete(xc_worker_create_tab(worker.get(), &request)); const auto tab = xc_request_tab(done.get());
    int value = 7, available = 7;
    check(xc_request_note_numbers(done.get(), &value, &available) == XC_INVALID_ARGUMENT && value == 0 && available == 0,
          "wrong-kind C getter clears typed outputs");
    done = complete(xc_worker_module_presentation_options(worker.get(), tab, "Text", &request));
    check(xc_request_note_numbers(done.get(), &value, &available) == XC_OK && value == 1 && available == 0,
          "C value and availability distinguish default On from unsupported consumer");
    check(xc_worker_set_note_numbering_enabled(worker.get(), tab, 2, &request) == XC_INVALID_ARGUMENT && !request &&
          xc_worker_set_note_numbering_enabled(worker.get(), tab, 1, nullptr) == XC_INVALID_ARGUMENT &&
          xc_worker_set_note_numbers(worker.get(), tab, "Text", -1, &request) == XC_INVALID_ARGUMENT && !request &&
          xc_worker_set_note_numbers(worker.get(), tab, "", 1, &request) == XC_INVALID_ARGUMENT && !request &&
          xc_worker_set_note_numbers(worker.get(), tab, nullptr, 1, &request) == XC_INVALID_ARGUMENT && !request &&
          xc_worker_set_note_numbers(nullptr, tab, "Text", 1, &request) == XC_INVALID_ARGUMENT && !request,
          "C strict boolean, null and empty arguments reject before queueing");
    done = complete(xc_worker_set_note_numbers(worker.get(), tab, "Text", 0, &request), XC_INVALID_ARGUMENT);
    value = available = 7;
    check(xc_request_note_numbers(done.get(), &value, &available) == XC_INVALID_ARGUMENT && value == 0 && available == 0,
          "failed command does not expose stale typed state");
    done = complete(xc_worker_set_note_numbering_enabled(worker.get(), tab, 1, &request));
    done = complete(xc_worker_set_note_numbers(worker.get(), tab, "Text", 0, &request), XC_INVALID_ARGUMENT);
    done = complete(xc_worker_set_engine_options(worker.get(), tab, XC_ENGINE_FOOTNOTES, &request));
    check(xc_worker_module_presentation_options(worker.get(), tab, "Text", &request) == XC_OK, "accept typed query");
    value = available = 7;
    check(xc_request_note_numbers(request, &value, &available) == XC_PENDING && value == 0 && available == 0,
          "unpolled query reports pending and clears outputs");
    done = complete(XC_OK);
    check(xc_request_note_numbers(done.get(), &value, &available) == XC_OK && value == 1 && available == 1 &&
          xc_request_note_numbers(done.get(), &value, &available) == XC_OK, "C typed state repeats after completion");
    auto held_state = std::move(done);
    done = complete(xc_worker_set_note_numbers(worker.get(), tab, "Missing", 0, &request), XC_MODULE_NOT_FOUND);
    done = complete(xc_worker_set_note_numbers(worker.get(), tab, "Text", 0, &request));
    done = complete(xc_worker_read_tab_entry(worker.get(), tab, "Text", "John4:5", &request));
    std::unique_ptr<xc_entry, decltype(&xc_entry_release)> plain(xc_request_take_entry(done.get()), xc_entry_release);
    check(plain && std::string(xc_entry_html(plain.get())).find("*n1") == std::string::npos, "C Off refresh carries raw markers");
    char module[] = "Text";
    const auto accepted = xc_worker_set_note_numbers(worker.get(), tab, module, 1, &request); module[0] = 'X';
    done = complete(accepted);
    done = complete(xc_worker_read_tab_chapter(worker.get(), tab, "Text", "John4:5", 0, &request));
    std::unique_ptr<xc_chapter, decltype(&xc_chapter_release)> chapter(xc_request_take_chapter(done.get()), xc_chapter_release);
    check(chapter && std::string(xc_chapter_html(chapter.get())).find("*n3") != std::string::npos,
          "C chapter carries document-wide counters with copied module arguments");
    done = complete(xc_worker_close_tab(worker.get(), tab, &request));
    done = complete(xc_worker_set_note_numbering_enabled(worker.get(), tab, 0, &request), XC_INVALID_ARGUMENT);
    value = available = 7;
    check(xc_request_note_numbers(nullptr, &value, &available) == XC_INVALID_ARGUMENT && !value && !available &&
          xc_request_note_numbers(held_state.get(), nullptr, &available) == XC_INVALID_ARGUMENT &&
          xc_request_note_numbers(held_state.get(), &value, nullptr) == XC_INVALID_ARGUMENT, "C null getters clear supplied outputs");
    worker.reset();
    check(xc_request_note_numbers(held_state.get(), &value, &available) == XC_OK && value == 1 && available == 1 &&
          std::string(xc_entry_html(plain.get())).find("*n1") == std::string::npos &&
          std::string(xc_chapter_html(chapter.get())).find("*n3") != std::string::npos,
          "C typed settings and HTML snapshots outlive later changes, tab close and worker destruction");
    std::cout << "Queued worker/C note controls, typed state, failure isolation and ownership passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
