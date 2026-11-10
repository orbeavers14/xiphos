// SPDX-License-Identifier: GPL-2.0-or-later
// Opt-in validation against a disposable, separately acquired SWORD corpus.
#include "xiphos/core/worker.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace xiphos::core;

static void check(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}

int main(int argc, char **argv) try {
  check(argc == 3, "Supply a disposable SWORD library and a new comparison profile path");
  check(!std::filesystem::exists(std::filesystem::u8path(argv[2])), "Comparison profile must be new");
  const auto before = inspect_legacy_assets(argv[1]).revision;
  StudyWorker worker(argv[1]);
  const auto tab = worker.create_tab().get();
  (void)worker.visit_passage(tab,"ASV","","John 3:16").get();
  (void)worker.visit_passage(tab,"ASV","","John 3:17").get();
  const auto history = worker.history(tab).get();
  auto word = [&](const char *module, const char *reference, const char *surface) {
    auto result = worker.word_study(tab, module, reference, surface).get();
    check(result.module == module && result.reference == reference && result.surface == surface,
          "Word lookup lost source identity or surface text");
    return result.lemmas;
  };
  const auto hebrew = word("OSHB","Genesis 1:1",u8"בְּרֵאשִׁית");
  check(hebrew == std::vector<std::string>{"H07225"} &&
        word("OSHB","Genesis 1:1",u8"בראשית") == hebrew, "OSHB pointed and unpointed terms");
  const auto aramaic = word("OSHB","Daniel 2:4",u8"מַלְכָּא");
  check(aramaic == std::vector<std::string>{"H04430"} &&
        word("OSHB","Daniel 2:4",u8"מלכא") == aramaic, "OSHB Biblical Aramaic pointed and unpointed terms");
  const auto greek = word("MorphGNT","John 1:1",u8"Ἐν");
  check(greek == std::vector<std::string>{u8"ἐν","G1722"} &&
        word("MorphGNT","John 1:1",u8"Εν") == greek, "MorphGNT breathing and multiple lemma terms");
  const auto accented = word("MorphGNT","John 1:1",u8"ἀρχῇ");
  check(accented == std::vector<std::string>{u8"ἀρχή","G0746"} &&
        word("MorphGNT","John 1:1",u8"αρχη") == accented, "MorphGNT accents and exact padded Strong’s term");
  check(word("Peshitta","John 1:1",u8"ܒܪܫܝܬ").empty(), "Untagged Syriac should return no lemma");
  SearchQuery syriac{"Peshitta",u8"ܒܪܫܝܬ",SearchMode::phrase};
  syriac.range = SearchRange{"Peshitta","John 1:1"};
  check(worker.search_module(tab,syriac).get().references == std::vector<std::string>{"John 1:1"},
        "Untagged Syriac still supports surface search");
  auto surface = [&](const char *module, const char *reference, const char *text, SearchMode mode) {
    SearchQuery query{module,text,mode,true};
    query.range = SearchRange{module,reference};
    return worker.search_module(tab,query).get().references;
  };
  check(surface("OSHB","Genesis 1:1",u8"בְּרֵאשִׁ֖ית",SearchMode::surface_literal).size() == 1 &&
        surface("OSHB","Genesis 1:1",u8"בראשית",SearchMode::surface_literal).empty() &&
        surface("OSHB","Genesis 1:1",u8"בראשית",SearchMode::surface_unmarked).size() == 1,
        "Literal and mark-insensitive Hebrew surface search");
  check(surface("OSHB","Daniel 2:4",u8"מלכא",SearchMode::surface_literal).empty() &&
        surface("OSHB","Daniel 2:4",u8"מלכא",SearchMode::surface_unmarked).size() == 1,
        "Mark-insensitive Biblical Aramaic surface search");
  check(surface("MorphGNT","John 1:1",u8"Ἐν",SearchMode::surface_literal).size() == 1 &&
        surface("MorphGNT","John 1:1",u8"Εν",SearchMode::surface_literal).empty() &&
        surface("MorphGNT","John 1:1",u8"Εν",SearchMode::surface_unmarked).size() == 1,
        "Literal and mark-insensitive Greek surface search");
  check(surface("Peshitta","John 1:1",u8"ܒܪܫܝܬ",SearchMode::surface_unmarked).size() == 1,
        "Untagged Syriac participates in Unicode surface search");
  for (const auto &term : accented) {
    SearchQuery query{"MorphGNT",term,SearchMode::lemma};
    query.range = SearchRange{"MorphGNT","John 1:1"};
    check(worker.search_module(tab,query).get().references == std::vector<std::string>{"John 1:1"},
          "Real Greek lemma choice must find the source");
  }
  const auto psalm = word("OSHB","Psalms 3:2",u8"יהוה");
  check(psalm == std::vector<std::string>{"H03068"}, "OSHB Psalm lemma available");
  SearchQuery query{"OSHB",psalm.front(),SearchMode::lemma};
  query.range = SearchRange{"OSHB","Psalms 3:2"};
  check(worker.search_module(tab,query).get().references == std::vector<std::string>{"Psalms 3:2"},
        "Real Hebrew lemma choice must find the source");
  SearchQuery synodal{"RusSynodal",u8"умножились",SearchMode::phrase};
  synodal.range = SearchRange{"RusSynodal","Psalms 3:2"};
  const auto hits = worker.search_module(tab,synodal).get();
  check(hits.references == std::vector<std::string>{"Psalms 3:2"}, "Find Synodal verse through shared search");
  (void)worker.edit_parallel_profile(tab,argv[2],
    {ParallelProfileAction::save,"Study","",{"ASV","RusSynodal","Missing"},true}).get();
  const auto chapter = worker.visit_parallel_profile(tab,argv[2],std::nullopt,hits.references.front(),{},
      false,maximum_parallel_snapshot_bytes,"RusSynodal").get();
  const auto &row = chapter.rows.at(chapter.selected_verse - 1);
  check(chapter.reference == "Psalms 3:1" && row.cells.at(1).reference == hits.references.front() &&
        !row.cells.at(1).html.empty() && row.cells.at(2).reference.empty(), "Synodal to KJV comparison mapping and missing slot");
  const auto reread = worker.read_committed_parallel(tab).get();
  check(reread.rows.at(reread.selected_verse - 1).cells.at(1).reference == hits.references.front(),
        "Committed comparison reread lost mapped alignment");
  bool rejected = false;
  try {
    (void)worker.visit_parallel_profile(tab,argv[2],std::nullopt,"Tobit 1:1",{},false,
        maximum_parallel_snapshot_bytes,"KJVA").get();
  } catch (const std::invalid_argument &) { rejected = true; }
  check(rejected && worker.parallel_selection(tab).get()->reference == chapter.reference,
        "A verse absent from the control canon must preserve prior comparison");
  (void)worker.edit_parallel_profile(tab,argv[2],
    {ParallelProfileAction::save,"Extended","",{"KJVA","ASV"},false}).get();
  const auto extended = worker.visit_parallel_profile(tab,argv[2],"Extended","Tobit 1:1",{},
      false,maximum_parallel_snapshot_bytes,"KJVA").get();
  const auto &absent = extended.rows.at(extended.selected_verse - 1).cells.at(1);
  check(extended.columns.at(1).status == ParallelColumnStatus::ready &&
        absent.reference.empty() && absent.html.empty(), "Installed KJV module has no mapped Tobit verse");
  (void)worker.edit_parallel_profile(tab,argv[2],
    {ParallelProfileAction::save,"Split","",{"RusSynodal","ASV"},false}).get();
  const auto split = worker.visit_parallel_profile(tab,argv[2],"Split","Leviticus 14:55",{},
      false,maximum_parallel_snapshot_bytes,"RusSynodal").get();
  const auto &parts = split.rows.at(split.selected_verse - 1).cells.at(1);

  check(parts.reference == "Leviticus 14:55" && parts.additional.size() == 1 &&
        parts.additional.front().reference == "Leviticus 14:56" && !parts.additional.front().html.empty(),
        "Split mapping retains both actual KJV verses");
  const auto merged = worker.visit_parallel_profile(tab,argv[2],"Study","Leviticus 14:55",{},
      false,maximum_parallel_snapshot_bytes,"RusSynodal").get();
  check(merged.reference == "Leviticus 14:55" &&
        merged.rows.at(54).cells.at(1).reference == "Leviticus 14:55" &&
        merged.rows.at(55).cells.at(1).reference == "Leviticus 14:55",
        "Merged mapping repeats the same actual Synodal verse for both control rows");
  const auto numbered = worker.visit_parallel_profile(tab,argv[2],"Study","Psalms 10:1",{},
      false,maximum_parallel_snapshot_bytes,"RusSynodal").get();
  check(numbered.reference == "Psalms 11:1", "A split starting at verse zero opens at the first numbered verse");
  bool unmapped = false;
  try {
    (void)worker.visit_parallel_profile(tab,argv[2],"Study","Genesis 1:1",{},
      false,maximum_parallel_snapshot_bytes,"OSHB").get();
  } catch (const std::invalid_argument &) { unmapped = true; }
  check(unmapped && worker.parallel_selection(tab).get()->reference == numbered.reference,
        "SDK systems without mapping tables must not silently fall back to textual alignment");
  const auto after = worker.history(tab).get();
  check(after.cursor == history.cursor && after.entries.size() == history.entries.size() &&
        after.can_back == history.can_back && after.can_forward == history.can_forward &&
        std::equal(after.entries.begin(),after.entries.end(),history.entries.begin(),
          [](const auto &a, const auto &b) { return a.module == b.module && a.reference == b.reference; }),
        "Word study and comparison changed main reading history");
  check(inspect_legacy_assets(argv[1]).revision == before, "Real corpus was changed");
  std::cout << "Real Hebrew, Greek, Aramaic and cross-canon comparison checks passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
