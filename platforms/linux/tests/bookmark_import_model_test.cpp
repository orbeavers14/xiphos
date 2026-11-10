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
		check(std::chrono::steady_clock::now() < end, "Import deadline");
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (m.busy());
	if (success)
		check(m.error().empty(), m.error().c_str());
}
std::string bytes(const std::filesystem::path &p)
{
	std::ifstream f(p, std::ios::binary);
	return {std::istreambuf_iterator<char>(f), {}};
}
int main(int argc, char **argv)
try {
	check(argc == 4, "Library, original backup and scratch required");
	namespace fs = std::filesystem;
	fs::path root(argv[3]);
	fs::remove_all(root);
	fs::create_directories(root);
	const auto backup = bytes(argv[2]);
	ReaderModel model(argv[1]);
	settle(model);
	auto destination = (root / "bookmarks.xml").string();
	check(model.open_bookmarks(destination, true), "New explicit destination");
	check(model.preview_andbible(argv[2]), "Read-only backup preview");
	settle(model);
	check(model.bookmark_import().ready && model.bookmark_import().source.imported > 0, "Counts and collision review copied");
	check(model.merge_bookmarks("Original import", XC_BOOKMARK_IMPORT_REJECT), "Apply reviewed import");
	settle(model);
	check(!model.bookmarks().items.empty(), "Saved hierarchy copied");
	check(bytes(argv[2]) == backup, "Original backup unchanged");
	check(model.preview_andbible(argv[2]), "Repeat review");
	settle(model);
	check(model.bookmark_import().review.identical > 0, "Stable identities reported");
	check(model.merge_bookmarks("Repeated", XC_BOOKMARK_IMPORT_REJECT), "Explicit reject policy");
	settle(model, false);
	check(!model.error().empty(), "Collision rejection preserves file");
	check(model.preview_andbible(argv[2]), "Review retry");
	settle(model);
	check(model.merge_bookmarks("Repeated", XC_BOOKMARK_IMPORT_SKIP_IDENTICAL), "Explicit skip-identical policy");
	settle(model);
	const auto exact = bytes(destination);
	check(model.export_bookmarks((root / "export.xml").string()), "Exact XML export");
	settle(model);
	check(bytes(root / "export.xml") == exact, "Export preserves exact stored bytes");
	check(model.preview_andbible(argv[2]), "Review destination revision");
	settle(model);
	{
		std::ofstream out(destination, std::ios::app);
		out << "<!--external-->";
	}
	check(model.merge_bookmarks("Stale", XC_BOOKMARK_IMPORT_KEEP_EXISTING), "Queue stale merge");
	settle(model, false);
	check(!model.error().empty() && bytes(destination) == exact + "<!--external-->", "Stale merge does not overwrite external data");
	std::cout << "Linux AndBible review, collision policies, exact export and stale merge protection passed\n";
	return 0;
} catch (const std::exception &e) {
	std::cerr << e.what() << '\n';
	return 1;
}
