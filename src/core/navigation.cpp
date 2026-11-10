// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/navigation.hpp"
#include <swmodule.h>
#include <versekey.h>
#include <stdexcept>
#include <swlocale.h>
#include <versificationmgr.h>
#include <glib.h>
#include <filesystem>
#include <memory>
#include <cstring>
namespace xiphos::core {
namespace {
std::unique_ptr<sword::SWLocale> reference_locale(const std::string &locale_file) {
  std::unique_ptr<sword::SWLocale> locale;
  if (!locale_file.empty()) {
    const auto path = std::filesystem::u8path(locale_file);
    if (locale_file.size() > 4096 || locale_file.find('\0') != std::string::npos ||
        std::filesystem::is_symlink(path) || !std::filesystem::is_regular_file(path) ||
        std::filesystem::file_size(path) > 512 * 1024)
      throw std::invalid_argument("Reference locale must be a bounded regular file");
    locale = std::make_unique<sword::SWLocale>(locale_file.c_str());
    if (!locale->getEncoding() || std::strcmp(locale->getEncoding(), "UTF-8"))
      throw std::invalid_argument("Reference locale must use UTF-8");
  }
  return locale;
}
std::string folded_reference(const char *text) {
  if (!text || !g_utf8_validate(text,-1,nullptr)) return {};
  using Buffer = std::unique_ptr<char,decltype(&g_free)>;
  Buffer folded(g_utf8_casefold(text,-1), &g_free);
  if (!folded) throw std::bad_alloc();
  Buffer normalized(g_utf8_normalize(folded.get(),-1,G_NORMALIZE_NFC), &g_free);
  if (!normalized) throw std::bad_alloc();
  std::string result;
  bool space = false;
  for (const char *at = normalized.get(); *at; at = g_utf8_next_char(at)) {
    const auto scalar = g_utf8_get_char(at);
    if (g_unichar_isspace(scalar)) {
      space = !result.empty();
      continue;
    }
    if (space) result += ' ';
    space = false;
    const auto digit = g_unichar_digit_value(scalar);
    if (digit >= 0) result += static_cast<char>('0' + digit);
    else result.append(at,g_utf8_next_char(at));
  }
  return result;
}
}

std::string localized_reference_expression(sword::VerseKey &key, const std::string &reference,
                                           const std::string &locale_file) {
  const auto locale = reference_locale(locale_file);
  if (!locale) return reference;
  const auto input = folded_reference(reference.c_str());
  if (input.empty()) return reference;
  const auto *canon = sword::VersificationMgr::getSystemVersificationMgr()
      ->getVersificationSystem(key.getVersificationSystem());
  if (!canon) throw std::invalid_argument("Unknown reference versification");
  std::size_t longest = 0;
  std::string book;
  bool ambiguous = false;
  auto consider = [&](const char *label, const char *osis) {
    const auto candidate = folded_reference(label);
    if (candidate.empty() || candidate.size() < longest ||
        input.compare(0,candidate.size(),candidate) != 0) return;
    if (input.size() > candidate.size()) {
      const auto next = g_utf8_get_char(input.data() + candidate.size());
      if (!g_unichar_isspace(next) && (next < '0' || next > '9')) return;
    }
    if (candidate.size() > longest) {
      longest = candidate.size();
      book = osis;
      ambiguous = false;
    } else if (book != osis) ambiguous = true;
  };
  int count = 0;
  const auto *aliases = locale->getBookAbbrevs(&count);
  if (count < 0 || count > 16384) throw std::invalid_argument("Reference alias limit exceeded");
  for (int i = 0; i < count; ++i) consider(aliases[i].ab, aliases[i].osis);
  for (int i = 0; i < canon->getBookCount(); ++i) {
    const auto *entry = canon->getBook(i);
    consider(locale->translate(entry->getLongName()), entry->getOSISName());
  }
  if (!longest) {
    // A locale can normalize a numeric shorthand without introducing a book.
    // Unknown book text still goes through the existing SDK parser unchanged.
    return input.find_first_not_of("0123456789:.,;- ") == std::string::npos ? input : reference;
  }
  if (ambiguous) throw std::invalid_argument("Localized book name is ambiguous");
  if (canon->getBookNumberByOSISName(book.c_str()) < 1)
    throw std::invalid_argument("Localized book is absent from this module's canon");
  const char *suffix = input.c_str() + longest;
  while (*suffix && g_unichar_isspace(g_utf8_get_char(suffix))) suffix = g_utf8_next_char(suffix);
  return *suffix ? book + " " + suffix : book;
}
ReferenceChoices reference_choices(sword::SWModule &module, const std::string &reference) {
  return reference_choices(module, reference, {});
}
ReferenceChoices reference_choices(sword::SWModule &module, const std::string &reference, const std::string &locale_file) {
  const auto locale = reference_locale(locale_file);
  auto label = [&](sword::VerseKey key) {
    key.setLocale("en");
    const std::string english = key.getBookName();
    return locale ? std::string(locale->translate(english.c_str())) : std::string(key.getBookName());
  };
  auto *key = dynamic_cast<sword::VerseKey *>(module.getKey());
  if (!key) throw std::invalid_argument("Selectors require a verse-key module");
  sword::VerseKey current(*key);
  current.setAutoNormalize(true);
  current.setText(reference.c_str());
  if (current.getError() || current.getChapter() < 1 || current.getVerse() < 1)
    throw std::invalid_argument("SWORD could not resolve selector context");
  ReferenceChoices result;
  result.book_label = locale ? label(current) : current.getBookName();
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
      result.books.push_back({locale ? label(names) : names.getBookName(), target.getText()});
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
