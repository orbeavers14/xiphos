// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "lexical.hpp"
#include <vector>
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
// Raw named advanced-search lists from the first legacy section. Preserve
// ordering, duplicate labels, missing versus empty attributes and unknown modules.
// No tokenization, installed-module lookup, search execution or profile writes.
struct SavedSearchList {
  std::optional<std::string> label, value;
};
struct SavedSearchLists {
  std::vector<SavedSearchList> module_lists, ranges;
};
inline constexpr std::size_t maximum_saved_search_text_bytes = 16 * 1024 * 1024;
inline constexpr std::size_t maximum_saved_search_lists = 1024; // Per section.
SavedSearchLists parse_saved_search_lists(const std::string &xml);
SavedSearchLists read_saved_search_lists(const std::string &path);
// Exact first matching label, including a missing label. A missing value on that
// first match shadows later duplicates, as in xml_get_list_from_label.
std::optional<std::size_t> saved_search_list_index(const std::vector<SavedSearchList> &items,
                                                const std::optional<std::string> &label);
std::optional<std::string> saved_search_list_value(const std::vector<SavedSearchList> &items,
                                                 const std::optional<std::string> &label);
// Legacy comma tokenization: skip empty fields, preserve spaces/order/duplicates.
// Reject malformed names or more modules than one bounded shared search batch.
// This never resolves abbreviations, validates installation or starts a search.
std::vector<std::string> saved_search_module_names(const std::string &text);
enum class SavedSearchListKind { module_list = 0, range = 1 };
struct SavedSearchListEdit {
  SavedSearchListKind kind = SavedSearchListKind::module_list;
  std::string label, value; // Explicit literal attributes; empty is valid, NUL is not.
};
// Update/create only the first matching named item in the first section, retaining
// unrelated XML, attributes, comments and shadowed duplicates. Output is verified
// UTF-8 under the existing profile bounds; no automatic profile discovery.
std::string serialize_saved_search_list(const std::string &xml, const SavedSearchListEdit &edit);
// Same-directory staged verification/replacement, as for explicit lexicon saves.
// Return the copied staged snapshot prepared before the atomic replacement.
SavedSearchLists save_saved_search_list(const std::string &path, const SavedSearchListEdit &edit);
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
