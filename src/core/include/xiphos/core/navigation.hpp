// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>
#include <vector>
namespace sword { class SWModule; }
namespace xiphos::core {
struct ReferenceChoice { std::string label, reference; };
struct ReferenceChoices {
  std::vector<ReferenceChoice> books, chapters, verses;
  std::string book_label, chapter_label, verse_label;
};
// Caller supplies a verse-key module and serializes all accesses.
ReferenceChoices reference_choices(sword::SWModule &module, const std::string &reference);
}
