// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <array>
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
struct SearchTextOptions {
  bool strongs, morphology, footnotes;
  SearchTextOptions(bool words=false,bool morphs=false,bool notes=false)
    : strongs(words),morphology(morphs),footnotes(notes) {}
};
struct SearchTextSetting { const char *name;bool enabled; };
// Legacy advanced-dialog annotation names/order. Exact phrase leaves the prior
// search-manager state untouched; that state transition is handled by the caller.
std::array<SearchTextSetting,3> search_text_settings(const SearchTextOptions &options);
}}
