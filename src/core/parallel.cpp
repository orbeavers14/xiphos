// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/parallel.hpp"
#include "xiphos/core/navigation_key.hpp"
#include <swmodule.h>
#include <versekey.h>
#include <memory>
#include <stdexcept>
namespace xiphos { namespace core {
ParallelReferences parallel_references(sword::SWModule &control,
    const std::string &reference, const char *locale) {
  if (reference.empty() || reference.size() > 32768 || reference.find('\0') != std::string::npos)
    throw std::invalid_argument("Invalid parallel reference");
  auto *live = dynamic_cast<sword::VerseKey *>(control.getKey());
  std::unique_ptr<sword::SWKey> owned_limit(control.createKey());
  auto *limit = dynamic_cast<sword::VerseKey *>(owned_limit.get());
  if (!live || !limit) throw std::invalid_argument("Parallel control requires a verse key");
  limit->setAutoNormalize(true);limit->setText(reference.c_str());
  const int count = limit->getVerseMax();
  sword::VerseKey navigation(*live);
  auto normalized = navigation_key_text(navigation, reference.c_str(), true, locale);
  if (navigation.getError() || limit->getError() || navigation.getChapter() < 1 ||
      navigation.getVerse() < 1 || count < 1 || count > 1024)
    throw std::invalid_argument("SWORD could not resolve a bounded parallel chapter");
  const std::string book = navigation.getBookName();
  const int chapter = navigation.getChapter();
  ParallelReferences result{std::move(normalized), book + " " + std::to_string(chapter),
    static_cast<unsigned>(navigation.getVerse()), {}};
  result.rows.reserve(static_cast<std::size_t>(count));
  for (int verse = 1; verse <= count; ++verse) {
    const auto row = book + " " + std::to_string(chapter) + ":" + std::to_string(verse);
    result.rows.push_back(navigation_key_text(navigation, row.c_str(), true, locale));
    if (navigation.getError()) throw std::invalid_argument("SWORD could not resolve a parallel row");
  }
  return result;
}
}}
