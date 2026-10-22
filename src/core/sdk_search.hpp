// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <swmodule.h>
namespace xiphos { namespace core { namespace detail {
// Released SWORD 1.8/1.9 documents numeric search types and a macro flag.
// Newer 1.9 SVN SDKs expose class constants instead (same version string).
#ifdef SEARCHFLAG_MATCHWHOLEENTRY
constexpr int sdk_search_regex = 0;
constexpr int sdk_search_phrase = -1;
constexpr int sdk_search_multiword = -2;
constexpr int sdk_search_attribute = -3;
constexpr int sdk_search_external = -4;
constexpr int sdk_search_whole_entry = SEARCHFLAG_MATCHWHOLEENTRY;
#else
// These SDK members are defined in the library, not initialized in the header.
// Their values are runtime constants and cannot initialize constexpr variables.
const int sdk_search_regex = sword::SWModule::SEARCHTYPE_REGEX;
const int sdk_search_phrase = sword::SWModule::SEARCHTYPE_PHRASE;
const int sdk_search_multiword = sword::SWModule::SEARCHTYPE_MULTIWORD;
const int sdk_search_attribute = sword::SWModule::SEARCHTYPE_ENTRYATTR;
const int sdk_search_external = sword::SWModule::SEARCHTYPE_EXTERNAL;
const int sdk_search_whole_entry = sword::SWModule::SEARCHFLAG_MATCHWHOLEENTRY;
#endif
}}}
