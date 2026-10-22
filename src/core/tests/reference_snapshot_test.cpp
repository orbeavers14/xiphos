// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include "xiphos/core/engine_manager.hpp"
#include <markupfiltmgr.h>
#include <array>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
xc_status wait(xc_request *request) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  xc_status status;
  while ((status = xc_request_poll(request)) == XC_PENDING) {
    check(std::chrono::steady_clock::now() < deadline, "C reference-list request deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return status;
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture root required"); const std::string root = argv[1];
    const std::array<const char *, 5> references = {"John 3:16", "John 3:17", "John 3:18", "John 3:19", "John 3:16"};
    const std::string expression = "16-19;16";
    std::array<std::vector<Entry>, 4> expected;
    {
      EngineManager legacy(root.c_str(), true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
      auto *module = legacy.getModule("OptionOther"); check(module != nullptr, "load snapshot fixture");
      legacy.setGlobalOption("Words of Christ in Red", "On"); legacy.setGlobalOption("Footnotes", "On");
      legacy.setGlobalOption("Morphological Tags", "Off"); legacy.setGlobalOption("Cross-references", "On");
      for (unsigned bits = 0; bits < expected.size(); ++bits) {
        legacy.setGlobalOption("Strong's Numbers", bits & 1 ? "On" : "Off");
        legacy.setGlobalOption("Greek Accents", bits & 2 ? "On" : "Off");
        for (const auto *reference : references) expected[bits].push_back({reference, render_entry(*module, reference)});
      }
    }
    ReferenceList owned;
    {
      Reader reader(root);
      const auto before = reader.read("OptionOther", "John 3:16").html;
      for (unsigned bits = 0; bits < expected.size(); ++bits) {
        auto options = engine_options_from_preferences({}); options.strongs = bits & 1; options.greekaccents = bits & 2;
        const auto snapshot = reader.read_reference_list("OptionOther", expression, "John.3.16", options);
        check(snapshot.module == "OptionOther" && snapshot.context == "John 3:16" && snapshot.expression == expression &&
              snapshot.entries.size() == expected[bits].size() && !snapshot.limited, "owned list metadata/order/context");
        for (std::size_t i = 0; i < snapshot.entries.size(); ++i)
          check(snapshot.entries[i].reference == expected[bits][i].reference && snapshot.entries[i].html == expected[bits][i].html,
                "all list entry outputs match direct legacy SWORD calls");
        check(snapshot.html.find("reference-5") != std::string::npos && snapshot.html.find("sword:///John%203:16") != std::string::npos,
              "duplicate references retain positions and shared navigation links");
        check(reader.read("OptionOther", "John 3:16").html == before, "snapshot restores SDK option state");
      }
      const auto missing = reader.read_reference_list("OptionTest", "John3:36-4:2", "John3:16");
      check(missing.entries.size() == 3 && missing.entries[0].reference == "John 3:36" && missing.entries[2].reference == "John 4:2" &&
            missing.entries[0].html.empty(), "cross-chapter range retains missing-text positions");
      const auto empty = reader.read_reference_list("OptionOther", "", "John3:16");
      check(empty.entries.empty() && empty.html.empty() && empty.context == "John 3:16", "empty list result");
      owned = reader.read_reference_list("OptionOther", expression, "John3:16");
      StudySession first(reader), second(reader);
      auto options = engine_options_from_preferences({}); options.strongs = true;
      first.set_module_engine_options("OptionOther", options);
      options.strongs = options.greekaccents = false; second.set_module_engine_options("OptionOther", options);
      first.visit("OptionOther", "John3:19"); first.visit("OptionOther", "John3:18");
      const auto count = first.history().entries().size(), cursor = first.history().cursor();
      check(first.read_reference_list("OptionOther", expression, "John3:16").entries[0].html == expected[3][0].html &&
            second.read_reference_list("OptionOther", expression, "John3:16").entries[2].html == expected[0][2].html,
            "list settings isolated per tab");
      check(first.history().entries().size() == count && first.history().cursor() == cursor, "list preview preserves history");
      bool rejected = false;
      try { first.read_reference_list("Missing", expression, "John3:16"); } catch (const std::out_of_range &) { rejected = true; }
      check(rejected && first.history().cursor() == cursor, "missing module rejects without moving history");
      rejected = false;
      try { first.read_reference_list("OptionOther", expression, ""); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected && first.read_entry("OptionOther", "John3:16").html == expected[3][0].html, "failed list preserves settings");
      const auto guarded = first.read_reference_list("OptionOther", "Gen1:1-Rev22:21", "John3:16");
      check(guarded.limited && guarded.entries.size() == 2000, "rendered snapshots propagate legacy guard metadata");
      const std::string multi="Gen1:1-Gen50:26;Exod1:1-Exod40:38";
      const auto boundedTargets=reader.reference_targets("OptionOther",multi,"John3:16");
      const auto boundedPreview=first.read_reference_list("OptionOther",multi,"John3:16");
      check(boundedTargets.limited && boundedTargets.references.size()==2000 && boundedPreview.limited && boundedPreview.entries.size()==2000 &&
            boundedPreview.entries.back().reference==boundedTargets.references.back() && first.history().cursor()==cursor,
            "native target and rendered previews share aggregate ordered prefix without history mutation");
    }
    check(owned.entries.size() == 5 && owned.html.find("reference-5") != std::string::npos, "snapshot outlives reader");
    std::future<ReferenceList> accepted;
    {
      StudyWorker worker(root); const auto tab = worker.create_tab().get();
      auto options = engine_options_from_preferences({}); options.strongs = true;
      worker.set_module_engine_options(tab, "OptionOther", options).get();
      std::string input = expression, context = "John3:16";
      accepted = worker.read_reference_list(tab, "OptionOther", input, context); input = ""; context = "";
      bool rejected = false;
      try { (void)worker.read_reference_list(tab, "Missing", expression, "John3:16").get(); }
      catch (const std::out_of_range &) { rejected = true; }
      check(rejected && worker.history(tab).get().entries.empty(), "worker queue recovers and preserves empty history");
      worker.close_tab(tab).get(); rejected = false;
      try { (void)worker.read_reference_list(tab, "OptionOther", expression, "John3:16").get(); }
      catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "closed list tab rejected");
    }
    const auto result = accepted.get();
    check(result.entries.size() == 5 && result.entries[0].html == expected[3][0].html, "queued arguments copied and result outlives worker");
    {
      xc_worker *worker = nullptr; xc_request *request = nullptr;
      check(xc_worker_open(root.c_str(), &worker) == XC_OK, "C reference-list worker");
      check(xc_worker_create_tab(worker, &request) == XC_OK && wait(request) == XC_OK && !xc_request_take_reference_list(request), "C wrong request kind");
      const auto tab = xc_request_tab(request); xc_request_release(request);
      const unsigned flags = XC_ENGINE_STRONGS | XC_ENGINE_WORDS_IN_RED | XC_ENGINE_FOOTNOTES | XC_ENGINE_GREEK_ACCENTS | XC_ENGINE_CROSS_REFERENCES;
      check(xc_worker_set_module_engine_options(worker, tab, "OptionOther", flags, &request) == XC_OK && wait(request) == XC_OK, "C list options");
      xc_request_release(request);
      std::string input = expression, context = "John.3.16";
      check(xc_worker_read_reference_list(worker, tab, "OptionOther", input.c_str(), context.c_str(), &request) == XC_OK && !xc_request_take_reference_list(request),
            "C reference list submitted/not ready"); input = ""; context = "";
      check(wait(request) == XC_OK && !xc_request_take_entry(request) && !xc_request_take_note(request), "C reference-list result kind");
      auto *snapshot = xc_request_take_reference_list(request);
      check(snapshot && !xc_request_take_reference_list(request), "C reference-list transfers once"); xc_request_release(request);
      check(xc_reference_list_count(snapshot) == 5 && std::string(xc_reference_list_module(snapshot)) == "OptionOther" &&
            std::string(xc_reference_list_context(snapshot)) == "John 3:16" && std::string(xc_reference_list_expression(snapshot)) == expression &&
            !xc_reference_list_limited(snapshot) && !xc_reference_list_skipped(snapshot), "C copied snapshot metadata");
      for (std::size_t i = 0; i < expected[3].size(); ++i)
        check(std::string(xc_reference_list_entry_reference(snapshot, i)) == expected[3][i].reference &&
              std::string(xc_reference_list_entry_html(snapshot, i)) == expected[3][i].html, "C actual rendered entry outputs");
      check(!xc_reference_list_entry_reference(snapshot, 5) && !xc_reference_list_entry_html(snapshot, static_cast<std::size_t>(-1)), "C snapshot bounds");
      check(xc_worker_read_reference_list(worker, tab, "OptionOther", "", "John3:16", &request) == XC_OK && wait(request) == XC_OK, "C empty expression accepted");
      auto *empty = xc_request_take_reference_list(request); xc_request_release(request);
      check(empty && xc_reference_list_count(empty) == 0 && !xc_reference_list_entry_reference(empty, 0) && std::string(xc_reference_list_html(empty)).empty(), "C empty snapshot");
      xc_reference_list_release(empty);
      check(xc_worker_read_reference_list(worker, tab, "OptionOther", "Gen1:1-Gen50:26;Exod1:1-Exod40:38", "John3:16", &request) == XC_OK && wait(request) == XC_OK, "C guarded list");
      auto *guarded = xc_request_take_reference_list(request); xc_request_release(request);
      check(guarded && xc_reference_list_limited(guarded) && xc_reference_list_count(guarded) == 2000, "C aggregate preview guard metadata"); xc_reference_list_release(guarded);
      check(xc_worker_read_reference_list(worker, tab, "Missing", expression.c_str(), "John3:16", &request) == XC_OK && wait(request) == XC_MODULE_NOT_FOUND &&
            !xc_request_take_reference_list(request), "C missing module error"); xc_request_release(request);
      check(xc_worker_read_reference_list(worker, tab, nullptr, "16", "John3:16", &request) == XC_INVALID_ARGUMENT && !request, "C null module rejected");
      check(xc_worker_read_reference_list(worker, tab, "OptionOther", nullptr, "John3:16", &request) == XC_INVALID_ARGUMENT && !request, "C null expression rejected");
      check(xc_worker_read_reference_list(worker, tab, "OptionOther", "16", "", &request) == XC_INVALID_ARGUMENT && !request, "C empty context rejected");
      check(xc_worker_read_reference_list(worker, tab, "OptionOther", "16", "John3:16", nullptr) == XC_INVALID_ARGUMENT, "C null output rejected");
      check(xc_worker_history(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "C list queue recovers");
      auto *history = xc_request_take_history(request); xc_request_release(request);
      check(history && xc_history_count(history) == 0, "C list preview preserves history"); xc_history_release(history);
      xc_request *held = nullptr;
      check(xc_worker_read_reference_list(worker, tab, "OptionOther", expression.c_str(), "John3:16", &held) == XC_OK, "C request retained through shutdown");
      check(xc_worker_close_tab(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "C list tab close"); xc_request_release(request);
      check(xc_worker_read_reference_list(worker, tab, "OptionOther", "16", "John3:16", &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT, "C closed list tab rejected");
      xc_request_release(request); xc_worker_close(worker);
      check(wait(held) == XC_OK, "C pending list result observed after worker close");
      auto *afterClose = xc_request_take_reference_list(held); xc_request_release(held);
      check(afterClose && xc_reference_list_count(afterClose) == 5 && std::string(xc_reference_list_entry_html(snapshot, 0)) == expected[3][0].html,
            "C request/result snapshots outlive worker"); xc_reference_list_release(afterClose); xc_reference_list_release(snapshot);
      xc_reference_list_release(nullptr);
      check(!xc_reference_list_module(nullptr) && !xc_reference_list_context(nullptr) && !xc_reference_list_expression(nullptr) && !xc_reference_list_html(nullptr) &&
            !xc_reference_list_count(nullptr) && !xc_reference_list_limited(nullptr) && !xc_reference_list_skipped(nullptr) &&
            !xc_reference_list_entry_reference(nullptr, 0) && !xc_reference_list_entry_html(nullptr, 0) && !xc_request_take_reference_list(nullptr), "C null reference-list access");
    }
    std::cout << "Rendered reference-list legacy output/settings/history/lifetime checks passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
