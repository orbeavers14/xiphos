// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/lexical.hpp"
#include <stdexcept>
namespace xiphos::core {
namespace {
const char *borrow(const std::optional<std::string> &value) {
  if (!value) return nullptr;
  if (value->find('\0') != std::string::npos) throw std::invalid_argument("Lexicon preference contains NUL");
  return value->c_str();
}
}
XiphosLexiconPreferences borrow_lexicon_preferences(const LexiconPreferences &p) {
  return {borrow(p.greek), borrow(p.hebrew), borrow(p.greek_morph_nt), borrow(p.greek_morph_ot), borrow(p.hebrew_morph)};
}
}
