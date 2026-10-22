// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <cstddef>
#include <vector>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
// Native previews bound the entire list, including repeated ranges/singles.
constexpr std::size_t maximum_native_reference_list_entries = 2000;
struct ParsedReferenceList {
	std::string context;
	std::vector<std::string> references;
	bool limited = false;
	unsigned skipped = 0;
};
// Uses the module's own locale/versification and preserves ordering/duplicates.
// Keeps the existing fork's 2000-verse guard per range, reporting it explicitly.
// Works on a cloned key; caller serializes access to the module.
// Optional aggregate limit preserves the original ordered prefix and reports
// limited only when another reference would be appended. Zero preserves the
// retained per-range-only policy. skipped counts cover the inspected prefix.
ParsedReferenceList parse_reference_list(sword::SWModule &module, const std::string &list,
					const std::string &context);
ParsedReferenceList parse_reference_list(sword::SWModule &module, const std::string &list,
                                        const std::string &context, std::size_t maximum_total);
}}
