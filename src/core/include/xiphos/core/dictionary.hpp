// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
// Caller positions the module first. Direction -1/+1; owns SDK returned key.
// Legacy nine-key chooser: five steps back, then nine forward snapshots.
std::vector<std::string> dictionary_nearby_keys(sword::SWModule &module);
std::string dictionary_step_key(sword::SWModule &module, int direction);
}}
