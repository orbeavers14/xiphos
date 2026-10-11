// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
// Caller positions the module first. Direction -1/+1; owns SDK returned key.
std::string dictionary_step_key(sword::SWModule &module, int direction);
}}
