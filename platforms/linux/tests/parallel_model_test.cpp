// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
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
		check(std::chrono::steady_clock::now() < end, "Parallel model deadline");
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (m.busy());
	if (success)
		check(m.error().empty(), m.error().c_str());
}
int main(int argc, char **argv)
try {
	check(argc == 3, "Generated parallel modules and scratch profile required");
	ReaderModel m(argv[1]);
	settle(m);
	check(m.visit("TextA", "Notes", "John3:16"), "Main reading");
	settle(m);
	const auto main = m.passage();
	const auto back = m.can_back();
	check(m.visit_parallel({"TextA", "Missing", "TextA", "RtlText"}, "John3:16"), "Visit explicit columns");
	settle(m);
	const auto held = m.parallel();
	check(held.columns.size() == 4 && held.columns[1].status == XC_PARALLEL_MISSING && held.columns[3].rtl && !held.rows.empty(), "Copied missing/duplicate/RTL columns");
	check(held.rows[15].cells[0].html.find("Original TextA parallel prose") != std::string::npos && held.rows[15].cells[0].html == held.rows[15].cells[2].html, "Core aligns complete duplicated cells");
	check(m.navigate_parallel(1), "Independent chapter navigation");
	settle(m);
	check(m.parallel().reference != held.reference && m.passage().reference == main.reference && m.can_back() == back, "Parallel navigation preserves main reading/history");
	check(m.visit_parallel({"Missing"}, "John3:16"), "Queue invalid control");
	settle(m, false);
	check(!m.error().empty() && m.parallel().columns[0].module == "TextA", "Failed visit retains prior matrix");
	const auto path = std::filesystem::path(argv[2]);
	std::filesystem::create_directories(path.parent_path());
	std::filesystem::remove(path);
	check(m.new_parallel_profile(path.string()), "Prepare create-only profile");
	check(m.edit_parallel_set(XC_PARALLEL_PROFILE_CREATE, "Original Set", "", {"TextA", "Missing", "TextA"}, true), "Checked new set");
	settle(m);
	check(m.parallel_profile().sets.size() == 1 && m.parallel_profile().sets[0].modules.size() == 3, "Owned saved set preserves duplicates and missing slots");
	const auto set = m.parallel_profile().sets[0];
	check(m.open_parallel_set(set.key, "John3:16"), "Apply exact saved set");
	settle(m);
	check(m.parallel().columns.size() == 3, "Profile application owns matrix");
	const auto revision = m.parallel_profile().revision;
	{
		std::ofstream out(path, std::ios::app);
		out << "\n<!-- external writer -->\n";
	}
	check(m.edit_parallel_set(XC_PARALLEL_PROFILE_SAVE, set.key, "", {"TextA"}), "Submit stale replacement");
	settle(m, false);
	check(!m.error().empty() && m.parallel_profile().revision == revision, "Stale repository edit preserves prior snapshot and external data");
	check(m.load_parallel_profile(path.string()), "Reload explicit changed profile");
	settle(m);
	check(m.edit_parallel_set(XC_PARALLEL_PROFILE_RENAME, set.key, "Renamed Set", {}), "Shared name codec rename");
	settle(m);
	check(m.parallel_profile().sets[0].name == "Renamed Set", "Renamed display name copied");
	std::cout << "Linux parallel matrix, missing/RTL/duplicate slots, independent navigation and stale-safe saved sets passed\n";
	return 0;
} catch (const std::exception &e) {
	std::cerr << e.what() << '\n';
	return 1;
}
