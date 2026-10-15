// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/lexical_style.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <memory>
#include <utility>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char **argv) {
  try {
    check(argc == 2, "private lexical fixture required");
    const std::string root(argv[1]);
    {
      Reader reader(root); StudySession session(reader), other(reader);
      check(!session.module_presentation_options("Text").double_space, "missing spacing defaults off");
      session.set_double_space("Text", true);
      const auto compatible = session.read_entry("Text", "John4:1");
      check(compatible.lexical.css.empty(), "existing non-CSS consumers remain compatible");
      session.set_lexical_layout_enabled(true);
      const auto spaced = session.read_entry("Text", "John4:1");
      const auto css = lexical_block_style(false, false, false, true);
      check(spaced.html == compatible.html && spaced.lexical.css == css && !spaced.lexical.requested && !spaced.lexical.applied,
            "unconfigured engine still receives spacing-only CSS without word rendering");
      for (unsigned flags = 0; flags < 8; ++flags) {
        EngineOptions engine; engine.strongs = flags & 1; engine.lemmas = flags & 2; engine.morphs = flags & 4;
        session.set_module_engine_options("Text", engine);
        const auto entry = session.read_entry("Text", "John4:1");
        const auto chapter = session.read_chapter("Text", "John4:2");
        check(entry.lexical.css == lexical_block_style(engine.strongs, engine.lemmas, engine.morphs, true) &&
              chapter.lexical.css == entry.lexical.css && chapter.verses[0].lexical.css == entry.lexical.css,
              "annotations retain legacy spacing precedence in entry/chapter/verse CSS");
        const auto unsupported = session.read_entry("Text", "John6:1");
        check(!unsupported.lexical.applied && unsupported.lexical.css == css &&
              unsupported.html.find("class=\"word\"") == std::string::npos,
              "unsupported word nesting retains spacing CSS only");
      }
      session.set_module_engine_options("Text", EngineOptions{});
      const auto visited = session.visit_passage("Text", "ThMLNotes", "John4:1");
      check(visited.text.lexical.css == css && visited.commentary && visited.commentary->lexical.css.empty(),
            "paired modules retain independent spacing preferences");
      const auto history_size = session.history().entries().size();
      session.set_double_space("Text", false);
      const auto normal = session.read_entry("Text", "John4:1");
      check(normal.html == visited.text.html && normal.lexical.css.empty() && session.history().entries().size() == history_size &&
            !other.module_presentation_options("Text").double_space, "CSS-only toggle retains HTML/history and other tab");
      session.set_double_space("Text", true);
      const auto profile = (std::filesystem::path(root) / "double-space-profile.conf").string();
      { std::ofstream file(profile); file << "[Text]\nCustom=retain\n[Uninstalled]\nDoublespace=On\n"; }
      session.enable_module_preferences();
      session.save_module_preferences(profile);
      other.import_module_preferences(profile); other.set_lexical_layout_enabled(true);
      check(other.module_presentation_options("Text").double_space && other.read_entry("Text", "John4:1").lexical.css == css,
            "saved module spacing restores in another opted-in tab");
      bool missing = false;
      try { other.set_double_space("Missing", true); } catch (const std::out_of_range &) { missing = true; }
      check(missing && other.module_presentation_options("Text").double_space, "missing module does not mutate preferences");
    }
    {
      StudyWorker worker(root); const auto first = worker.create_tab().get(), second = worker.create_tab().get();
      worker.set_lexical_layout_enabled(first, true).get();
      worker.set_double_space(first, "Text", true).get();
      check(worker.module_presentation_options(first, "Text").get().double_space &&
            !worker.module_presentation_options(second, "Text").get().double_space &&
            !worker.read_entry(first, "Text", "John4:1").get().lexical.css.empty() && worker.history(first).get().entries.empty(),
            "worker queues per-module spacing with copied state and no visit");
      worker.close_tab(first).get(); bool closed = false;
      try { worker.set_double_space(first, "Text", false).get(); } catch (const std::invalid_argument &) { closed = true; }
      check(closed && !worker.module_presentation_options(second, "Text").get().double_space, "closed tab rejects spacing without disrupting FIFO");
    }
    {
      xc_worker *opened = nullptr; check(xc_worker_open(root.c_str(), &opened) == XC_OK, "open C spacing worker");
      std::unique_ptr<xc_worker, decltype(&xc_worker_close)> worker(opened, xc_worker_close);
      xc_request *request = nullptr;
      auto finish = [&](xc_status status, xc_status expected = XC_OK) {
        std::unique_ptr<xc_request, decltype(&xc_request_release)> result(std::exchange(request, nullptr), xc_request_release);
        check(status == XC_OK && result, "accept C spacing request");
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (xc_request_poll(result.get()) == XC_PENDING) {
          check(std::chrono::steady_clock::now() < deadline, "C spacing request deadline");
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        check(xc_request_poll(result.get()) == expected, "C spacing completion status"); return result;
      };
      auto done = finish(xc_worker_create_tab(worker.get(), &request)); const auto tab = xc_request_tab(done.get());
      check(xc_request_double_space(done.get()) == -1 && xc_request_double_space(nullptr) == -1, "C spacing getter rejects wrong-kind and null");
      check(xc_worker_module_presentation_options(worker.get(), tab, "Text", &request) == XC_OK &&
            xc_request_double_space(request) == -1, "C spacing pending result is unavailable");
      done = finish(XC_OK);
      check(xc_request_double_space(done.get()) == 0 && xc_request_double_space(done.get()) == 0, "C spacing defaults off and getter repeats");
      for (int invalid : {-1, 2, 3}) {
        request = reinterpret_cast<xc_request *>(1);
        check(xc_worker_set_double_space(worker.get(), tab, "Text", invalid, &request) == XC_INVALID_ARGUMENT && !request,
              "C spacing rejects nonboolean before queueing and clears output");
      }
      check(xc_worker_set_double_space(worker.get(), tab, nullptr, 1, &request) == XC_INVALID_ARGUMENT && !request &&
            xc_worker_set_double_space(worker.get(), tab, "", 1, &request) == XC_INVALID_ARGUMENT && !request &&
            xc_worker_set_double_space(worker.get(), tab, "Text", 1, nullptr) == XC_INVALID_ARGUMENT,
            "C spacing setter validates module and request output");
      done = finish(xc_worker_set_double_space(worker.get(), tab, "Missing", 1, &request), XC_MODULE_NOT_FOUND);
      check(xc_request_double_space(done.get()) == -1, "C failed spacing request cannot return stale state");
      done = finish(xc_worker_set_lexical_layout_enabled(worker.get(), tab, 1, &request));
      done = finish(xc_worker_set_double_space(worker.get(), tab, "Text", 1, &request));
      check(xc_request_double_space(done.get()) == -1, "C spacing setter result remains void");
      done = finish(xc_worker_module_presentation_options(worker.get(), tab, "Text", &request));
      check(xc_request_double_space(done.get()) == 1, "C spacing getter returns owned enabled value");
      done = finish(xc_worker_read_tab_entry(worker.get(), tab, "Text", "John4:1", &request));
      std::unique_ptr<xc_entry, decltype(&xc_entry_release)> entry(xc_request_take_entry(done.get()), xc_entry_release);
      xc_lexical_presentation layout{};
      check(xc_entry_lexical_layout(entry.get(), &layout) == XC_OK && !layout.requested && !layout.applied &&
            std::string(layout.css) == lexical_block_style(false, false, false, true), "C snapshot exposes spacing-only CSS without annotation flags");
      done = finish(xc_worker_close_tab(worker.get(), tab, &request));
      done = finish(xc_worker_set_double_space(worker.get(), tab, "Text", 0, &request), XC_INVALID_ARGUMENT);
      worker.reset();
      check(xc_entry_lexical_layout(entry.get(), &layout) == XC_OK && std::string(layout.css).find("2em") != std::string::npos,
            "C spacing-only CSS remains owned after tab/worker destruction");
    }
    std::cout << "Shared double-spacing defaults, CSS precedence/fallback, profile, history and worker isolation passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
