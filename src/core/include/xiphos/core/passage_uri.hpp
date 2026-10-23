// SPDX-License-Identifier: GPL-2.0-or-later
// Extracted from main/url.cc, Copyright (C) 2004-2026 Xiphos Developer Team.
#ifndef XIPHOS_CORE_PASSAGE_URI_HPP
#define XIPHOS_CORE_PASSAGE_URI_HPP
#include <cstring>
namespace xiphos { namespace core {
// Split the retained click anchor in a caller-owned mutable reference. Hash
// takes precedence over bang regardless of order. Return borrowed text after
// the separator (including an empty anchor), or null without mutating the key.
// Input is non-null; this does not decode URI escapes or validate a module.
inline char *passage_uri_anchor(char *reference) {
  char *anchor = std::strchr(reference, '#');
  if (!anchor) anchor = std::strchr(reference, '!');
  if (anchor) *anchor++ = '\0';
  return anchor;
}
// The retained URI sidebar branch is only for Bible modules with list/range
// punctuation. Inputs are non-null borrowed strings; type lookup is deferred
// until punctuation is found, preserving retained callback order.
template<class IsBible>
bool passage_uri_reference_list(const char *module, const char *reference, IsBible is_bible) {
  return std::strpbrk(reference, "-;,") && is_bible(module);
}
}}
#endif
