// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace xiphos { namespace core {
// Legacy sidebar external-index AND preparation: prefix + per ASCII-space word,
// preserve literal spacing/tabs/punctuation, append workaround trailing space.
// Non-indexed text is unchanged. No trimming, tokenization or SDK/file access.
std::string sidebar_search_text(std::string text, bool indexed);
enum class AttributeSearch { lemma, morphology, footnote };
struct AttributeSearchText { std::string expression; bool whole_entry; };
// Exact legacy SDK attribute path, preserving query bytes. Whole-entry matching
// prevents lemma/morph prefixes from matching longer identifiers. No SDK/UI work.
AttributeSearchText attribute_search_text(const std::string &text,AttributeSearch kind);
}}
