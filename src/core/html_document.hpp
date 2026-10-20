// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/html_source.hpp"
#include "xiphos/core/search_export.hpp"
namespace xiphos::core::detail {
struct PreparedExportHtml {
  std::string html,css;
  unsigned style=0;
  HtmlExportIssue issue=HtmlExportIssue::none;
  bool limited=false;
};
// Initial passive fragment policy. No resource loading, module CSS, scripts or
// inline styles. Unsupported material rejects the whole export, never disappears.
PreparedExportHtml prepare_export_html(const EntryHtmlSource &source,std::size_t max_bytes);
}
