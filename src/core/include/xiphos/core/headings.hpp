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
std::vector<std::string> render_preverse_headings(sword::SWModule &module);
}}
