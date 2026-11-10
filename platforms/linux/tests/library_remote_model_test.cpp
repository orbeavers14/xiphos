// SPDX-License-Identifier: GPL-2.0-or-later
#include "library_model.hpp"
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
void settle(LibraryModel &m, bool success = true)
{
	auto end = std::chrono::steady_clock::now() + std::chrono::seconds(45);
	do {
		m.tick();
		check(std::chrono::steady_clock::now() < end, "Remote model deadline");
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (m.busy());
	if (success)
		check(m.error().empty(), m.error().c_str());
}
int main(int argc, char **argv)
try {
	check(argc == 3, "Loopback TLS fixture required");
	auto root = std::filesystem::path(argv[1]);
	xc_worker *worker = nullptr;
	check(xc_worker_open((root / "installed").c_str(), &worker) == XC_OK, "Shared worker");
	{
		LibraryModel library(*worker);
		auto profile = (root / "repositories.xml").string();
		auto ca = (root / "certificate.pem").string();
		check(library.repositories(profile, true), "Explicit source profile");
		check(library.put_repository({"fixture", "127.0.0.1", "/catalog", "/packages", "Original HTTPS source", unsigned(std::stoul(argv[2])), true}), "Reviewed HTTPS source");
		library.fetch("fixture", root.string(), false, ca);
		settle(library, false);
		check(!library.error().empty(), "Remote acknowledgement required");
		check(library.fetch("fixture", root.string(), true), "System trust probe");
		settle(library, false);
		check(!library.error().empty(), "Untrusted TLS rejects");
		check(library.fetch("fixture", root.string(), true, ca), "Trusted loopback catalog");
		settle(library);
		check(library.items().size() == 1 && library.items()[0].id == "Added", "Copied remote offer");
		check(library.save_repositories(), "Persist cache explicitly");
		settle(library);
		LibraryModel offline(*worker);
		check(offline.repositories(profile), "Read stored cache without network");
		settle(offline);
		check(offline.use_cached(0) && offline.items()[0].id == "Added", "Offline catalog copied");
		check(offline.install_remote("Added", root.string(), false, true, ca), "Install from reviewed cached offer");
		settle(offline);
		check(offline.changed() == 1, "Live library change published");
		check(offline.install_remote("Added", root.string(), false, true, ca), "Next destination fingerprint");
		settle(offline, false);
		check(!offline.error().empty(), "Already installed no-replace refuses");
		check(library.install_remote("Added", root.string(), false, true, ca), "Stale offer submission");
		settle(library, false);
		check(!library.error().empty(), "Stale destination rejects");
		check(offline.catalog(), "Current installed catalog");
		settle(offline);
		bool found = false;
		for (const auto &i : offline.items())
			found |= i.id == "Added";
		check(found, "Installed module visible after SDK reload");
	}
	xc_worker_close(worker);
	std::cout << "Linux HTTPS acknowledgement, TLS trust, cached catalog, installation and stale offer preservation passed\n";
	return 0;
} catch (const std::exception &e) {
	std::cerr << e.what() << '\n';
	return 1;
}
