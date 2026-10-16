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
// Read-only saved general-book fields, with the legacy XML getter's null/empty
// and first-matching-key semantics. Raw key/offset text stays unnormalized:
// startup resolution, module validation, tab state and writes are separate.
struct SavedBookSelection {
  std::optional<std::string> module, key, offset_text;
};
SavedBookSelection parse_saved_book_selection(const std::string &xml);
SavedBookSelection read_saved_book_selection(const std::string &path);
// Separately selected preview settings: lexicon import does not apply these.
struct StudyPreviewPreferences {
  bool cross_references_in_verse_list = true;
};
// Legacy misc/xrefsinverselist: missing/null defaults true; a bounded atoi
// prefix is false only when zero. Out-of-int-range prefixes reject safely.
// No file writes, implicit startup application or mutation of active tabs.
StudyPreviewPreferences parse_study_preview_preferences(const std::string &xml);
StudyPreviewPreferences read_study_preview_preferences(const std::string &path);
// Explicitly update the five effective lexicon values; retain unrelated XML
// nodes/attributes/comments outside updated values and shadowed duplicate keys.
// XML formatting/encoding normalizes to UTF-8. Null/empty choices cannot round
// trip through legacy defaults and reject. Saved locale is never modified.
std::string serialize_lexicon_profile(const std::string &xml, const LexiconPreferences &preferences);
// Verify a same-directory staged file before replacing the selected file.
// Missing files start with an Xiphos root. No implicit profile/home writes.
void save_lexicon_profile(const std::string &path, const LexiconPreferences &preferences);
}
