// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
// Sequential Heading/Preverse attributes at the current entry, with the legacy
// SDK UTF8HTML conversion and renderText dataflow. Stops at the first empty
// index. Caller serializes the module and establishes engine options first.
// Xiphos block/cleanup transforms and GTK header caching remain separate.
struct HeadingFragment {
  std::string html;
  std::string reported_note_label; // UTF8HTML Footnote/1/n at the legacy cleanup point.
};
// Same legacy SDK Preverse indices and HTML; additionally copies each fragment's
// reported first-note label before later renders replace entry attributes. This
// can remain the source-entry label, rather than visible heading marker text.
// A fragment may contain multiple HTML headings. Empty follows the legacy probe.
// On normal completion, the final source-entry refresh matches the string API.
std::vector<HeadingFragment> render_preverse_heading_fragments(sword::SWModule &module);
std::vector<std::string> render_preverse_headings(sword::SWModule &module);
}}
