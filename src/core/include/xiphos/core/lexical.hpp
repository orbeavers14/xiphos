// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "lexicon.h"
#include "link.hpp"
#include "content.hpp"
#include <optional>
namespace xiphos::core {
// Legacy settings defaults; opt-in per session, no user profile access.
struct LexiconPreferences {
  std::optional<std::string> greek = "StrongsGreek", hebrew = "StrongsHebrew";
  std::optional<std::string> greek_morph_nt = "Robinson", greek_morph_ot = "Packard", hebrew_morph = "OSHM";
};
// Reject embedded NULs before borrowing strings through the C11 policy helper.
XiphosLexiconPreferences borrow_lexicon_preferences(const LexiconPreferences &preferences);
struct LexicalEntry {
  LexicalLink command;
  std::string source_reference;
  DictionaryEntry entry;
};
}
