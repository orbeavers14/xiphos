// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
void check(bool ok, const char *message)
{
	if (!ok)
		throw std::runtime_error(message);
}
xc_status wait(xc_request *request)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
	xc_status status;
	while ((status = xc_request_poll(request)) == XC_PENDING) {
		check(std::chrono::steady_clock::now() < deadline, "C named-list deadline");
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return status;
}
int main(int argc, char **argv)
{
	try {
		check(argc == 2, "Disposable C fixture path required");
		const auto root = std::filesystem::u8path(argv[1]);
		std::filesystem::create_directories(root);
		const auto file = root / std::filesystem::u8path("search-café.xml"), bad = root / "bad.xml";
		const std::string source = "<Xiphos><modlists><modlist label='Original' list='Text,, MissingModule,Text,'/><modlist label='Original' list='Shadowed'/><modlist label='' list=''/><modlist list='Unlabelled'/><modlist label='Absent'/><modlist label='Absent' list='Hidden'/></modlists><ranges><range label='café α' list=' John1:1-2 '/><range/></ranges></Xiphos>";
		{
			std::ofstream out(file, std::ios::binary);
			out << source;
		}
		{
			std::ofstream out(bad);
			out << "<Xiphos>";
		}
		xc_worker *worker = nullptr;
		xc_request *request = nullptr;
		check(xc_worker_open(root.u8string().c_str(), &worker) == XC_OK, "Open C worker");
		check(xc_worker_create_tab(worker, &request) == XC_OK && wait(request) == XC_OK, "Create C tab");
		const auto tab = xc_request_tab(request);
		xc_request_release(request);
		auto path = file.u8string();
		check(xc_worker_read_search_lists(worker, tab, path.c_str(), &request) == XC_OK, "Queue selected named-list profile");
		path.clear();
		check(!xc_request_take_search_lists(request) && !xc_request_take_search_module_names(request), "Take requires successful explicit poll");
		check(wait(request) == XC_OK, "Read selected profile");
		auto *lists = xc_request_take_search_lists(request);
		check(lists && !xc_request_take_search_lists(request) && !xc_request_take_lexicons(request), "Typed snapshot takes once only");
		xc_request_release(request);
		check(xc_search_lists_count(lists, XC_SAVED_SEARCH_MODULES) == 6 && xc_search_lists_count(lists, XC_SAVED_SEARCH_RANGES) == 2 &&
			  std::string(xc_search_lists_label(lists, XC_SAVED_SEARCH_RANGES, 0)) == "café α" && std::string(xc_search_lists_value(lists, XC_SAVED_SEARCH_RANGES, 0)) == " John1:1-2 ",
		      "Copied ordered typed groups and Unicode");
		check(!xc_search_lists_label(lists, XC_SAVED_SEARCH_MODULES, 3) && !xc_search_lists_value(lists, XC_SAVED_SEARCH_MODULES, 4) &&
			  std::string(xc_search_lists_label(lists, XC_SAVED_SEARCH_MODULES, 2)).empty() && std::string(xc_search_lists_value(lists, XC_SAVED_SEARCH_MODULES, 2)).empty(),
		      "Absent and explicit-empty attributes remain distinct");
		size_t index = 99;
		check(xc_search_lists_effective_index(lists, XC_SAVED_SEARCH_MODULES, 1, &index) == XC_OK && index == 0, "Duplicate label uses shared first match");
		check(xc_search_lists_effective_index(lists, XC_SAVED_SEARCH_MODULES, 5, &index) == XC_OK && index == 4 && !xc_search_lists_value(lists, XC_SAVED_SEARCH_MODULES, index), "First missing value shadows later duplicate");
		for (int kind : {-1, 2, 99}) {
			index = 99;
			check(xc_search_lists_count(lists, kind) == 0 && !xc_search_lists_label(lists, kind, 0) && !xc_search_lists_value(lists, kind, 0) && xc_search_lists_effective_index(lists, kind, 0, &index) == XC_INVALID_ARGUMENT && index == 0, "Invalid kind has safe zero/NULL/cleared outputs");
		}
		index = 99;
		check(xc_search_lists_effective_index(lists, XC_SAVED_SEARCH_MODULES, 99, &index) == XC_INVALID_ARGUMENT && index == 0 && !xc_search_lists_label(lists, 0, 99) && !xc_search_lists_value(lists, 0, 99), "Invalid row is guarded");
		check(xc_search_lists_effective_index(lists, 0, 0, nullptr) == XC_INVALID_ARGUMENT && !xc_search_lists_count(nullptr, 0) && !xc_search_lists_label(nullptr, 0, 0) && !xc_search_lists_value(nullptr, 0, 0), "Null getter inputs are guarded");
		std::string text = xc_search_lists_value(lists, XC_SAVED_SEARCH_MODULES, 0);
		check(xc_worker_parse_search_module_list(worker, tab, text.c_str(), &request) == XC_OK, "Queue shared module splitting");
		text.clear();
		check(!xc_request_take_search_module_names(request) && wait(request) == XC_OK && !xc_request_take_search_lists(request), "Copied tokenization has explicit typed completion");
		auto *names = xc_request_take_search_module_names(request);
		check(names && !xc_request_take_search_module_names(request), "Names transfer once");
		xc_request_release(request);
		check(xc_search_module_names_count(names) == 3 && std::string(xc_search_module_names_at(names, 1)) == " MissingModule" && std::string(xc_search_module_names_at(names, 2)) == "Text" && !xc_search_module_names_at(names, 3), "Shared token order, literal spaces and duplicates");
		for (const char *empty : {"", ",,,"}) {
			check(xc_worker_parse_search_module_list(worker, tab, empty, &request) == XC_OK && wait(request) == XC_OK, "Empty module list accepted without all-modules implication");
			auto *zero = xc_request_take_search_module_names(request);
			check(zero && xc_search_module_names_count(zero) == 0, "Empty owned snapshot");
			xc_search_module_names_release(zero);
			xc_request_release(request);
		}
		for (const auto &invalid : {std::string(4097, 'x'), std::string(1, static_cast<char>(0xff))}) {
			check(xc_worker_parse_search_module_list(worker, tab, invalid.c_str(), &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT && !xc_request_take_search_module_names(request), "Malformed names reject on owner without payload");
			xc_request_release(request);
		}
		for (const auto &invalid : {bad.u8string(), (root / "missing.xml").u8string()}) {
			check(xc_worker_read_search_lists(worker, tab, invalid.c_str(), &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT && !xc_request_take_search_lists(request), "Bad selected profile fails without snapshot");
			xc_request_release(request);
		}
		for (const char *invalid : {static_cast<const char *>(nullptr), ""}) {
			request = reinterpret_cast<xc_request *>(1);
			check(xc_worker_read_search_lists(worker, tab, invalid, &request) == XC_INVALID_ARGUMENT && !request, "Bad path clears observer before queueing");
		}
		request = reinterpret_cast<xc_request *>(1);
		check(xc_worker_parse_search_module_list(worker, tab, nullptr, &request) == XC_INVALID_ARGUMENT && !request, "NULL text is not empty text");
		request = reinterpret_cast<xc_request *>(1);
		check(xc_worker_read_search_lists(nullptr, tab, file.u8string().c_str(), &request) == XC_INVALID_ARGUMENT && !request, "NULL worker clears output");
		const std::string oversized(16 * 1024 * 1024 + 1, ',');
		request = reinterpret_cast<xc_request *>(1);
		check(xc_worker_parse_search_module_list(worker, tab, oversized.c_str(), &request) == XC_INVALID_ARGUMENT && !request, "Oversized text rejects before copying/queueing");
		check(xc_worker_read_search_lists(worker, tab, file.u8string().c_str(), nullptr) == XC_INVALID_ARGUMENT, "NULL observer rejects submission");
		check(xc_worker_close_tab(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "Close C tab");
		xc_request_release(request);
		check(xc_worker_read_search_lists(worker, tab, file.u8string().c_str(), &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT && !xc_request_take_search_lists(request), "Closed-tab read fails on owner");
		xc_request_release(request);
		check(xc_worker_create_tab(worker, &request) == XC_OK && wait(request) == XC_OK, "Create drain tab");
		const auto live = xc_request_tab(request);
		xc_request_release(request);
		check(xc_worker_read_search_lists(worker, live, file.u8string().c_str(), &request) == XC_OK, "Queue drain read");
		xc_worker_close(worker);
		check(wait(request) == XC_OK, "Worker close drains accepted read");
		auto *drained = xc_request_take_search_lists(request);
		xc_request_release(request);
		std::filesystem::remove(file);
		std::filesystem::remove(bad);
		check(std::string(xc_search_lists_value(drained, XC_SAVED_SEARCH_RANGES, 0)) == " John1:1-2 " && std::string(xc_search_module_names_at(names, 0)) == "Text" && std::string(xc_search_lists_label(lists, 0, 0)) == "Original", "Owned snapshots outlive request/worker/source files");
		xc_search_lists_release(drained);
		xc_search_lists_release(lists);
		xc_search_module_names_release(names);
		xc_search_lists_release(nullptr);
		xc_search_module_names_release(nullptr);
		check(!xc_request_take_search_lists(nullptr) && !xc_request_take_search_module_names(nullptr) && !xc_search_module_names_count(nullptr) && !xc_search_module_names_at(nullptr, 0), "Null result functions are safe");
		std::cout << "C copied named lists, effective labels, shared module splitting, strict getters and owner lifetime passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
