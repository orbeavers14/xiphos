// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
xc_status wait(xc_request *request) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  xc_status status;
  while ((status = xc_request_poll(request)) == XC_PENDING) {
    check(std::chrono::steady_clock::now() < deadline, "async request timeout");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  check(xc_request_poll(request) == status, "terminal status cached");
  return status;
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture root required");
    xc_worker *worker = nullptr;
    check(xc_worker_open(argv[1], &worker) == XC_OK && worker, "async worker accepted");
    xc_request *request = nullptr;
    check(xc_worker_modules(worker, &request) == XC_OK && request, "catalog request accepted");
    check(xc_request_take_entry(request) == nullptr && xc_request_tab(request) == 0, "wrong result type not transferable");
    check(wait(request) == XC_OK, "catalog ready");
    auto *catalog = xc_request_take_catalog(request);
    check(catalog && xc_catalog_count(catalog) == 1 && std::strcmp(xc_catalog_id(catalog, 0), "ParityTest") == 0, "owned catalog");
    check(xc_request_take_catalog(request) == nullptr, "take exactly once");
    xc_request_release(request);
    check(xc_worker_create_tab(worker, &request) == XC_OK && wait(request) == XC_OK, "create async tab");
    const auto tab = xc_request_tab(request);
    check(tab != 0, "nonzero tab ID");
    xc_request_release(request);
    xc_request *first = nullptr, *second = nullptr, *back = nullptr;
    std::string key = "John 3:16";
    check(xc_worker_visit(worker, tab, "ParityTest", key.c_str(), &first) == XC_OK, "first visit queued");
    key = "John 3:17";
    check(xc_worker_visit(worker, tab, "ParityTest", key.c_str(), &second) == XC_OK, "second visit queued");
    check(xc_worker_navigate(worker, tab, -1, &back) == XC_OK, "back queued");
    check(wait(back) == XC_OK && wait(first) == XC_OK && wait(second) == XC_OK, "FIFO completion");
    auto *entry = xc_request_take_entry(first);
    check(entry && std::strstr(xc_entry_html(entry), "café"), "queued argument ownership");
    auto *restored = xc_request_take_entry(back);
    check(restored && std::strcmp(xc_entry_reference(restored), "John 3:16") == 0, "async back restores first");
    xc_entry_release(restored);
    xc_request_release(first); xc_request_release(second); xc_request_release(back);
    check(xc_worker_history(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "history request");
    auto *history = xc_request_take_history(request);
    check(history && xc_history_count(history) == 2 && !xc_history_can_back(history) && xc_history_can_forward(history), "history snapshot");
    check(std::strcmp(xc_history_module(history), "ParityTest") == 0, "history module snapshot");
    xc_request_release(request);
    check(xc_worker_visit(worker, tab, "Missing", "John 3:16", &request) == XC_OK && wait(request) == XC_MODULE_NOT_FOUND, "queued error");
    xc_request_release(request);
    check(xc_worker_reference_choices(worker, "ParityTest", "John 3:16", &request) == XC_OK && wait(request) == XC_OK, "selector request after error");
    auto *choices = xc_request_take_choices(request);
    check(choices && xc_choices_count(choices, XC_VERSE) == 36, "async choices");
    xc_choices_release(choices); xc_request_release(request);
    check(xc_worker_resolve_link(worker, "ParityTest", "sword:///John+3%3A17", &request) == XC_OK && wait(request) == XC_OK, "link request");
    auto *link = xc_request_take_link(request);
    check(link && std::strcmp(xc_link_reference(link), "John 3:17") == 0, "async link target");
    xc_link_release(link); xc_request_release(request);
    check(xc_worker_navigate_chapter(worker, tab, "ParityTest", "John 21:25", 1, &request) == XC_OK && wait(request) == XC_OK, "chapter move request");
    auto *moved = xc_request_take_entry(request);
    check(moved && std::strcmp(xc_entry_reference(moved), "Acts 1:1") == 0, "async chapter move target");
    xc_entry_release(moved); xc_request_release(request);
    check(xc_worker_clear_history(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "async clear");
    xc_request_release(request);
    // Releasing the observation must NOT imply cancellation of accepted work.
    check(xc_worker_visit(worker, tab, "ParityTest", "John 3:17", &request) == XC_OK, "dropped observation queued");
    xc_request_release(request);
    check(xc_worker_history(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "history after dropped observation");
    auto *after_drop = xc_request_take_history(request);
    check(after_drop && xc_history_count(after_drop) == 1, "dropping request retains operation");
    xc_history_release(after_drop); xc_request_release(request);
    xc_request *chapter_request = nullptr;
    check(xc_worker_read_chapter(worker, "ParityTest", "John 3:16", 0, &chapter_request) == XC_OK, "chapter result queued");
    check(xc_worker_close_tab(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "tab close request");
    xc_request_release(request);
    check(xc_worker_history(worker, tab, &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT, "closed tab asynchronous rejection");
    xc_request_release(request);
    check(xc_worker_read_chapter(worker, "ParityTest", "John 3:16", 8, &request) == XC_INVALID_ARGUMENT && !request, "unknown display flag rejected before queue");
    check(xc_worker_visit(worker, tab, nullptr, "John 3:16", &request) == XC_INVALID_ARGUMENT && !request, "invalid input clears out");
    check(xc_worker_modules(nullptr, &request) == XC_INVALID_ARGUMENT && !request, "null worker");
    check(xc_worker_modules(worker, nullptr) == XC_INVALID_ARGUMENT, "null request output");
    xc_worker_close(worker);
    check(wait(chapter_request) == XC_OK, "request survives worker drain");
    auto *chapter = xc_request_take_chapter(chapter_request);
    check(chapter && xc_chapter_selected_verse(chapter) == 16 && !std::strstr(xc_chapter_html(chapter), "<sup>"), "owned async chapter options");
    xc_request_release(chapter_request);
    check(std::strstr(xc_entry_html(entry), "café") && std::strcmp(xc_catalog_language(catalog, 0), "en") == 0 &&
          xc_history_count(history) == 2, "transferred snapshots survive request/worker close");
    xc_entry_release(entry); xc_chapter_release(chapter); xc_catalog_release(catalog); xc_history_release(history);
    const auto missing = (std::filesystem::path(argv[1]) / "not-installed").string();
    check(xc_worker_open(missing.c_str(), &worker) == XC_OK, "worker starts before root validation finishes");
    check(xc_worker_modules(worker, &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT, "initialization error through request");
    xc_request_release(request); xc_worker_close(worker);
    check(xc_request_poll(nullptr) == XC_INVALID_ARGUMENT && xc_catalog_count(nullptr) == 0 && xc_history_count(nullptr) == 0, "null async snapshots");
    xc_request_release(nullptr); xc_worker_close(nullptr); xc_catalog_release(nullptr); xc_history_release(nullptr);
    std::cout << "Nonblocking C request checks passed\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
