// SPDX-License-Identifier: GPL-2.0-or-later
#include "search_scope.hpp"
#include <versekey.h>
#include <cstring>
#include <stdexcept>
namespace xiphos::core::detail {
SearchScope::SearchScope(const sword::ListKey &keys,const SearchScopeLimits &limits) {
  if(!limits.max_elements||!limits.max_key_bytes)throw std::invalid_argument("Invalid retained search scope limits");
  if(keys.getCount()<0||static_cast<std::size_t>(keys.getCount())>limits.max_elements)
    throw std::length_error("Retained search scope exceeds element capacity");
  std::size_t bytes=0;
  auto account=[&](const char *text){
    if(!text)throw std::runtime_error("SDK scope key has missing text");
    const auto length=std::strlen(text);
    if(length>limits.max_key_bytes-bytes)throw std::length_error("Retained search scope exceeds text capacity");
    bytes+=length;
  };
  for(int i=0;i<keys.getCount();++i){
    const auto *key=keys.getElement(i);
    if(!key)throw std::runtime_error("SDK scope has a missing element");
    account(key->getText());account(key->getRangeText());
    if(const auto *verse=dynamic_cast<const sword::VerseKey *>(key))account(verse->getVersificationSystem());
    keys_.add(*key); // SDK clone retains key type, bounds, flags and versification.
  }
  keys_.setPosition(sword::TOP);
}
}
