// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
namespace xiphos { namespace core {
struct WordBlockResult { std::string html; bool applied = false; };
// Owned, nonmutating interface around the characterized valid block grammar.
// Unsupported/malformed/oversized markup returns original HTML with applied=false.
// NUL is rejected. Uses local recursion/output state; caller initializes locale.
WordBlockResult render_word_blocks(std::string html);
struct LexicalLayout {
  std::string html, css;
  bool requested = false;
  bool applied = false;
};
// One owned decision for HTML and its matching legacy layout CSS. No annotation
// flags means unchanged HTML. Unsupported markup retains raw HTML and uses only
// optional double-spacing CSS, never positioning rules for missing word spans.
// Top-level SDK p/h1-h6 blocks retain their wrappers outside inline word spans.
// Nested supported blocks and crossed inline/word tag boundaries fall back.
// NUL is rejected even when layout is disabled. Does not apply module filters.
LexicalLayout layout_lexical_content(std::string html, bool strongs, bool lemmas,
                                     bool morphology, bool double_space);
}}
