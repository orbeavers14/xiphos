// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace xiphos::linux_preview;
void check(bool ok, const char *why)
{
	if (!ok)
		throw std::runtime_error(why);
}
void settle(ReaderModel &m, bool success = true)
{
	auto end = std::chrono::steady_clock::now() + std::chrono::seconds(30);
	do {
		m.tick();
		check(std::chrono::steady_clock::now() < end, "Mixed model deadline");
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (m.busy());
	if (success)
		check(m.error().empty(), m.error().c_str());
}
int main(int argc, char **argv)
try {
	check(argc == 3, "Mixed module fixture and disposable profile required");
	ReaderModel model(argv[1]);
	settle(model);
	check(model.visit("Text", "", "John3:16"), "Seed reading");
	settle(model);
	const auto main = model.passage();
	SearchInput query;
	query.modules = {"KeyDictionary", "CaptureBook"};
	query.expression = "signal";
	check(model.search(query), "Legacy verse-only request");
	settle(model);
	check(model.search_results().hits.empty(), "Keyed modules remain opt-in");
	query.mixed = true;
	check(model.search(query), "Mixed query");
	settle(model);
	const auto held = model.search_results();
	check(!held.hits.empty(), "Mixed results copied");
	bool dictionary = false, book = false;
	for (size_t i = 0; i < held.hits.size(); ++i) {
		const auto hit = held.hits[i];
		check(model.preview_search_hit(i), "Preview typed hit");
		settle(model);
		check(model.search_preview().module == hit.module && model.search_preview().reference == hit.reference, "Exact keyed preview identity");
		check(model.activate_search_hit(i), "Activate typed result");
		settle(model);
		if (hit.kind == 1) {
			dictionary = true;
			check(model.dictionary().module == hit.module && model.dictionary().key == hit.reference, "Dictionary activates exact canonical key");
		}
		if (hit.kind == 2) {
			book = true;
			check(model.book().module == hit.module && model.book().nodes[0].path == hit.reference, "Book opens exact path");
		}
		check(model.passage().reference == main.reference && model.passage().bible == main.bible, "Keyed activation preserves Bible and history");
	}
	check(dictionary && book, "Both keyed families checked");
	check(model.export_search(false, true, {}, true, true, false, false), "Mixed complete text export");
	settle(model);
	check(model.search_export().disposition == XC_SEARCH_EXPORT_OK && !model.search_export().text.empty(), "Typed mixed generation exports through shared service");
	query.modules = {"KeyDictionary"};
	query.interval = std::make_pair("ALPHA", "ALPHA");
	check(model.search(query), "Inclusive exact key interval");
	settle(model);
	check(model.search_results().hits.size() == 1 && model.search_results().hits[0].reference == "ALPHA", "Interval filters opaque keys without verse parsing");
	query.interval.reset();
	query.scope = XC_SEARCH_SCOPE_LAST;
	check(model.search(query), "Narrow complete keyed scope");
	settle(model);
	check(model.search_results().hits.size() == 1, "Source-specific last scope retained");
	const auto generation = model.search_results().generation.id;
	check(model.clear_search_scope(), "Clear keyed scope");
	settle(model);
	check(model.search_results().generation.id == generation, "Clearing scope retains export generation");
	const auto path = std::filesystem::path(argv[2]);
	std::filesystem::create_directories(path.parent_path());
	check(model.edit_search_list(path.string(), false, "Pair", "KeyDictionary,CaptureBook"), "Explicit named-list save");
	settle(model);
	check(model.edit_search_list(path.string(), true, "Short", "John3:16"), "Save range without applying scope");
	settle(model);
	check(model.saved_lists(false).size() == 1 && model.saved_lists(true).size() == 1 && model.search_results().generation.id == generation, "Prepared snapshots retain both sections and search state");
	check(model.edit_search_list(path.string(), false, "Pair", "", true), "Remove explicit first label");
	settle(model);
	check(model.saved_lists(false).empty() && model.saved_lists(true).size() == 1, "Delete preserves unrelated range section");
	query.scope = XC_SEARCH_SCOPE_ALL;
	query.expression = "compatmarker";
	query.modules = {"ImageThML", "ImageOSIS", "CaptureBook"};
	check(model.search(query), "Unicode resource-bearing keyed query");
	settle(model);
	check(model.search_results().hits.size() == 3, "Three source-specific image entries");
	for (size_t i = 0; i < 3; ++i) {
		check(model.preview_search_hit(i), "Prepared keyed image preview");
		settle(model);
		check(model.search_preview().html.find("data:image/png;base64,") != std::string::npos, "Only copied module-owned image bytes are embedded");
	}
	check(model.export_search(true, true, {}, true, true, true, false), "Opt-in mixed image HTML");
	settle(model);
	check(model.search_export().html.find("data:image/png;base64,") != std::string::npos && !model.search_export().text.empty(), "HTML and plain companion share typed generation");
	std::cout << "Linux mixed keys, exact activation, interval/last scope, named choices and image exports passed\n";
	return 0;
} catch (const std::exception &e) {
	std::cerr << e.what() << '\n';
	return 1;
}
