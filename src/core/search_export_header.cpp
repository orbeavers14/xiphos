// SPDX-License-Identifier: GPL-2.0-or-later
#include "search_generation.hpp"
#include <stdexcept>
#include "sdk_search.hpp"
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
	case SearchMode::surface_literal:
		return "Literal surface";
	case SearchMode::surface_unmarked:
		return "Surface ignoring Greek, Hebrew and Syriac marks";
	case SearchMode::indexed:
		return "Raw indexed query";
	}
	throw std::logic_error("Invalid committed search mode");
}
const char *backend_name(int type)
{
	if (type == xiphos::core::detail::sdk_search_regex)
		return "regular expression";
	if (type == xiphos::core::detail::sdk_search_phrase)
		return "phrase";
	if (type == xiphos::core::detail::sdk_search_multiword)
		return "multiword";
	if (type == xiphos::core::detail::sdk_search_attribute)
		return "entry attribute";
	if (type == xiphos::core::detail::sdk_search_external)
		return "external index";
	throw std::logic_error("Unknown committed search backend");
}
std::string scope_name(const SearchQuery &query)
{
	if (query.keyed_interval)
		return "keyed interval: " + query.keyed_interval->first + " through " + query.keyed_interval->last;
	if (query.range)
		return "range (" + query.range->module + "): " + query.range->expression;
	if (query.within_last_results)
		return "previous results";
	return "entire module";
}
const char *flag(bool enabled) { return enabled ? "on" : "off"; }
} // namespace
std::string SearchGeneration::export_source_summary() const
{
	std::string text = "Source search groups\n";
	auto append_scope = [&](const SearchQuery &query) {
		text += std::string("Case sensitive: ") + flag(query.case_sensitive) +
			"\nScope: " + scope_name(query) + "\n";
	};
	for (std::size_t index = 0; index < requested_.size(); ++index) {
		const auto &query = requested_[index];
		text += "Group " + std::to_string(index + 1) + "\nModule: " + query.module +
			"\nQuery: " + query.expression + "\nStatus: ";
		if (index >= groups_.size()) {
			text += "not attempted\n";
			append_scope(query);
			continue;
		}
		const auto &group = groups_[index];
		if (!group.result) {
			switch (group.failure) {
			case SearchBatchFailure::module_not_found:
				text += "failed (module not found)\n";
				break;
			case SearchBatchFailure::invalid_request:
				text += "failed (invalid request)\n";
				break;
			case SearchBatchFailure::engine_error:
				text += "failed (engine error)\n";
				break;
			case SearchBatchFailure::none:
				throw std::logic_error("Missing successful search result");
			}
			append_scope(query);
			continue;
		}
		const auto &result = *group.result;
		if (result.cancelled)
			text += "stopped\n";
		else if (!result.executed)
			text += "not executed (empty scope)\n";
		else
			text += result.total_results ? "completed\n" : "completed (no matches)\n";
		if (!result.total_results) append_scope(query);
		text += "SDK result elements: " + std::to_string(result.total_results) + "\n";
		if (result.source_module() != query.module)
			text += "Resolved module: " + result.source_module() + "\n";
	}
	return text;
}
std::string SearchGeneration::export_header(std::size_t index) const
{
	if (index >= groups_.size() || !groups_[index].result)
		throw std::logic_error("No successful export group");
	const auto &query = requested_[index];
	const auto &result = *groups_[index].result;
	std::string text = "Search group " + std::to_string(index + 1) + "\nModule: " + query.module;
	if (result.source_module() != query.module) text += "\nResolved module: " + result.source_module();
	if(result.key_kind!=SearchKeyKind::verse)text+=result.key_kind==SearchKeyKind::dictionary?"\nResult keys: dictionary entries":"\nResult keys: general-book paths";
	text += "\nQuery: " + query.expression + "\nMode: " + mode_name(query.mode) +
			    "\nCase sensitive: " + flag(query.case_sensitive) + "\nScope: " + scope_name(query);
	text += "\nSource SDK result elements: " + std::to_string(result.total_results) +
		"\nSource visible references: " + std::to_string(result.references.size()) + "\n";
	if (result.limited)
		text += "Source visible references were limited; this is not the complete returned key list.\n";
	if (result.effective_expression != query.expression)
		text += "Effective query: " + result.effective_expression + "\n";
	if (result.effective_text_options) {
		const auto &options = *result.effective_text_options;
		text += std::string("Search annotation policy: Strong's ") + flag(options.strongs) + ", morphology " + flag(options.morphology) + ", footnotes " + flag(options.footnotes) + "\n";
	} else
		text += "Search annotation policy: unchanged SDK state\n";
	text += std::string("Backend: ") +
		(surface_search_mode(result.requested.mode) ? "Unicode verse scan" : backend_name(result.engine_type)) + "\n";
	if (result.index_fallback)
		text += "Indexed backend unavailable; used multiword fallback.\n";
	if (result.cancelled)
		text += "Source group stopped; only returned matches are included.\n";
	return text;
}
} // namespace xiphos::core::detail
