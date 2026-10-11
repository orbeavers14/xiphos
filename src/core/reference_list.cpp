// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2000-2026 Xiphos Developer Team
// Adapted from src/backend/sword_main.cc; existing fork behavior retained.
#include "xiphos/core/reference_list.hpp"
#include <swmodule.h>
#include <versekey.h>
#include <listkey.h>
#include <stdexcept>
namespace xiphos { namespace core {
ParsedReferenceList parse_reference_list(sword::SWModule &module, const std::string &list,
					const std::string &context)
{
	auto *key = dynamic_cast<sword::VerseKey *>(module.getKey());
	if (!key || context.empty())
		throw std::invalid_argument("Reference lists require a verse-key module and context");
	sword::VerseKey current(*key);
	current.setText(context.c_str());
	if (current.getError())
		throw std::invalid_argument("SWORD could not resolve reference-list context");
	auto parsed = current.parseVerseList(list.c_str(), current.getText(), true);
	ParsedReferenceList result;
	for (int i = 0; i < parsed.getCount(); ++i) {
		auto *element = parsed.getElement(i);
		if (!element || element->popError()) {
			++result.skipped;
			continue;
		}
		auto *verse = dynamic_cast<sword::VerseKey *>(element);
		if (verse) {
			auto &bound = verse->getUpperBound();
			if (bound.compare(*verse) != 0) {
				sword::VerseKey walker(*verse);
				int guard = 0;
				while ((walker.compare(bound) <= 0) && !walker.popError() && (guard++ < 2000)) {
					result.references.emplace_back(walker.getText());
					walker.increment(1);
				}
				if (guard > 2000 && walker.compare(bound) <= 0)
					result.limited = true;
				continue;
			}
		}
		result.references.emplace_back(element->getText());
	}
	return result;
}
}}
