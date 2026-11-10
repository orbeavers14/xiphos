// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/parallel.hpp"
#include "xiphos/core/navigation_key.hpp"
#include <swmodule.h>
#include <versekey.h>
#include <algorithm>
#include <memory>
#include <stdexcept>
namespace xiphos { namespace core {
bool parallel_read_arguments(const std::vector<std::string> &modules,
    const std::string &reference, std::size_t max_bytes) noexcept {
  if (modules.empty() || modules.size() > maximum_parallel_columns || reference.empty() ||
      reference.size() > maximum_parallel_reference_bytes || reference.find('\0') != std::string::npos ||
      !max_bytes || max_bytes > maximum_parallel_snapshot_bytes) return false;
  for (const auto &module : modules)
    if (module.size() > maximum_parallel_module_bytes || module.find('\0') != std::string::npos) return false;
  return true;
}
ParallelReferences parallel_references(sword::SWModule &control,
    const std::string &reference, const char *locale) {
  if (reference.empty() || reference.size() > 32768 || reference.find('\0') != std::string::npos)
    throw std::invalid_argument("Invalid parallel reference");
  auto *live = dynamic_cast<sword::VerseKey *>(control.getKey());
  std::unique_ptr<sword::SWKey> owned_limit(control.createKey());
  auto *limit = dynamic_cast<sword::VerseKey *>(owned_limit.get());
  if (!live || !limit) throw std::invalid_argument("Parallel control requires a verse key");
  sword::VerseKey navigation(*live);
  auto normalized = navigation_key_text(navigation, reference.c_str(), true, locale);
  if (navigation.getError() || navigation.getChapter() < 1 || navigation.getVerse() < 1)
    throw std::invalid_argument("SWORD could not resolve a parallel control reference");
  // Count the resolved control chapter; do not ask a fresh key to reinterpret
  // the original expression. Bare numbers retain SDK shorthand semantics.
  limit->setAutoNormalize(true);limit->setText(normalized.c_str());
  const int count = limit->getVerseMax();
  if (limit->getError() || count < 1 || count > 1024)
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
std::vector<std::string> parallel_mapped_references(sword::SWModule &from,
    sword::SWModule &to, const std::string &reference) {
  auto *from_key = dynamic_cast<sword::VerseKey *>(from.getKey());
  auto *to_key = dynamic_cast<sword::VerseKey *>(to.getKey());
  if (!from_key || !to_key) return {};
  const std::string from_canon = from_key->getVersificationSystem();
  const std::string to_canon = to_key->getVersificationSystem();
  // The public SDK returns an unchanged reference even for systems with no
  // mapping table. Only admit known table-backed systems and the KJV anchors.
  const auto verified = [](const std::string &canon) {
    const char *const systems[] = {"KJV","KJVA","NRSV","Synodal","Vulg","Calvin","DarbyFr","Segond"};
    return std::find(std::begin(systems),std::end(systems),canon) != std::end(systems);
  };
  sword::VerseKey source(*from_key), target(*to_key);
  source.clearBounds();
  source.setAutoNormalize(false);
  source.setText(reference.c_str());
  if (source.getError() || source.getChapter() < 1 || source.getVerse() < 1 ||
      source.getVerse() > source.getVerseMax()) return {};
  target.clearBounds();
  const bool from_mt = from_canon == "MT";
  const bool to_mt = to_canon == "MT";
  const auto kjv_anchor = [](const std::string &canon) { return canon == "KJV" || canon == "KJVA"; };
  if ((from_mt && kjv_anchor(to_canon)) || (to_mt && kjv_anchor(from_canon))) {
    struct VerseRun {
      const char *book;
      int chapter, mt_first, kjv_first, count;
    };
    // Sources and verification scope: doc/modernization/real-corpus-validation.md.
    // The numbered Psalm body starts after the MT title.
    static constexpr VerseRun runs[] = {{"Gen", 1, 1, 1, 31}, {"Gen", 2, 1, 1, 25},
                                        {"Ps", 3, 2, 1, 8}, {"Ps", 4, 2, 1, 8}};
    const std::string book = source.getOSISBookName();
    int mapped = 0;
    for (const auto &run : runs) {
      const int offset = source.getVerse() - (from_mt ? run.mt_first : run.kjv_first);
      if (book == run.book && source.getChapter() == run.chapter &&
          offset >= 0 && offset < run.count) {
        mapped = (from_mt ? run.kjv_first : run.mt_first) + offset;
        break;
      }
    }
    if (!mapped) return {};
    target.setAutoNormalize(false);
    target.setText((book + " " + std::to_string(source.getChapter()) + ":" + std::to_string(mapped)).c_str());
    if (target.getError() || target.getVerse() != mapped || mapped > target.getVerseMax()) return {};
    return {target.getText()};
  }
  if (from_canon != to_canon && (!verified(from_canon) || !verified(to_canon))) return {};
  target.positionFrom(source);
  if (target.getError() || target.getChapter() < 1) return {};
  const int first = target.getVerse();
  const auto index = target.getIndex();
  const sword::VerseKey upper(target.getUpperBound());
  const bool bounded = target.getLowerBound().getIndex() == index;
  const bool range = bounded && upper.getIndex() > index;
  constexpr long maximum_split_verses = 16;
  if (range && (upper.getIndex() - index >= maximum_split_verses ||
      upper.getChapter() != target.getChapter() || upper.getBook() != target.getBook() ||
      upper.getTestament() != target.getTestament())) return {};
  const int last = range ? upper.getVerse() : first;
  if (last < 1 || last > target.getVerseMax()) return {};
  target.clearBounds();
  std::vector<std::string> result;
  for (int verse = std::max(1,first); verse <= last; ++verse) {
    target.setVerse(verse);
    result.emplace_back(target.getText());
  }
  return result;
}
}}
