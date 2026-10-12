// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace xiphos { namespace core {
struct WordBlockResult { std::string html; bool applied = false; };
// Owned, nonmutating interface around the characterized valid block grammar.
// Unsupported/malformed/oversized markup returns original HTML with applied=false.
// NUL is rejected. Uses local recursion/output state; caller initializes locale.
WordBlockResult render_word_blocks(std::string html);
}}
