// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/variant.h"
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
    check(std::chrono::steady_clock::now() < deadline, "C variant request deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return status;
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "variant fixture root required"); const std::string root = argv[1];
    std::array<std::string, 3> expected;
    const std::array<const char *, 3> choices = {"Primary Reading", "Secondary Reading", "All Readings"};
    {
      EngineManager legacy(root.c_str(), true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
      auto *module = legacy.getModule("OptionOther"); check(module != nullptr, "variant module installed");
      for (unsigned choice = 0; choice < choices.size(); ++choice) {
        legacy.setGlobalOption("Textual Variants", choices[choice]);
        expected[choice] = render_entry(*module, "John3:22");
      }
      check(expected[0].find("Original primary reading") != std::string::npos && expected[0].find("Original secondary reading") == std::string::npos &&
            expected[1].find("Original primary reading") == std::string::npos && expected[1].find("Original secondary reading") != std::string::npos &&
            expected[2].find("Original primary reading") != std::string::npos && expected[2].find("Original secondary reading") != std::string::npos,
            "all three direct legacy choices affect actual output");
    }
    for (unsigned prior = 0; prior < 8; ++prior) for (int choice = 0; choice < 3; ++choice) {
      auto options = engine_options_from_preferences({}); options.strongs = options.morphs = true;
      options.variants_primary = prior & 1; options.variants_secondary = prior & 2; options.variants_all = prior & 4;
      const auto selected = select_variant_reading(options, choice);
      auto before = engine_options_to_preferences(options), after = engine_options_to_preferences(selected);
      for (const auto *name : choices) { before.erase(name); after.erase(name); }
      check(before == after, "variant selection preserves every unrelated preference");
      const auto flags = xiphos_core_variant_selection(choice);
      check(selected.variants_primary == bool(flags.primary) && selected.variants_secondary == bool(flags.secondary) &&
            selected.variants_all == bool(flags.all), "selection uses characterized legacy policy");
    }
    {
      Reader reader(root); StudySession first(reader), second(reader);
      bool rejected = false;
      try { first.select_module_variant("OptionOther", XIPHOS_VARIANT_ALL); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected && !first.module_engine_options("OptionOther"), "unconfigured selection rejects without guessing SDK defaults");
      first.enable_module_preferences(); second.enable_module_preferences();
      auto options = engine_options_from_preferences({}); options.strongs = true;
      first.set_module_engine_options("OptionOther", options);
      first.visit("OptionOther", "John3:21"); first.visit("OptionOther", "John3:22"); first.navigate(-1);
      const auto count = first.history().entries().size(), cursor = first.history().cursor();
      for (int choice = 0; choice < 3; ++choice) {
        first.select_module_variant("OptionOther", choice);
        check(first.read_entry("OptionOther", "John3:22").html == expected[choice] &&
              first.read_chapter("OptionOther", "John3:22").verses[21].html == expected[choice], "variant entry/chapter matches direct legacy calls");
        const auto preview = first.read_reference_list("OptionOther", "22;22", "John3:21");
        check(preview.entries.size() == 2 && preview.entries[0].html == expected[choice], "reference preview shares selected variant");
        check(first.module_engine_options("OptionOther")->strongs && second.read_entry("OptionOther", "John3:22").html == expected[0],
              "unrelated options and other tab remain unchanged");
      }
      check(first.history().entries().size() == count && first.history().cursor() == cursor && first.history().can_forward(), "selection preserves history and forward state");
      const auto before = engine_options_to_preferences(*first.module_engine_options("OptionOther"));
      rejected = false; try { first.select_module_variant("OptionOther", -1); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected && engine_options_to_preferences(*first.module_engine_options("OptionOther")) == before, "invalid choice preserves configured options");
      rejected = false; try { first.select_module_variant("OptionTest", XIPHOS_VARIANT_ALL); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected && first.module_engine_options("OptionTest")->variants_primary, "undeclared variant rejected without mutation");
      const auto saved = root + "/variant-saved-modops.conf";
      first.save_module_preferences(saved); second.import_module_preferences(saved);
      check(second.read_entry("OptionOther", "John3:22").html == expected[2], "selected variant saves/reloads in shared format");
    }
    {
      StudyWorker worker(root); const auto tab = worker.create_tab().get();
      worker.enable_module_preferences(tab).get();
      std::string module = "OptionOther";
      auto accepted = worker.select_module_variant(tab, module, XIPHOS_VARIANT_SECONDARY); module = "Missing"; accepted.get();
      check(worker.read_entry(tab, "OptionOther", "John3:22").get().html == expected[1], "worker selection copies module and serializes settings");
      bool rejected = false; try { worker.select_module_variant(tab, "OptionOther", 9).get(); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected && worker.read_entry(tab, "OptionOther", "John3:22").get().html == expected[1] && worker.history(tab).get().entries.empty(), "failed worker selection preserves state and queue");
      worker.close_tab(tab).get(); rejected = false;
      try { worker.select_module_variant(tab, "OptionOther", XIPHOS_VARIANT_ALL).get(); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "closed tab rejects variant selection");
    }
    {
      xc_worker *worker = nullptr; xc_request *request = nullptr;
      check(xc_worker_open(root.c_str(), &worker) == XC_OK, "C variant worker");
      check(xc_worker_create_tab(worker, &request) == XC_OK && wait(request) == XC_OK, "C variant tab");
      const auto tab = xc_request_tab(request); xc_request_release(request);
      check(xc_worker_select_module_variant(worker, tab, "OptionOther", XIPHOS_VARIANT_ALL, &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT,
            "C unconfigured selection rejects"); xc_request_release(request);
      const unsigned variantFlags = XC_ENGINE_VARIANTS_PRIMARY | XC_ENGINE_VARIANTS_SECONDARY | XC_ENGINE_VARIANTS_ALL;
      check(xc_worker_set_module_engine_options(worker, tab, "OptionOther", XC_ENGINE_STRONGS | variantFlags, &request) == XC_OK && wait(request) == XC_OK,
            "C prior ambiguous flags"); xc_request_release(request);
      const std::array<unsigned, 3> selectedFlags = {XC_ENGINE_VARIANTS_PRIMARY, XC_ENGINE_VARIANTS_SECONDARY, XC_ENGINE_VARIANTS_ALL};
      for (int choice = 0; choice < 3; ++choice) {
        check(xc_worker_select_module_variant(worker, tab, "OptionOther", choice, &request) == XC_OK && wait(request) == XC_OK, "C variant selection");
        check(!xc_request_take_entry(request), "selection request is status-only"); xc_request_release(request);
        check(xc_worker_module_engine_options(worker, tab, "OptionOther", &request) == XC_OK && wait(request) == XC_OK, "C selected flags");
        unsigned flags = 0; int configured = 0;
        check(xc_request_engine_options(request, &flags, &configured) == XC_OK && configured && flags == (XC_ENGINE_STRONGS | selectedFlags[choice]),
              "C selection preserves unrelated flags and selects one reading"); xc_request_release(request);
        check(xc_worker_read_tab_entry(worker, tab, "OptionOther", "John3:22", &request) == XC_OK && wait(request) == XC_OK, "C selected rendering");
        auto *entry = xc_request_take_entry(request); xc_request_release(request);
        check(entry && std::string(xc_entry_html(entry)) == expected[choice], "C actual variant output"); xc_entry_release(entry);
      }
      for (const int choice : {-1, 3})
        check(xc_worker_select_module_variant(worker, tab, "OptionOther", choice, &request) == XC_INVALID_ARGUMENT && !request, "C invalid choice before queue");
      check(xc_worker_select_module_variant(worker, tab, nullptr, 0, &request) == XC_INVALID_ARGUMENT && !request &&
            xc_worker_select_module_variant(worker, tab, "", 0, &request) == XC_INVALID_ARGUMENT && !request &&
            xc_worker_select_module_variant(worker, tab, "OptionOther", 0, nullptr) == XC_INVALID_ARGUMENT, "C null/empty selection arguments");
      check(xc_worker_select_module_variant(worker, tab, "Missing", 0, &request) == XC_OK && wait(request) == XC_MODULE_NOT_FOUND, "C missing variant module"); xc_request_release(request);
      check(xc_worker_enable_module_preferences(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "C enable preferences"); xc_request_release(request);
      check(xc_worker_select_module_variant(worker, tab, "OptionTest", 0, &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT, "C undeclared variant"); xc_request_release(request);
      check(xc_worker_close_tab(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "C close variant tab"); xc_request_release(request);
      check(xc_worker_select_module_variant(worker, tab, "OptionOther", 0, &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT, "C closed variant tab"); xc_request_release(request);
      xc_worker_close(worker);
    }
    std::cout << "Shared variant rendering, isolation, history and storage checks passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
