// SPDX-License-Identifier: GPL-2.0-or-later
#include "search_generation.hpp"
#include <stdexcept>
#include <swmodule.h>
namespace xiphos::core::detail
{
namespace
{
const char *mode_name(SearchMode mode)
{
	switch (mode) {
	case SearchMode::regular_expression:
		return "Regular expression";
	case SearchMode::phrase:
		return "Exact phrase";
	case SearchMode::words:
		return "Sidebar words (automatic index selection)";
	case SearchMode::lemma:
		return "Lemma";
	case SearchMode::morphology:
		return "Morphology";
	case SearchMode::footnote:
		return "Footnote";
	case SearchMode::multiword:
		return "Multiword";
	case SearchMode::indexed:
		return "Raw indexed query";
	}
	throw std::logic_error("Invalid committed search mode");
}
const char *backend_name(int type)
{
	if (type == sword::SWModule::SEARCHTYPE_REGEX)
		return "regular expression";
	if (type == sword::SWModule::SEARCHTYPE_PHRASE)
		return "phrase";
	if (type == sword::SWModule::SEARCHTYPE_MULTIWORD)
		return "multiword";
	if (type == sword::SWModule::SEARCHTYPE_ENTRYATTR)
		return "entry attribute";
	if (type == sword::SWModule::SEARCHTYPE_EXTERNAL)
		return "external index";
	throw std::logic_error("Unknown committed search backend");
}
const char *flag(bool enabled) { return enabled ? "on" : "off"; }
} // namespace
std::string SearchGeneration::export_header(std::size_t index) const
{
	if (index >= groups_.size() || !groups_[index].result)
		throw std::logic_error("No successful export group");
	const auto &query = requested_[index];
	const auto &result = *groups_[index].result;
	std::string text = "Search group " + std::to_string(index + 1) + "\nModule: " + query.module +
			   "\nQuery: " + query.expression + "\nMode: " + mode_name(query.mode) +
			   "\nCase sensitive: " + flag(query.case_sensitive) + "\nScope: ";
	if (query.range)
		text += "range (" + query.range->module + "): " + query.range->expression;
	else if (query.within_last_results)
		text += "previous results";
	else
		text += "entire module";
	text += "\n";
	if (result.effective_expression != query.expression)
		text += "Effective query: " + result.effective_expression + "\n";
	if (result.effective_text_options) {
		const auto &options = *result.effective_text_options;
		text += std::string("Search annotation policy: Strong's ") + flag(options.strongs) + ", morphology " + flag(options.morphology) + ", footnotes " + flag(options.footnotes) + "\n";
	} else
		text += "Search annotation policy: unchanged SDK state\n";
	text += std::string("Backend: ") + backend_name(result.engine_type) + "\n";
	if (result.index_fallback)
		text += "Indexed backend unavailable; used multiword fallback.\n";
	if (result.cancelled)
		text += "Source group stopped; only returned matches are included.\n";
	return text;
}
} // namespace xiphos::core::detail
