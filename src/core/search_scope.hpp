// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/search.hpp"
#include <listkey.h>
namespace xiphos::core::detail {
struct SearchScopeLimits {
  std::size_t max_elements=100000;
  std::size_t max_key_bytes=16*1024*1024;
};
// Private engine-owner snapshot. Never crosses the worker boundary. The limits
// bound logical key count/text, not total SDK heap or search execution time.
class SearchScope {
  sword::ListKey keys_;
  std::size_t key_bytes_=0;
public:
  explicit SearchScope(const sword::ListKey &keys,const SearchScopeLimits &limits = {});
  const sword::ListKey &keys() const noexcept{return keys_;}
  std::size_t accounted_key_bytes() const noexcept{return key_bytes_;}
  SearchScopeInfo info() const noexcept{return {true,static_cast<std::uint64_t>(keys_.getCount())};}
};
}
