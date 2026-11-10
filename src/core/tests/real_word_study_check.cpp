// SPDX-License-Identifier: GPL-2.0-or-later
// Opt-in validation against a disposable, separately acquired SWORD corpus.
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace xiphos::core;

static void check(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}

static void wait_for(xc_request *request, xc_status expected = XC_OK) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
  auto status = xc_request_poll(request);
  while (status == XC_PENDING && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    status = xc_request_poll(request);
  }
  check(status == expected, "C corpus request returned an unexpected status");
}

static void check_c_split(const char *root, const char *profile) {
  xc_worker *raw_worker = nullptr;
  check(xc_worker_open(root, &raw_worker) == XC_OK, "Open C corpus worker");
  std::unique_ptr<xc_worker, decltype(&xc_worker_close)> worker(raw_worker, xc_worker_close);
  xc_request *raw_request = nullptr;
  check(xc_worker_create_tab(worker.get(), &raw_request) == XC_OK, "Create C corpus tab");
  std::unique_ptr<xc_request, decltype(&xc_request_release)> request(raw_request, xc_request_release);
  wait_for(request.get());
  const auto tab = xc_request_tab(request.get());
  check(xc_worker_visit_parallel_profile_from(worker.get(), tab, profile, "Split",
      "RusSynodal", "Leviticus 14:55", 0, 0, XC_PARALLEL_MAX_SNAPSHOT_BYTES,
      &raw_request) == XC_OK, "Submit C split comparison");
  request.reset(raw_request);
  wait_for(request.get());
  std::unique_ptr<xc_parallel_chapter, decltype(&xc_parallel_chapter_release)>
      chapter(xc_request_take_parallel_chapter(request.get()), xc_parallel_chapter_release);
  request.reset();
  check(xc_worker_map_reference(worker.get(), "OSHB", "ASV", "Psalms 3:2",
      &raw_request) == XC_OK, "Submit standalone C translation mapping");
  request.reset(raw_request);
  wait_for(request.get());
  std::unique_ptr<xc_link, decltype(&xc_link_release)>
      link(xc_request_take_link(request.get()), xc_link_release);
  check(link && !xc_request_take_link(request.get()), "Take standalone mapping once");
  request.reset();
  check(xc_worker_map_reference(worker.get(), "OSHB", "ASV", "Genesis 2:1",
      &raw_request) == XC_OK, "Admit an unavailable mapping");
  request.reset(raw_request);
  wait_for(request.get(), XC_INVALID_ARGUMENT);
  check(!xc_request_take_link(request.get()), "Unavailable mapping returns no C link");
  check(xc_worker_map_reference(worker.get(), "OSHB", "Missing", "Genesis 1:1",
      &raw_request) == XC_OK, "Admit a request with an uninstalled destination");
  request.reset(raw_request);
  wait_for(request.get(), XC_MODULE_NOT_FOUND);
  check(!xc_request_take_link(request.get()), "Missing destination returns no C link");
  check(xc_worker_map_reference(worker.get(), "OSHB", "", "Genesis 1:1",
      &raw_request) == XC_INVALID_ARGUMENT && !raw_request, "Empty destination rejects at admission");
  request.reset();
  worker.reset();
  check(std::string(xc_link_module(link.get())) == "ASV" &&
        std::string(xc_link_reference(link.get())) == "Psalms 3:1",
        "Mapped destination survives C worker shutdown");
  check(chapter && xc_parallel_selected_verse(chapter.get()) == 55, "C split control verse");
  const auto *reference = xc_parallel_cell_additional_reference(chapter.get(), 54, 1, 0);
  const auto *html = xc_parallel_cell_additional_html(chapter.get(), 54, 1, 0);
  check(xc_parallel_cell_additional_count(chapter.get(), 54, 1) == 1 &&
        reference && std::string(reference) == "Leviticus 14:56" && html && *html &&
        !xc_parallel_cell_additional_reference(chapter.get(), 54, 1, 1),
        "C result retains exactly one additional verse after worker shutdown");
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
    (void)worker.visit_parallel_profile(tab,argv[2],"Study","Genesis 2:1",{},
      false,maximum_parallel_snapshot_bytes,"OSHB").get();
  } catch (const std::invalid_argument &) { unmapped = true; }
  check(unmapped && worker.parallel_selection(tab).get()->reference == numbered.reference,
        "SDK systems without mapping tables must not silently fall back to textual alignment");
  (void)worker.edit_parallel_profile(tab,argv[2],
    {ParallelProfileAction::save,"RTL","",{"OSHB","ASV","OSHB"},false}).get();
  const auto rtl = worker.visit_parallel_profile(tab,argv[2],"RTL","Genesis 1:1",{},
      false,maximum_parallel_snapshot_bytes,"OSHB").get();
  const auto &rtl_row = rtl.rows.at(rtl.selected_verse - 1);
  check(rtl.control_module == "OSHB" && rtl.reference == "Genesis 1:1" &&
        rtl.columns.at(0).right_to_left && !rtl.columns.at(1).right_to_left &&
        rtl.columns.at(2).right_to_left &&
        rtl_row.cells.at(0).reference == "Genesis 1:1" &&
        !rtl_row.cells.at(0).html.empty() &&
        rtl_row.cells.at(1).reference == "Genesis 1:1" && !rtl_row.cells.at(1).html.empty() &&
        rtl_row.cells.at(2).html == rtl_row.cells.at(0).html,
        "Real RTL control and duplicate column render alongside the verified KJV anchor");

  const auto mt_to_kjv = worker.visit_parallel_profile(tab,argv[2],"Study","Psalms 3:2",{},
      false,maximum_parallel_snapshot_bytes,"OSHB").get();
  check(mt_to_kjv.reference == "Psalms 3:1", "MT Psalm body maps to KJV verse one");
  const auto kjv_to_mt = worker.visit_parallel_profile(tab,argv[2],"RTL","Psalms 3:1",{},
      false,maximum_parallel_snapshot_bytes,"ASV").get();
  check(kjv_to_mt.reference == "Psalms 3:2" &&
        kjv_to_mt.rows.at(0).cells.at(1).reference.empty() &&
        kjv_to_mt.rows.at(8).cells.at(1).reference == "Psalms 3:8" &&
        !kjv_to_mt.rows.at(8).cells.at(1).html.empty(),
        "Reverse mapping preserves the title gap and aligns the last verse");
  const std::vector<std::pair<std::string,std::string>> export_sources{
      {"OSHB","Genesis 1:1"}, {"MorphGNT","John 1:1"}, {"Peshitta","John 1:1"}};
  for (const auto &source : export_sources) {
    const auto expression = source.second + ";" + source.second;
    const auto plain = worker.export_passage_text(tab,source.first,expression,source.second).get();
    check(plain.module == source.first && plain.verse_count == 2 &&
          plain.text.find(source.second + "\t") != std::string::npos,
          "Real multilingual plain passage export retains module and duplicates");
    try {
      const auto html = worker.export_passage_html(tab,source.first,expression,source.second).get();
      check(html.module == source.first && html.verse_count == 2 &&
            html.html.find("default-src 'none'") != std::string::npos,
            "Real multilingual HTML passage export retains module and duplicates");
    } catch (const std::exception &error) {
      throw std::runtime_error("Real passage HTML failed for " + source.first + ": " + error.what());
    }
  }
  const auto rtl_export = worker.export_passage_html(tab,"OSHB","Genesis 1:1","Genesis 1:1").get();
  check(rtl_export.html.find("dir=\"auto\"") != std::string::npos &&
        rtl_export.html.find(u8"ב") != std::string::npos,
        "Real Hebrew HTML export preserves automatic direction and Hebrew verse text");
  const auto switched = worker.map_reference("ASV", "OSHB", "Psalms 3:1").get();
  check(switched.module == "OSHB" && switched.reference == "Psalms 3:2",
        "Standalone resolver works without a comparison profile");
  for (int verse = 1; verse <= 31; ++verse) {
    const auto reference = "Genesis 1:" + std::to_string(verse);
    check(worker.map_reference("OSHB", "ASV", reference).get().reference == reference &&
          worker.map_reference("ASV", "OSHB", reference).get().reference == reference,
          "All Genesis 1 verses map directly in both directions");
  }
  const auto apocrypha_anchor = worker.map_reference("OSHB", "KJVA", "Psalms 3:9").get();
  check(apocrypha_anchor.module == "KJVA" && apocrypha_anchor.reference == "Psalms 3:8",
        "KJVA uses the same verified Psalm numbering as KJV");
  const auto split_target = worker.map_reference("RusSynodal", "ASV", "Leviticus 14:55").get();
  check(split_target.reference == "Leviticus 14:55",
        "Standalone resolver selects the first numbered destination of a split");
  const auto numbered_target = worker.map_reference("RusSynodal", "ASV", "Psalms 10:1").get();
  check(numbered_target.reference == "Psalms 11:1",
        "Standalone resolver skips a verse-zero destination in a split");
  bool title_unmapped = false;
  try {
    (void)worker.map_reference("OSHB", "ASV", "Psalms 3:1").get();
  } catch (const std::invalid_argument &) { title_unmapped = true; }
  check(title_unmapped, "The Hebrew Psalm title must not open the first English body verse");
  bool outside_coverage = false;
  try {
    (void)worker.map_reference("OSHB", "ASV", "Genesis 2:1").get();
  } catch (const std::invalid_argument &) { outside_coverage = true; }
  check(outside_coverage, "Standalone resolver refuses unverified references");
  bool embedded_nul = false;
  try {
    (void)worker.map_reference(std::string("OSHB\0extra", 10), "ASV", "Genesis 1:1").get();
  } catch (const std::invalid_argument &) { embedded_nul = true; }
  check(embedded_nul, "C++ resolver rejects embedded NUL instead of truncating the module ID");
  const auto after = worker.history(tab).get();
  check(after.cursor == history.cursor && after.entries.size() == history.entries.size() &&
        after.can_back == history.can_back && after.can_forward == history.can_forward &&
        std::equal(after.entries.begin(),after.entries.end(),history.entries.begin(),
          [](const auto &a, const auto &b) { return a.module == b.module && a.reference == b.reference; }),
        "Word study and comparison changed main reading history");
  check_c_split(argv[1], argv[2]);
  check(inspect_legacy_assets(argv[1]).revision == before, "Real corpus was changed");
  std::cout << "Real Hebrew, Greek, Aramaic and cross-canon comparison checks passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
