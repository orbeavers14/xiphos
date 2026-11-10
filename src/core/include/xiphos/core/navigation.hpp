// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
namespace sword { class SWModule; class VerseKey; }
namespace xiphos::core {
struct ReferenceChoice { std::string label, reference; };
struct ReferenceChoices {
  std::vector<ReferenceChoice> books, chapters, verses;
  std::string book_label, chapter_label, verse_label;
};
// Rewrite a recognized localized leading book name to its canonical OSIS name.
std::string localized_reference_expression(sword::VerseKey &key, const std::string &reference,
                                           const std::string &locale_file);
// Caller supplies a verse-key module and serializes all accesses.
ReferenceChoices reference_choices(sword::SWModule &module, const std::string &reference);
ReferenceChoices reference_choices(sword::SWModule &module, const std::string &reference, const std::string &locale_file);
}
