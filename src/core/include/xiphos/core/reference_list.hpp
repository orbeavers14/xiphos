// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
struct ParsedReferenceList {
	std::vector<std::string> references;
	bool limited = false;
	unsigned skipped = 0;
};
// Uses the module's own locale/versification and preserves ordering/duplicates.
// Keeps the existing fork's 2000-verse guard per range, reporting it explicitly.
// Works on a cloned key; caller serializes access to the module.
ParsedReferenceList parse_reference_list(sword::SWModule &module, const std::string &list,
					const std::string &context);
}}
