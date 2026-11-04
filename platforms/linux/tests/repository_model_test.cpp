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
		check(std::chrono::steady_clock::now() < end, "Repository model deadline");
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (m.busy());
	if (success)
		check(m.error().empty(), m.error().c_str());
}
int main(int argc, char **argv)
try {
	check(argc == 3, "Generated modules and private scratch required");
	namespace fs = std::filesystem;
	const fs::path scratch(argv[2]);
	fs::create_directories(scratch);
	ReaderModel m(argv[1]);
	settle(m);
	check(m.visit("Text", "", "John3:16"), "Read verse");
	settle(m);
	auto marks = scratch / "marks.xml", notes = scratch / "notes.xml", file = scratch / "source.html";
	for (const auto &p : {marks, notes, file})
		fs::remove(p);
	check(m.open_bookmarks(marks.string(), true), "Explicit create-only bookmark draft");
	std::array<std::optional<std::string>, 6> fields;
	fields[0] = "Original folder";
	fields[1] = "#123456";
	check(m.edit_bookmark(XC_BOOKMARK_INSERT, XC_BOOKMARK_FOLDER, {}, {}, 0, fields), "Save folder");
	settle(m);
	fields = {};
	fields[2] = "Text";
	fields[3] = "John3:16";
	fields[5] = "Original verse";
	check(m.edit_bookmark(XC_BOOKMARK_INSERT, XC_BOOKMARK_LEAF, {}, 0, 0, fields), "Nested bookmark");
	settle(m);
	check(m.bookmarks().items.size() == 2 && m.bookmarks().items[1].parent == 0, "Hierarchy retained");
	check(m.open_bookmark(1), "Open saved verse");
	settle(m);
	check(m.passage().reference == "John 3:16", "Bookmark routes through core");
	{
		std::ofstream out(marks, std::ios::app);
		out << "\n<!-- external writer -->\n";
	}
	check(m.edit_bookmark(XC_BOOKMARK_REMOVE, XC_BOOKMARK_LEAF, 1, {}, 0, {}), "Queue stale delete");
	settle(m, false);
	check(!m.error().empty() && m.bookmarks().items.size() == 2, "Stale delete preserves snapshot");
	check(m.open_bookmarks(marks.string()), "Reload changed bookmarks");
	settle(m);
	check(m.open_annotations(notes.string(), true) && m.begin_note(false), "Capture SDK-owned note target");
	settle(m);
	const auto target = m.note_draft();
	check(!target.label.empty() && target.reference == "John 3:16", "Canonical note source retained");
	check(m.visit("Text", "", "John3:17"), "Navigate while editing captured note");
	settle(m);
	check(m.save_note("Original café note <literal>"), "Save captured note");
	settle(m);
	check(m.note_draft().reference == target.reference && m.annotations().items.size() == 1, "Later reading cannot redirect pending note source");
	check(m.visit("Text", "", "John3:16"), "Return to annotated verse");
	settle(m);
	check(m.decorate_reading(), "Shared bookmark/note decorations");
	settle(m);
	check(m.decorated().html.find("Original café note") != std::string::npos, "Owned reading shows saved annotation");
	{
		std::ofstream out(notes, std::ios::app);
		out << "\n<!-- external note writer -->\n";
	}
	check(m.save_note("Stale overwrite"), "Queue stale note save");
	settle(m, false);
	check(!m.error().empty() && m.note_draft().text.find("Original café") != std::string::npos, "Stale note save preserves draft and external source");
	check(m.open_editor(file.string(), true) && m.change_editor("<p>Original café source</p>"), "Create checked source buffer");
	check(m.editor_dirty(), "Dirty state comes from shared buffer");
	check(m.format_editor(0, 8, XC_RICH_BOLD, 1), "Shared rich formatting uses UTF8 byte offsets");
	check(m.editor().text.find("<b>") != std::string::npos || m.editor().text.find("<strong>") != std::string::npos, "Formatting changes owned HTML");
	check(m.editor_undo() && m.editor().text == "<p>Original café source</p>", "Shared undo preserves exact source baseline");
	check(m.save_editor(), "Save new source file");
	settle(m);
	check(!m.editor_dirty() && m.editor().revision, "Verified file result marks source clean");
	check(m.change_editor("Original changed draft"), "Edit retained buffer");
	{
		std::ofstream out(file);
		out << "Original external edit";
	}
	check(m.save_editor(), "Queue stale editor save");
	settle(m, false);
	check(!m.error().empty() && m.editor_dirty() && m.editor().text == "Original changed draft", "Failed save retains unsaved draft");
	check(!m.open_editor((scratch / "other.html").string(), true), "Dirty source cannot be replaced by another document");
	std::cout << "Linux bookmarks, captured verse notes, decorated reading, revision conflicts, rich source and undo passed\n";
	return 0;
} catch (const std::exception &e) {
	std::cerr << e.what() << '\n';
	return 1;
}
