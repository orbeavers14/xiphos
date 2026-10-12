// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/commentary.hpp"
#include <swmodule.h>
#include <versekey.h>
#include <stdexcept>
namespace xiphos { namespace core {
std::vector<std::string> commentary_positions(sword::SWModule &module, const std::string &reference) {
  auto *key = dynamic_cast<sword::VerseKey *>(module.getKey());
  if (!key || reference.empty() || reference.find('\0') != std::string::npos)
    throw std::invalid_argument("Commentary chapter requires a verse reference");
  sword::VerseKey requested(*key);
  requested.setAutoNormalize(true);
  requested.setText(reference.c_str());
  if (requested.getError() || requested.getChapter() < 1 || requested.getVerse() < 1)
    throw std::invalid_argument("SWORD could not resolve a commentary chapter");
  struct Restore {
    sword::SWModule &module;
    sword::VerseKey &key;
    sword::VerseKey previous;
    bool skip;
    ~Restore() { key = previous; module.setSkipConsecutiveLinks(skip); }
  } restore{module, *key, *key, module.isSkipConsecutiveLinks()};
  *key = requested;
  module.setSkipConsecutiveLinks(true);
  const int book = key->getBook(), chapter = key->getChapter();
  std::vector<std::string> result;
  // Keep the exact legacy frontier: start at verse 1, advance the MODULE
  // (not each canon verse), and stop on its book/chapter/error boundary.
  for (key->setVerse(1);
       key->getBook() == book && key->getChapter() == chapter && !module.popError();
       module++) {
    result.push_back(key->getText());
  }
  return result;
}
}}
