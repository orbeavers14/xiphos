// SPDX-License-Identifier: GPL-2.0-or-later
#include "search_validation.hpp"
#include <glib.h>
#include <stdexcept>
namespace xiphos::core::detail {
namespace {
void text(const std::string &value,std::size_t maximum){
  if(value.empty()||value.size()>maximum||value.find('\0')!=std::string::npos||!g_utf8_validate(value.data(),value.size(),nullptr))
    throw std::invalid_argument("Invalid bounded UTF-8 search input");
}
}
void validate_search_module_name(const std::string &module){text(module,4096);}
void validate_search_query(const SearchQuery &query){
  validate_search_module_name(query.module);text(query.expression,32768);
  if(static_cast<int>(query.mode)<0||static_cast<int>(query.mode)>static_cast<int>(SearchMode::surface_unmarked))throw std::invalid_argument("Unknown search mode");
  if(query.keyed_interval){
    if(query.range||query.within_last_results)throw std::invalid_argument("Keyed interval excludes verse range and last scope");
    text(query.keyed_interval->first,4096);text(query.keyed_interval->last,4096);
  }
  if(query.range){
    if(query.within_last_results)throw std::invalid_argument("Explicit range excludes last scope");
    validate_search_module_name(query.range->module);text(query.range->expression,4096);
  }
}
}
