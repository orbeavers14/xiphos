// SPDX-License-Identifier: GPL-2.0-or-later
#include "recovery_model.hpp"
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
template <class Model>
void settle(Model &m)
{
	auto end = std::chrono::steady_clock::now() + std::chrono::seconds(30);
	do {
		m.tick();
		check(std::chrono::steady_clock::now() < end, "Recovery deadline");
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (m.busy());
}
int main(int argc, char **argv)
try {
	check(argc == 3, "Fixture and scratch path required");
	auto dir = std::filesystem::path(argv[2]);
	std::filesystem::remove_all(dir);
	std::filesystem::create_directories(dir);
	auto path = (dir / "recovery").string(), source = (dir / "source.html").string();
	xc_worker *worker = nullptr;
	check(xc_worker_open(argv[1], &worker) == XC_OK, "Shared worker");
	{
		ReaderModel m(*worker);
		settle(m);
		check(m.open_editor(source, true), "Create source");
		settle(m);
		check(m.change_editor("Saved"), "Source text");
		check(m.save_editor(), "Save source");
		settle(m);
		RecoveryDraft d;
		d.id = "source";
		d.title = "Unicode source";
		d.target = 1;
		d.path = source;
		d.revision = m.editor().revision;
		d.saved = m.editor().text;
		d.text = "Ω unsaved";
		d.start = 0;
		d.end = 2;
		check(m.open_annotations((dir / "notes.xml").string(), true), "Note repository");
		check(m.visit("Text", "", "John3:16"), "Reading context");
		settle(m);
		check(m.begin_note(true), "Canonical generic note");
		settle(m);
		auto n = m.note_draft();
		RecoveryDraft note;
		note.id = "note";
		note.title = "Verse note";
		note.target = 3;
		note.path = n.path;
		note.module = n.module;
		note.reference = n.reference;
		note.label = n.label;
		note.generic = n.generic;
		note.revision = n.revision;
		note.saved = n.text;
		note.text = "Unsaved note Ω";
		RecoveryModel recovery(*worker);
		std::vector<WindowCapture> windows{{&m, "reading", "", false}};
		check(recovery.save(path, windows, "reading", {d, note}), "Capture recovery");
		settle(recovery);
		check(recovery.error().empty(), recovery.error().c_str());
		RecoveryModel reader(*worker);
		check(reader.load(path), "Load explicit recovery");
		settle(reader);
		check(reader.error().empty() && reader.drafts().size() == 2, "Copied source and annotation drafts");
		check(reader.drafts()[0].text == d.text && reader.drafts()[0].end == 2 && reader.drafts()[1].label == note.label && reader.drafts()[1].generic, "Unicode selection and canonical generic label");
		check(m.recover_draft(reader.drafts()[0]), "Recover saved source baseline");
		settle(m);
		check(m.editor_dirty() && m.editor().text == d.text, "Recovered source remains dirty");
		{
			std::ofstream changed(source);
			changed << "External edit";
		}
		check(m.save_editor(), "Try original revision");
		settle(m);
		check(!m.error().empty() && m.editor_dirty() && m.editor().text == d.text, "Stale source retained");
		// Another recovery writer must not replace a changed recovery revision.
		check(recovery.save(path, windows, "reading", {}), "Clear recovery with checked revision");
		settle(recovery);
		check(recovery.error().empty(), "Clear completed");
		check(reader.save(path, windows, "reading", {d}), "Queue stale recovery writer");
		settle(reader);
		check(!reader.error().empty(), "Stale recovery rejects");
	}
	xc_worker_close(worker);
	std::cout << "Linux recovery copies, Unicode selection, note labels and source/recovery conflict preservation passed\n";
	return 0;
} catch (const std::exception &e) {
	std::cerr << e.what() << '\n';
	return 1;
}
