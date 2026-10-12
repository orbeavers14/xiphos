// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace xiphos { namespace core {
// Legacy annotation styles take precedence over double spacing; lemmas share
// the Strong's row. Returns owned CSS; theme/module/RTL/font acceptance separate.
std::string lexical_block_style(bool strongs, bool lemmas, bool morphology, bool double_space);
}}
