// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "search_control.hpp"
#include "search_scope.hpp"
#include <glib.h>
#include <algorithm>
#include <stdexcept>
namespace xiphos::core {
namespace {
void validate_limits(const SearchLimits &limits) {
  if(!limits.max_results||limits.max_results>maximum_search_results||!limits.max_reference_bytes||
     limits.max_reference_bytes>maximum_search_reference_bytes)
    throw std::invalid_argument("Unsupported batch search output limits");
}
void validate_text(const std::string &value,std::size_t maximum) {
  if(value.empty()||value.size()>maximum||value.find('\0')!=std::string::npos||
     !g_utf8_validate(value.data(),value.size(),nullptr))
    throw std::invalid_argument("Invalid bounded UTF-8 batch search input");
}
}
SearchBatchResult StudySession::search_batch(std::vector<SearchQuery> queries,const SearchBatchLimits &limits,std::shared_ptr<SearchControl> control) {
  detail::SearchControlScope progress(control);
  validate_limits(limits.per_module);validate_limits(limits.total);
  if(queries.empty()||queries.size()>maximum_search_batch_modules)throw std::invalid_argument("Search batch needs 1 to 32 queries");
  // Validate every request before the first SDK execution or session mutation.
  for(const auto &query:queries){
    validate_text(query.module,4096);validate_text(query.expression,32768);
    if(static_cast<int>(query.mode)<0||static_cast<int>(query.mode)>static_cast<int>(SearchMode::indexed))
      throw std::invalid_argument("Unknown batch search mode");
    if(query.range){
      if(query.within_last_results)throw std::invalid_argument("Batch range excludes last-results scope");
      validate_text(query.range->module,4096);validate_text(query.range->expression,4096);
    }
  }
  SearchBatchResult output;output.requested=std::move(queries);output.groups.reserve(output.requested.size());
  // Copy only immutable search scope and small search settings, never tab history,
  // book snapshots or a second SDK manager. Publish once all owned output exists.
  auto scope=search_scope_;auto text=search_text_options_;
  for(std::size_t index=0;index<output.requested.size();++index){
    if(control&&control->cancel_requested()){output.cancelled=true;break;}
    const auto begin=static_cast<unsigned>(index*100/output.requested.size());
    const auto end=static_cast<unsigned>((index+1)*100/output.requested.size());
    SearchBatchGroup group;group.request_index=index;
    const SearchLimits remaining{
      std::min(limits.per_module.max_results,limits.total.max_results-output.copied_results),
      std::min(limits.per_module.max_reference_bytes,limits.total.max_reference_bytes-output.copied_reference_bytes)};
    try {
      auto result=reader_.search_impl(output.requested[index],remaining,control,&scope,text,true,begin,end);
      if(result.effective_text_options&&result.requested.text_options&&result.requested.mode!=SearchMode::phrase){
        text=result.requested.text_options;result.text_options_updated=true;
      }
      output.scope_updated|=result.scope_updated;output.text_options_updated|=result.text_options_updated;
      output.copied_results+=result.references.size();
      for(const auto &reference:result.references)output.copied_reference_bytes+=reference.size();
      output.limited|=result.limited;output.cancelled|=result.cancelled;
      group.result=std::move(result);
    } catch(const std::out_of_range &){group.failure=SearchBatchFailure::module_not_found;}
      catch(const std::invalid_argument &){group.failure=SearchBatchFailure::invalid_request;}
      catch(const std::length_error &){group.failure=SearchBatchFailure::engine_error;}
      catch(const std::runtime_error &){group.failure=SearchBatchFailure::engine_error;}
    if(group.failure!=SearchBatchFailure::none)++output.failures;
    output.groups.push_back(std::move(group));
    if(output.cancelled)break;
    if(control)detail::SearchControlAccess::progress(*control,end);
  }
  // A cancellation between modules yields the completed prefix. Late cancellation
  // after the final module can lose to completion, as for single-module jobs.
  output.scope=scope?scope->info():SearchScopeInfo{};output.text_options={text};
  search_scope_=std::move(scope);search_text_options_=text;
  progress.finish(output.cancelled);return output;
}
}
