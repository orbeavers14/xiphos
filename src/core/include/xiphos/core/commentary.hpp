// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
// Owned positions from the legacy main commentary chapter iterator. The module
// key and skip-links flag are restored; caller must serialize engine access.
// This is traversal only, not headings, presentation or editable commentary.
std::vector<std::string> commentary_positions(sword::SWModule &module, const std::string &reference);
}}
