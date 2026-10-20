// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "search_generation.hpp"
#include "search_validation.hpp"
#include "search_control.hpp"
namespace xiphos::core {
SearchResult StudySession::search_module(SearchQuery query,const SearchLimits &limits,std::shared_ptr<SearchControl> control) {
  detail::SearchControlScope progress(control);
  detail::validate_search_query(query);
  detail::SearchGenerationBuilder generation(detail::next_search_generation_id(search_generation_serial_),{query},search_retention_limits_);
  auto scope=search_scope_;auto text=search_text_options_;detail::SearchRetainedKeys retained;
  auto result=reader_.search_impl(std::move(query),limits,control,&scope,text,true,0,100,false,&retained,generation.remaining());
  if(result.effective_text_options&&result.requested.text_options&&result.requested.mode!=SearchMode::phrase){
    text=result.requested.text_options;result.text_options_updated=true;
  }
  std::shared_ptr<const detail::SearchGeneration> staged;
  if(!result.cancelled||result.executed){
    generation.append(0,result,SearchBatchFailure::none,std::move(retained));staged=generation.finish(result.cancelled);
    result.generation=staged->info();result.generation_updated=true;
  }else result.generation=search_generation();
  // No allocation after publication. Reading/history and other tabs are untouched.
  search_scope_=std::move(scope);search_text_options_=text;
  if(staged){search_generation_serial_=staged->info().id;search_generation_=std::move(staged);}
  progress.finish(result.cancelled);return result;
}
SearchGenerationInfo StudySession::search_generation() const {return search_generation_?search_generation_->info():SearchGenerationInfo{};}
void StudySession::set_search_retention_limits(SearchRetentionLimits limits){detail::validate_search_retention_limits(limits);search_retention_limits_=limits;}
SearchGenerationRead StudySession::read_search_generation(std::uint64_t id,std::size_t group,SearchReferenceSource source,const SearchLimits &limits) const {
  detail::validate_search_generation_read(source,limits);
  return search_generation_?search_generation_->read(id,group,source,limits):SearchGenerationRead{};
}
SearchScopeInfo StudySession::last_search_scope() const {return search_scope_?search_scope_->info():SearchScopeInfo{};}
void StudySession::clear_search_scope(){search_scope_.reset();}
}
