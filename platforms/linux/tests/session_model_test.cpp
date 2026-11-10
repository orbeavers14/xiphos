// SPDX-License-Identifier: GPL-2.0-or-later
#include "session_model.hpp"
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
void settle(ReaderModel &m)
{
	auto end = std::chrono::steady_clock::now() + std::chrono::seconds(30);
	do {
		m.tick();
		check(std::chrono::steady_clock::now() < end, "Restore model deadline");
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (m.busy());
	check(m.error().empty(), m.error().c_str());
}
void settle(SessionModel &m)
{
	auto end = std::chrono::steady_clock::now() + std::chrono::seconds(30);
	do {
		m.tick();
		check(std::chrono::steady_clock::now() < end, "Session deadline");
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (m.busy());
}
int main(int argc, char **argv)
try {
	check(argc == 3, "Generated modules and scratch session required");
	const auto file = std::filesystem::path(argv[2]);
	std::filesystem::create_directories(file.parent_path());
	xc_worker *worker = nullptr;
	check(xc_worker_open(argv[1], &worker) == XC_OK, "Shared worker");
	{
		ReaderModel a(*worker), b(*worker);
		settle(a);
		settle(b);
		SessionModel session(*worker);
		check(a.visit("Text", "", "John3:16"), "First passage");
		settle(a);
		check(a.visit("Text", "", "John3:17"), "History visit");
		settle(a);
		check(a.navigate(-1), "History cursor");
		settle(a);
		check(b.choose_book("CaptureBook"), "Book selection");
		settle(b);
		check(b.read_book_key("/Original"), "Exact book path");
		settle(b);
		check(b.lookup_dictionary("KeyDictionary", "ALPHA", true), "Dictionary selection/history");
		settle(b);
		check(session.save(file.string(), {{&a, "reading", "original host", false}, {&b, "book", "", false}}, "reading"), "Capture complete session");
		settle(session);
		check(session.error().empty(), "Verified session save");
		const auto old = a.tab_id();
		check(session.load(file.string()), "Atomic restore");
		settle(session);
		check(session.error().empty() && session.restored().size() == 2 && session.restored()[0].tab != old, "Fresh owned tab mappings");
		a.detach();
		b.detach();
		ReaderModel ra(*worker, session.restored()[0]), rb(*worker, session.restored()[1]);
		settle(ra);
		settle(rb);
		check(ra.passage().reference == "John 3:16" && ra.can_forward(), "Restored reading/history cursor not revisited");
		check(rb.book().nodes[0].path == "/Original" && rb.dictionary().key == "ALPHA" && rb.dictionary_can_back() == false, "Restored book/dictionary presentation");
		check(ra.navigate(1), "Use restored history");
		settle(ra);
		check(ra.passage().reference == "John 3:17", "Retained forward target");
		session.validate_host = [](const std::string &host) { return host.empty(); };
		const auto version = session.restored_version();
		check(session.load(file.string()), "Preflight bad host metadata");
		settle(session);
		check(!session.error().empty() && session.restored_version() == version, "Host validation fails before core replacement");
		check(ra.refresh(), "Rejected load leaves existing tab live");
		settle(ra);
		check(session.save(file.string(), {}, ""), "Persist explicit empty session");
		settle(session);
		check(session.error().empty(), "Empty save complete");
		ra.detach();
		rb.detach();
		check(session.load(file.string()), "Restore empty session");
		settle(session);
		check(session.error().empty() && session.restored().empty(), "Closed windows are not resurrected");
	}
	xc_worker_close(worker);
	std::cout << "Linux atomic session capture, fresh IDs, presentation bootstrap, histories and empty sessions passed\n";
	return 0;
} catch (const std::exception &e) {
	std::cerr << e.what() << '\n';
	return 1;
}
