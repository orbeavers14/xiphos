// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
// Main-pane clicks use GLib's locale-sensitive Unicode uppercase. Hover/raw
// reads do not. Owns the result; rejects malformed UTF-8 and embedded NUL.
std::string dictionary_click_key(const std::string &key);
// Caller positions the module first. Direction -1/+1; owns SDK returned key.
// Legacy nine-key chooser: five steps back, then nine forward snapshots.
std::vector<std::string> dictionary_nearby_keys(sword::SWModule &module);
std::string dictionary_step_key(sword::SWModule &module, int direction);
}}
