// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <libxml/parser.h>
#include <memory>
#include <stdexcept>
using namespace xiphos::core;
extern "C" char *legacy_search_list_edit(const char *, int, const char *, const char *);
void check(bool ok, const char *message)
{
	if (!ok)
		throw std::runtime_error(message);
}
template <class F>
void rejects(F call)
{
	bool failed = false;
	try {
		call();
	} catch (const std::exception &) {
		failed = true;
	}
	check(failed, "Expected rejected saved-list edit");
}
std::string bytes(const std::filesystem::path &file)
{
	std::ifstream in(file, std::ios::binary);
	return {std::istreambuf_iterator<char>(in), {}};
}
void write(const std::filesystem::path &file, const std::string &text)
{
	std::ofstream out(file, std::ios::binary);
	out.exceptions(std::ios::failbit | std::ios::badbit);
	out << text;
	out.close();
}
void compare(const std::string &source, const SavedSearchListEdit &edit)
{
	const std::unique_ptr<char, decltype(xmlFree)> expected(legacy_search_list_edit(source.c_str(), static_cast<int>(edit.kind), edit.label.c_str(), edit.value.c_str()), xmlFree);
	check(static_cast<bool>(expected), "Retained C XML writer returns a document");
	const auto actual = serialize_saved_search_list(source, edit);
	check(actual == expected.get(), "Shared named-list edit matches actual retained C XML writer bytes");
	const auto parsed = parse_saved_search_lists(actual);
	const auto &items = edit.kind == SavedSearchListKind::module_list ? parsed.module_lists : parsed.ranges;
	check(saved_search_list_value(items, edit.label) == edit.value, "Literal edited attributes round trip through shared lookup");
}
int main(int argc, char **argv)
{
	try {
		check(argc == 2, "Disposable save fixture required");
		const auto root = std::filesystem::u8path(argv[1]);
		std::filesystem::create_directories(root);
		const std::string source = "<Xiphos version='keep'><!--keep--><other flag='yes'><child>café α</child></other><modlists keep='yes'><modlist label='Original' list='Text' custom='retain'><child>keep</child></modlist><modlist label='Original' list='Shadowed'/><modlist label='' list='Empty label'/><modlist list='Missing label'/><unknown label='Original' list='Ignore'/></modlists><modlists><modlist label='Original' list='Later section'/></modlists><ranges><range label='Original' list='John1:1'/></ranges><locale><special>tr_TR.UTF-8</special></locale></Xiphos>";
		for (auto kind : {SavedSearchListKind::module_list, SavedSearchListKind::range})
			for (const auto &label : {std::string("Original"), std::string(), std::string("café α")})
				for (const auto &value : {std::string(), std::string("Text, MissingModule,Text"), std::string("John1:1 & <original> \"quoted\"\t\n")})
					compare(source, {kind, label, value});
		compare("<Xiphos/>", {SavedSearchListKind::range, "New", "John1:2"});
		compare("<Xiphos><ranges/><ranges><range label='Original' list='Later'/></ranges></Xiphos>", {SavedSearchListKind::range, "Original", "John1:2"});
		SavedSearchListEdit edit{SavedSearchListKind::module_list, "Original", "Text,MissingModule,Text"};
		const auto serialized = serialize_saved_search_list(source, edit);
		check(serialized.find("custom=\"retain\"") != std::string::npos && serialized.find("<child>keep</child>") != std::string::npos && serialized.find("list=\"Shadowed\"") != std::string::npos && serialized.find("Later section") != std::string::npos && serialized.find("<!--keep-->") != std::string::npos && serialized.find("tr_TR.UTF-8") != std::string::npos, "Unknown attributes, children, comments, duplicate sections/items and locale are preserved");
		const auto file = root / std::filesystem::u8path("profile-café.xml");
		write(file, source);
		std::filesystem::permissions(file, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
		const auto permissions = std::filesystem::status(file).permissions();
		const auto saved_snapshot = save_saved_search_list(file.u8string(), edit);
		check(saved_search_list_value(saved_snapshot.module_lists, "Original") == edit.value, "Successful save returns the exact copied staged source");
		check(bytes(file) == serialized && std::filesystem::status(file).permissions() == permissions, "Staged save preserves existing permissions and exact verified document");
		const auto fresh = root / "fresh.xml";
		std::filesystem::remove(fresh);
		save_saved_search_list(fresh.u8string(), {SavedSearchListKind::range, "New", "John1:2"});
		check(saved_search_list_value(read_saved_search_lists(fresh.u8string()).ranges, "New") == "John1:2", "New explicit profile creates root/section/item");
#ifndef _WIN32
		check(std::filesystem::status(fresh).permissions() == (std::filesystem::perms::owner_read | std::filesystem::perms::owner_write), "New profile has private permissions");
#endif
		for (const auto &bad : {std::string("bad\0tail", 8), std::string(1, static_cast<char>(0xff)), std::string(1, '\x01'), std::string(16 * 1024 * 1024 + 1, 'x')}) {
			auto invalid = edit;
			invalid.label = bad;
			rejects([&] { save_saved_search_list(file.u8string(), invalid); });
			check(bytes(file) == serialized, "Invalid label leaves original bytes intact");
			invalid = edit;
			invalid.value = bad;
			rejects([&] { save_saved_search_list(file.u8string(), invalid); });
			check(bytes(file) == serialized, "Invalid value leaves original bytes intact");
		}
		auto invalid = edit;
		invalid.kind = static_cast<SavedSearchListKind>(2);
		rejects([&] { save_saved_search_list(file.u8string(), invalid); });
		check(bytes(file) == serialized, "Invalid kind never modifies destination");
		for (const auto &bad : {std::string(), std::string("<Other/>"), std::string("<Xiphos>"), std::string("<!DOCTYPE Xiphos><Xiphos/>")}) {
			write(fresh, bad);
			rejects([&] { save_saved_search_list(fresh.u8string(), edit); });
			check(bytes(fresh) == bad, "Invalid existing profile is not replaced");
		}
		for (const auto &path : {std::string(), root.u8string(), (root / "missing-parent/profile.xml").u8string(), std::string("bad\0path", 8)})
			rejects([&] { save_saved_search_list(path, edit); });
		check(!std::filesystem::exists(root / "missing-parent"), "Missing parent is not silently created");
#ifndef _WIN32
		std::filesystem::permissions(file, std::filesystem::perms::owner_read);
		rejects([&] { save_saved_search_list(file.u8string(), edit); });
		check(bytes(file) == serialized, "Read-only profile remains intact");
		std::filesystem::permissions(file, permissions);
		const auto link = root / "alias.xml";
		std::filesystem::remove(link);
		std::filesystem::create_symlink(file.filename(), link);
		save_saved_search_list(link.u8string(), edit);
		check(std::filesystem::is_symlink(link) && bytes(file) == serialized, "Existing symlink alias and target are preserved");
		std::filesystem::remove(link);
#endif
		std::string encoded = "\xff\xfe";
		for (char16_t c : std::u16string(u"<?xml version='1.0' encoding='UTF-16'?><Xiphos><other>café α</other></Xiphos>")) {
			encoded.push_back(static_cast<char>(c & 255));
			encoded.push_back(static_cast<char>(c >> 8));
		}
		const auto utf8 = serialize_saved_search_list(encoded, {SavedSearchListKind::range, "café α", "John1:2"});
		check(utf8.find("encoding=\"UTF-8\"") != std::string::npos && utf8.find("<other>café α</other>") != std::string::npos, "Declared source encoding normalizes without losing unrelated Unicode");
		std::string capped = "<Xiphos><ranges>";
		for (std::size_t i = 0; i < maximum_saved_search_lists; ++i)
			capped += "<range label='Original' list='John1:1'/>";
		capped += "</ranges></Xiphos>";
		check(parse_saved_search_lists(serialize_saved_search_list(capped, {SavedSearchListKind::range, "Original", "John1:2"})).ranges.size() == maximum_saved_search_lists, "Editing an existing maximum collection remains valid");
		rejects([&] { serialize_saved_search_list(capped, {SavedSearchListKind::range, "Overflow", "John1:2"}); });
		{
			StudyWorker worker(root.u8string());
			const auto tab = worker.create_tab().get();
			auto path = file.u8string();
			auto copied = edit;
			copied.value = "Captured";
			auto task = worker.save_search_list(tab, path, copied);
			path.clear();
			copied.value = "Changed";
			const auto copied_snapshot = task.get();
			check(saved_search_list_value(copied_snapshot.module_lists, "Original") == "Captured", "Worker returns the staged committed snapshot without a second frontend read");
			check(saved_search_list_value(read_saved_search_lists(file.u8string()).module_lists, "Original") == "Captured" && !worker.search_generation(tab).get().id && worker.history(tab).get().entries.empty(), "Worker copies save arguments and leaves search/history unchanged");
			worker.close_tab(tab).get();
			rejects([&] { (void)worker.save_search_list(tab, file.u8string(), edit).get(); });
			check(saved_search_list_value(read_saved_search_lists(file.u8string()).module_lists, "Original") == "Captured", "Closed-tab save fails before file mutation");
			const auto live = worker.create_tab().get();
			worker.save_search_list(live, file.u8string(), {SavedSearchListKind::range, "Drain", "John1:3"});
		}
		check(saved_search_list_value(read_saved_search_lists(file.u8string()).ranges, "Drain") == "John1:3", "Dropped observer does not cancel accepted FIFO save before shutdown");
		for (const auto &entry : std::filesystem::directory_iterator(root))
			check(entry.path().filename().u8string().find(".xiphos-preferences-") != 0, "Staged directories cleaned after success and failure");
		std::cout << "Legacy named-list writes, preserved XML, staged validation, encoding/permissions, failure retention and FIFO draining passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
