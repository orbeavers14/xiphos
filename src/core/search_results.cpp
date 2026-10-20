// SPDX-License-Identifier: GPL-2.0-or-later
#include "search_results.hpp"
#include <listkey.h>
#include <glib.h>
#include <stdexcept>
namespace xiphos::core::detail {
SearchReferences copy_search_references(sword::ListKey &list,const SearchLimits &limits,bool allow_empty_output) {
  if((!allow_empty_output&&(!limits.max_results||!limits.max_reference_bytes))||limits.max_results>maximum_search_results||
     limits.max_reference_bytes>maximum_search_reference_bytes)
    throw std::invalid_argument("Unsupported search output limits");
  if(list.getCount()<0)throw std::runtime_error("SDK search returned an invalid count");
  SearchReferences result{static_cast<std::uint64_t>(list.getCount()),{},false};
  if(!limits.max_results||!limits.max_reference_bytes){result.limited=list.getCount()>0;return result;}
  std::size_t bytes=0;list.setPosition(sword::TOP);
  while(list.getCount()&&!list.popError()) {
    const std::string reference=list.getText();
    if(reference.empty())throw std::runtime_error("SDK search returned an empty reference");
    if(result.references.size()==limits.max_results||reference.size()>limits.max_reference_bytes-bytes){result.limited=true;break;}
    if(!g_utf8_validate(reference.data(),static_cast<gssize>(reference.size()),nullptr))
      throw std::runtime_error("SDK search reference is not valid UTF-8");
    bytes+=reference.size();result.references.push_back(reference);list++;
  }
  return result;
}
}
