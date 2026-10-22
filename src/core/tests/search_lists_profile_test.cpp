// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <libxml/parser.h>
#include <stdexcept>
using namespace xiphos::core;
#define _(value) (value)
#define XI_message(...) ((void)0)
static xmlDocPtr xml_settings_doc;
#include "legacy_xml_profile_actual.inc"
#include "legacy_xml_search_lists_actual.inc"
void check(bool ok, const char *message)
{
	if (!ok)
		throw std::runtime_error(message);
}
template <class F>
void rejects(F call)
{
	try {
		call();
	} catch (const std::invalid_argument &) {
		return;
	}
	throw std::runtime_error("Expected saved search input rejection");
}
std::optional<std::string> original(const char *section, const char *item, const std::optional<std::string> &label)
{
	auto *text = xml_get_list_from_label(section, item, label ? label->c_str() : nullptr);
	if (!text)
		return std::nullopt;
	std::string result(text);
	xmlFree(text);
	return result;
}
SavedSearchLists compare(const std::string &source)
{
	xml_settings_doc = xmlReadMemory(source.data(), static_cast<int>(source.size()), nullptr, nullptr, XML_PARSE_NONET);
	check(xml_settings_doc, "Original XML fixture parses");
	const auto lists = parse_saved_search_lists(source);
	for (const auto &label : {std::optional<std::string>{}, std::optional<std::string>{""}, std::optional<std::string>{"Original"}, std::optional<std::string>{"café α"}, std::optional<std::string>{"missing"}}) {
		check(saved_search_list_value(lists.module_lists, label) == original("modlists", "modlist", label), "Named module lookup matches actual legacy first-label behavior");
		check(saved_search_list_value(lists.ranges, label) == original("ranges", "range", label), "Named range lookup matches actual legacy first-label behavior");
	}
	xmlFreeDoc(xml_settings_doc);
	xml_settings_doc = nullptr;
	return lists;
}
std::string bytes(const std::filesystem::path &file)
{
	std::ifstream input(file, std::ios::binary);
	return {std::istreambuf_iterator<char>(input), {}};
}
int main(int argc, char **argv)
{
	try {
		check(argc == 2, "Disposable profile directory required");
		const auto root = std::filesystem::u8path(argv[1]);
		std::filesystem::create_directories(root);
		check(compare("<Xiphos/>").module_lists.empty(), "Absent lists stay empty");
		const std::string source = "<Xiphos><!--preserve--><modlists><modlist label='Original' list='Text,, Commentary,Text,'/><modlist label='Original' list='Shadowed'/><modlist label=''/><modlist list='Unlabelled'/><modlist label='café α' list='MissingModule'/><other label='ignored' list='ignored'/></modlists><modlists><modlist label='Later' list='Ignored'/></modlists><ranges><range label='Original' list='John 1:1-2'/><range label='Original' list='Shadowed'/><range label='' list=''/><range/><range label='café α' list=' Gen - Mal '/></ranges><unknown keep='yes'/></Xiphos>";
		const auto lists = compare(source);
		check(lists.module_lists.size() == 5 && lists.ranges.size() == 5 && lists.module_lists[1].value == "Shadowed" && !lists.module_lists[2].value && lists.module_lists[3].label == std::nullopt && lists.ranges[2].value == "", "Raw owned order, duplicates and missing/empty attributes survive import");
		check(compare("<Xiphos><modlists/><modlists><modlist label='Original' list='Ignored'/></modlists><ranges><range label='Original'/><range label='Original' list='Shadowed'/></ranges></Xiphos>").module_lists.empty(), "First empty section and first absent value shadow later duplicates");
		compare("<Xiphos><modlists><modlist label='Original' list='Text &amp; More'/></modlists><ranges><range label='café α' list='John&#32;1:1'/></ranges></Xiphos>");
		auto module_text = *saved_search_list_value(lists.module_lists, "Original");
		check(saved_search_module_names(module_text) == std::vector<std::string>{"Text", " Commentary", "Text"}, "Comma split skips empty fields without trimming, sorting, deduplication or installed-module lookup");
		check(saved_search_module_names("").empty() && saved_search_module_names(",,,").empty() && saved_search_module_names(" ,MissingModule, ") == std::vector<std::string>{" ", "MissingModule", " "}, "Empty fields and literal spaces retain legacy token behavior safely");
		std::string maximum = "Text";
		for (std::size_t i = 1; i < maximum_search_batch_modules; ++i)
			maximum += ",Text";
		check(saved_search_module_names(maximum).size() == maximum_search_batch_modules, "Maximum shared batch is accepted");
		rejects([&] { saved_search_module_names(maximum + ",Text"); });
		rejects([&] { saved_search_module_names(std::string(4097, 'x')); });
		rejects([&] { saved_search_module_names(std::string("Text\0,Hidden", 12)); });
		rejects([&] { saved_search_module_names(std::string(1, static_cast<char>(0xff))); });
		rejects([&] { saved_search_module_names(std::string(maximum_saved_search_text_bytes + 1, ',')); });
		std::string capped = "<Xiphos><ranges>";
		for (std::size_t i = 0; i < maximum_saved_search_lists; ++i)
			capped += "<range label='Original' list='John1:1'/>";
		check(parse_saved_search_lists(capped + "</ranges></Xiphos>").ranges.size() == maximum_saved_search_lists, "Bounded maximum named-list count");
		rejects([&] { parse_saved_search_lists(capped + "<range/></ranges></Xiphos>"); });
		std::string encoded = "\xff\xfe";
		for (char16_t c : std::u16string(u"<?xml version='1.0' encoding='UTF-16'?><Xiphos><ranges><range label='café α' list='John1:1'/></ranges></Xiphos>")) {
			encoded.push_back(static_cast<char>(c & 255));
			encoded.push_back(static_cast<char>(c >> 8));
		}
		check(compare(encoded).ranges[0].label == "café α", "Declared encoding becomes copied UTF-8");
		const auto file = root / std::filesystem::u8path("search-café.xml");
		{
			std::ofstream out(file, std::ios::binary);
			out << source;
		}
		const auto permissions = std::filesystem::status(file).permissions();
		check(read_saved_search_lists(file.u8string()).module_lists.size() == 5 && bytes(file) == source && std::filesystem::status(file).permissions() == permissions, "Explicit file read preserves bytes and permissions");
		std::future<SavedSearchLists> drained;
		{
			StudyWorker worker(root.u8string());
			const auto tab = worker.create_tab().get();
			const auto before = worker.search_generation(tab).get();
			const auto history = worker.history(tab).get();
			auto path = file.u8string();
			auto copied = worker.read_search_lists(tab, path);
			path.clear();
			const auto loaded = copied.get();
			check(loaded.module_lists[4].value == "MissingModule" && worker.search_generation(tab).get().id == before.id && worker.history(tab).get().entries.size() == history.entries.size(), "Queued copied read leaves search/history unchanged");
			worker.close_tab(tab).get();
			bool missing = false;
			try {
				(void)worker.read_search_lists(tab, file.u8string()).get();
			} catch (const std::invalid_argument &) {
				missing = true;
			}
			check(missing, "Closed tab rejects queued profile read");
			const auto live = worker.create_tab().get();
			drained = worker.read_search_lists(live, file.u8string());
			worker.close_tab(live).get();
		}
		std::filesystem::remove(file);
		check(drained.get().ranges[4].value == " Gen - Mal " && lists.module_lists[4].value == "MissingModule", "Accepted read drains before close/shutdown and copied values outlive source file");
		for (const auto &path : {std::string(), root.u8string(), file.u8string(), std::string("bad\0path", 8)})
			rejects([&] { read_saved_search_lists(path); });
		for (const auto &bad : {std::string(), std::string("<Other/>"), std::string("<Xiphos>"), std::string("<!DOCTYPE Xiphos SYSTEM 'https://example.test/profile'><Xiphos/>"), std::string("<Xiphos/>\0hidden", 16), std::string(16 * 1024 * 1024 + 1, 'x')})
			rejects([&] { parse_saved_search_lists(bad); });
		std::future<SavedSearchLists> jobs[4];
		for (unsigned i = 0; i < 4; ++i)
			jobs[i] = std::async(std::launch::async, [&] { return parse_saved_search_lists(source); });
		for (auto &job : jobs)
			check(job.get().ranges.size() == 5, "Independent parser contexts share no traversal cursor");
		std::cout << "Legacy named search lists, first-match lookup, bounded module splitting, read-only XML and FIFO ownership passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
