// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/engine_manager.hpp"
#include <markupfiltmgr.h>
#include <array>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
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
    std::cout << "Rendered reference-list legacy output/settings/history/lifetime checks passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
