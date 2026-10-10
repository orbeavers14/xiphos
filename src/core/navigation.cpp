// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/navigation.hpp"
#include <swmodule.h>
#include <versekey.h>
#include <stdexcept>
namespace xiphos::core {
ReferenceChoices reference_choices(sword::SWModule &module, const std::string &reference) {
  auto *key = dynamic_cast<sword::VerseKey *>(module.getKey());
  if (!key) throw std::invalid_argument("Selectors require a verse-key module");
  sword::VerseKey current(*key);
  current.setAutoNormalize(true);
  current.setText(reference.c_str());
  if (current.getError() || current.getChapter() < 1 || current.getVerse() < 1)
    throw std::invalid_argument("SWORD could not resolve selector context");
  ReferenceChoices result;
  result.book_label = current.getBookName();
  result.chapter_label = std::to_string(current.getChapter());
  result.verse_label = std::to_string(current.getVerse());
  // Match legacy module_has_testament's first/last-entry probe. Restore the
  // caller's module position and link-skipping flag even on failed allocation.
  struct Restore {
    sword::SWModule &module;
    std::string reference;
    bool skip;
    ~Restore() { module.setSkipConsecutiveLinks(skip); module.getKey()->setText(reference.c_str()); }
  } restore{module, module.getKeyText(), module.isSkipConsecutiveLinks()};
  module.setSkipConsecutiveLinks(true);
  module = sword::TOP;
  sword::VerseKey edge(current);
  edge.setText(module.getKeyText());
  const bool ot = edge.getTestament() == 1;
  module = sword::BOTTOM;
  edge.setText(module.getKeyText());
  const bool nt = edge.getTestament() == 2;
  for (int testament = 1; testament <= current.getTestamentMax(); ++testament) {
    if ((testament == 1 && !ot) || (testament == 2 && !nt)) continue;
    sword::VerseKey names(current);
    names.setChapter(1); names.setVerse(1); names.setTestament(testament); names.setBook(1);
    const auto count = names.getBookMax();
    for (int book = 1; book <= count; ++book) {
      names.setBook(book);
      sword::VerseKey target(current);
      // This order mirrors on_ot/nt_book_menu_select. Preserve SWORD's behavior
      // rather than inventing chapter/verse reset rules in the frontend.
      target.setTestament(testament); target.setBook(book);
      result.books.push_back({names.getBookName(), target.getText()});
    }
  }
  for (int chapter = 1; chapter <= current.getChapterMax(); ++chapter) {
    sword::VerseKey target(current); target.setChapter(chapter);
    result.chapters.push_back({std::to_string(chapter), target.getText()});
  }
  for (int verse = 1; verse <= current.getVerseMax(); ++verse) {
    sword::VerseKey target(current); target.setVerse(verse);
    result.verses.push_back({std::to_string(verse), target.getText()});
  }
  return result;
}
}
