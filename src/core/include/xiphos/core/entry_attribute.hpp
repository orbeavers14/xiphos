// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace sword { class SWModule; }
namespace xiphos { namespace core {
// Owned legacy BackEnd::get_entry_attribute value, including UTF8HTML conversion.
// Empty represents the legacy null result. refresh=true renders the current key;
// false reads the last rendered entry/buffer's attributes without another render.
// Caller owns/serializes module access. Preserves legacy map lookup semantics.
// NUL in a field path rejects before rendering or changing attributes.
std::string entry_attribute(sword::SWModule &module, const std::string &group,
                             const std::string &item, const std::string &field,
                             bool refresh);
}}
