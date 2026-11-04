// SPDX-License-Identifier: GPL-2.0-or-later
#include "library_model.hpp"
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
template <class T>
void settle(T &m, bool success = true)
{
	auto end = std::chrono::steady_clock::now() + std::chrono::seconds(30);
	do {
		m.tick();
		check(std::chrono::steady_clock::now() < end, "Library model deadline");
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
	check(argc == 3, "Generated fixture and scratch required");
	namespace fs = std::filesystem;
	fs::path root(argv[2]);
	fs::remove_all(root);
	fs::create_directories(root / "installed/mods.d");
	fs::copy(argv[1], root / "source", fs::copy_options::recursive);
	xc_worker *worker = nullptr;
	check(xc_worker_open((root / "installed").c_str(), &worker) == XC_OK, "Shared worker");
	{
		LibraryModel library(*worker);
		ReaderModel reader(*worker);
		settle(reader);
		check(library.catalog((root / "source").string()), "Inspect local library");
		settle(library);
		check(!library.items().empty(), "Copied source catalog");
		check(library.prepare_install((root / "source").string(), "Text"), "Local install review");
		settle(library);
		check(library.install_review().module == "Text" && !library.install_review().source.empty(), "Full-tree fingerprints copied");
		check(library.install(false), "Install reviewed source");
		settle(library);
		check(library.changed() == 1, "Library changed signal");
		check(reader.reload_modules(), "Refresh model catalog");
		settle(reader);
		check(reader.visit("Text", "", "John3:16"), "Installed module reads through existing process worker");
		settle(reader);
		check(library.review_maintenance("Text"), "Maintenance preview");
		settle(library);
		check(!library.maintenance().revision.empty(), "Maintenance fingerprint");
		check(library.maintain(0), "Try removing live history module");
		settle(library, false);
		check(!library.error().empty() && fs::exists(root / "installed/mods.d/Text.conf"), "In-use module removal rejects");
		const auto organization = (root / "organization.xml").string();
		check(library.organization(organization, true), "New organization profile");
		check(library.organize({"sword", "Text", "Original Ω group", true, true}), "Persist provider-scoped organization");
		settle(library);
		check(library.groups().size() == 1 && library.groups()[0].favorite && library.groups()[0].hidden, "Organization copied");
		LibraryModel stale(*worker);
		check(stale.organization(organization), "Second writer");
		settle(stale);
		check(library.organize({"sword", "Text", "Changed", true, false}), "First writer update");
		settle(library);
		check(stale.organize({"sword", "Text", "Stale", false, false}), "Stale update queued");
		settle(stale, false);
		check(!stale.error().empty() && stale.groups()[0].group == "Original Ω group", "Stale organization retains copied snapshot");
		// Read-only explicit migration, mapping and exact backups; no home discovery.
		auto settings = root / "settings.xml";
		{
			std::ofstream out(settings);
			out << "<Xiphos><modules><bible>Absent</bible></modules><keys><verse>John 3:16</verse></keys><future><opaque>Original Ω</opaque></future></Xiphos>";
		}
		check(library.prepare_migration(settings.string(), "", ""), "Review unavailable legacy module");
		settle(library);
		check(library.migration_review().find("Absent") != std::string::npos, "Migration issues copied");
		check(library.prepare_migration(settings.string(), "", "", {{"Absent", "Text"}}), "Explicit mapping");
		settle(library);
		check(library.migration_review().find("Ready for explicit export") != std::string::npos, "Mapped migration ready");
		check(library.write_migration((root / "backup.bundle").string(), 0), "Save exact backup");
		settle(library);
		check(library.write_migration((root / "converted.xml").string(), 1), "Export converted session");
		settle(library);
		check(fs::exists(root / "converted.xml"), "Session output exists");
		check(library.read_migration((root / "backup.bundle").string(), {{"Absent", "Text"}}), "Read backup");
		settle(library);
		check(library.write_migration((root / "restored").string(), 2), "Restore sources to new folder");
		settle(library);
		check(bytes(root / "restored/settings.xml") == bytes(settings), "Exact source bytes preserved");
		fs::create_directories(root / "assets/empty");
		{
			std::ofstream out(root / "assets/original.pad");
			out << "Original café";
		}
		check(library.inspect_assets((root / "assets").string()), "Review legacy assets");
		settle(library);
		check(library.copy_assets((root / "copied").string()), "Copy reviewed assets");
		settle(library);
		check(bytes(root / "copied/original.pad") == "Original café" && fs::is_directory(root / "copied/empty"), "Unicode and empty directories retained");
		check(library.repositories((root / "sources.xml").string(), true), "New source profile");
		LibraryEndpoint endpoint{"original", "example.invalid", "/catalog", "/packages", "Original source", 443, false};
		check(library.put_repository(endpoint), "Reviewed disabled source");
		check(library.save_repositories(), "Save source profile");
		settle(library);
		check(!library.fetch("original", root.string(), true), "Disabled source cannot access network");
	}
	xc_worker_close(worker);
	std::cout << "Linux local install, maintenance protection, organization conflicts, migration and assets passed\n";
	return 0;
} catch (const std::exception &e) {
	std::cerr << e.what() << '\n';
	return 1;
}
