// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "search_control.hpp"
#include "search_export_selection.hpp"
#include "xiphos/core/export_item.hpp"
#include <functional>
#include <stdexcept>
namespace xiphos::core::detail
{
inline SearchTextExport fail_export(SearchTextExport out, SearchExportStatus status, std::optional<std::size_t> row = std::nullopt)
{
	out.status = status;
	out.failed_row = row;
	out.row_count = 0;
	out.text.clear();
	return out;
}
inline bool stopped(const std::shared_ptr<SearchControl> &control) { return control && control->cancel_requested(); }
// One document policy for explicit visible selections and complete returned keys.
// The owning session supplies configured SDK reads, never a frontend renderer.
inline SearchTextExport render_search_export_text(const std::string &title, std::size_t cap, const SearchExportDocument &document,
						  SearchTextExport out, const std::shared_ptr<SearchControl> &control, const std::function<std::string(const std::string &, const std::string &, std::size_t)> &read, bool report_progress = true, bool include_context = false)
{
	const auto &rows = document.rows;
	if (title.size() + 3 > cap)
		return fail_export(std::move(out), SearchExportStatus::output_limit);
	std::string staged = title + "\n\n";
	if (include_context && (out.generation.cancelled || out.generation.failed_groups || out.generation.group_count != out.generation.requested_count)) {
		const std::string warning = "Source search stopped or had module failures; only returned matches are included.\n\n";
		if (warning.size() > cap - staged.size() - 1)
			return fail_export(std::move(out), SearchExportStatus::output_limit);
		staged += warning;
	}
	if (!document.source_summary.empty()) {
		if (document.source_summary.size() + 1 > cap - staged.size() - 1)
			return fail_export(std::move(out), SearchExportStatus::output_limit);
		staged += document.source_summary + "\n";
	}
	std::optional<std::size_t> previous_group;
	for (std::size_t i = 0; i < rows.size(); ++i) {
		if (stopped(control))
			return fail_export(std::move(out), SearchExportStatus::cancelled, i);
		const auto &row = rows[i];
		if (row.context && previous_group != row.group) {
			if (row.context->size() + 1 > cap - staged.size() - 1)
				return fail_export(std::move(out), SearchExportStatus::output_limit, i);
			staged += *row.context + "\n";
		}
		previous_group = row.group;
		const auto description = row.second + " " + row.first;
		const auto base = 10 + description.size() + row.first.size() + row.second.size();
		const auto remaining = cap - staged.size() - 1; // Reserve final document newline.
		if (base > remaining)
			return fail_export(std::move(out), SearchExportStatus::output_limit, i);
		std::string scripture;
		if (out.includes_scripture) {
			try {
				scripture = read(row.first, row.second, remaining - base);
			} catch (const std::length_error &) {
				return fail_export(std::move(out), SearchExportStatus::output_limit, i);
			} catch (const std::out_of_range &) {
				return fail_export(std::move(out), SearchExportStatus::module_not_found, i);
			} catch (const std::invalid_argument &) {
				return fail_export(std::move(out), SearchExportStatus::engine_error, i);
			} catch (const std::runtime_error &) {
				return fail_export(std::move(out), SearchExportStatus::engine_error, i);
			}
		}
		if (stopped(control))
			return fail_export(std::move(out), SearchExportStatus::cancelled, i);
		const auto item = format_export_item(description, row.first, row.second, scripture, false, out.includes_scripture);
		if (item.size() > remaining)
			return fail_export(std::move(out), SearchExportStatus::output_limit, i);
		staged += item;
		if (control && report_progress)
			detail::SearchControlAccess::progress(*control, static_cast<unsigned>((i + 1) * 100 / rows.size()));
	}
	staged += '\n';
	if (stopped(control))
		return fail_export(std::move(out), SearchExportStatus::cancelled);
	out.status = SearchExportStatus::ok;
	out.row_count = rows.size();
	out.text = std::move(staged);
	return out;
}
} // namespace xiphos::core::detail
