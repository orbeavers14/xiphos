// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
// Preserves legacy setKey/renderText semantics, including key mutation.
// Caller owns the module and must serialize access to it.
std::string render_entry(sword::SWModule &module, const char *key);
}}
