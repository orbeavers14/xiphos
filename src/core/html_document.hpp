// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/html_source.hpp"
#include "xiphos/core/search_export.hpp"
#include "export_png.hpp"
#include <functional>
namespace xiphos::core::detail {
struct PreparedExportHtml {
  std::string html,css;
  unsigned style=0;
  HtmlExportIssue issue=HtmlExportIssue::none;
  bool limited=false,cancelled=false;
};
// Only the core's bounded PNG packager supplies this resolver. All markup is
// checked before any resolution; original source URIs never become output URIs.
using ExportImageResolver=std::function<ExportPng(const std::string &)>;
// Empty resolver preserves the original resource-rejecting policy. Unsupported
// material rejects the whole fragment; there is no partially prepared HTML.
PreparedExportHtml prepare_export_html(const EntryHtmlSource &source,std::size_t max_bytes,
                                       const ExportImageResolver &images={});
}
