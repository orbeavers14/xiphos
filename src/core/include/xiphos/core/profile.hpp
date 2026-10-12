// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "lexical.hpp"
namespace xiphos::core {
// Owned, read-only subset of the legacy Xiphos XML profile. Unknown settings
// are not exposed or changed. Locale is data, never applied by the reader.
struct StudyProfile {
  LexiconPreferences lexicons;
  std::optional<std::string> saved_locale;
};
// Input bytes may use an XML-declared encoding. Values are copied UTF-8.
// Require an Xiphos root; reject malformed, DTD and over-16-MiB documents.
// No external resources, home/profile discovery, module validation or writes.
StudyProfile parse_study_profile(const std::string &xml);
StudyProfile read_study_profile(const std::string &path);
}
