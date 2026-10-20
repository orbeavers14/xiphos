// SPDX-License-Identifier: GPL-2.0-or-later
#include "search_generation.hpp"
#include "search_results.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace xiphos::core::detail {
void validate_search_generation_read(SearchReferenceSource source,const SearchLimits &limits){
  if((source!=SearchReferenceSource::visible&&source!=SearchReferenceSource::complete)||!limits.max_results||
     limits.max_results>maximum_search_results||!limits.max_reference_bytes||limits.max_reference_bytes>maximum_search_reference_bytes)
    throw std::invalid_argument("Invalid retained search read options");
}
void validate_search_retention_limits(SearchRetentionLimits limits){
  if(!limits.max_elements||limits.max_elements>maximum_search_retained_elements||!limits.max_key_bytes||limits.max_key_bytes>maximum_search_retained_key_bytes)
    throw std::invalid_argument("Unsupported complete search retention limits");
}
std::uint64_t next_search_generation_id(std::uint64_t current){
  if(current==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("Search generation ID exhausted");
  return current+1;
}
SearchRetainedKeys retain_search_keys(const sword::ListKey &keys,std::shared_ptr<const SearchScope> reusable,SearchRetentionLimits limits){
  if(keys.getCount()<0)throw std::runtime_error("Invalid retained search count");
  if(static_cast<std::uint64_t>(keys.getCount())>limits.max_elements)return {{},true};
  if(reusable){
    if(reusable->accounted_key_bytes()>limits.max_key_bytes)return {{},true};
    return {std::move(reusable),false};
  }
  if(keys.getCount()&&!limits.max_key_bytes)return {{},true};
  try {
    return {std::make_shared<const SearchScope>(keys,SearchScopeLimits{
      std::max(std::size_t{1},limits.max_elements),std::max(std::size_t{1},limits.max_key_bytes)}),false};
  }catch(const std::length_error &){return {{},true};}
}
SearchGenerationBuilder::SearchGenerationBuilder(std::uint64_t id,std::vector<SearchQuery> queries,SearchRetentionLimits limits):limits_(limits){
  validate_search_retention_limits(limits);
  if(!id||queries.empty()||queries.size()>maximum_search_batch_modules)throw std::invalid_argument("Invalid search generation");
  value_.info_.id=id;value_.info_.retention=SearchRetentionStatus::complete;
  value_.info_.requested_count=queries.size();value_.requested_=std::move(queries);
  value_.groups_.reserve(value_.requested_.size());
}
SearchRetentionLimits SearchGenerationBuilder::remaining() const noexcept {
  if(!wants_keys())return {0,0};
  return {limits_.max_elements-static_cast<std::size_t>(value_.info_.retained_elements),limits_.max_key_bytes-value_.info_.accounted_key_bytes};
}
void SearchGenerationBuilder::append(std::size_t index,const std::optional<SearchResult> &result,SearchBatchFailure failure,SearchRetainedKeys keys){
  if(index!=value_.groups_.size()||index>=value_.requested_.size()||(result.has_value()!=(failure==SearchBatchFailure::none)))throw std::logic_error("Inconsistent retained search group");
  SearchGeneration::Group group{result,failure,{}}; // All metadata allocation precedes publication.
  auto &info=value_.info_;
  if(result){info.visible_results+=result->references.size();info.matched_elements+=result->total_results;}
  else ++info.failed_groups;
  if(wants_keys()&&keys.capacity_exceeded){
    info.retention=SearchRetentionStatus::capacity_exceeded;info.retained_elements=0;info.accounted_key_bytes=0;
    for(auto &prior:value_.groups_)prior.keys.reset(); // owner-only SDK destruction
  }
  if(wants_keys()&&result&&result->executed){
    if(!keys.keys)throw std::logic_error("Executed group has no retained keys");
    const auto count=keys.keys->info().element_count;const auto bytes=keys.keys->accounted_key_bytes();
    if(count>remaining().max_elements||bytes>remaining().max_key_bytes)throw std::logic_error("Retention accounting exceeds generation budget");
    info.retained_elements+=count;info.accounted_key_bytes+=bytes;group.keys=std::move(keys.keys);
  }
  value_.groups_.push_back(std::move(group));info.group_count=value_.groups_.size();
}
std::shared_ptr<const SearchGeneration> SearchGenerationBuilder::finish(bool cancelled){
  value_.info_.cancelled=cancelled;
  for(auto &group:value_.groups_)if(group.result){group.result->generation=value_.info_;group.result->generation_updated=true;}
  return std::make_shared<const SearchGeneration>(std::move(value_));
}
SearchGenerationRead SearchGeneration::read(std::uint64_t id,std::size_t index,SearchReferenceSource source,const SearchLimits &limits) const {
  validate_search_generation_read(source,limits);
  SearchGenerationRead out;out.generation=info_;
  if(id!=info_.id)return out;
  if(index>=requested_.size()){out.status=SearchGenerationReadStatus::invalid_group;return out;}
  out.requested=requested_[index];
  if(index>=groups_.size()){out.status=SearchGenerationReadStatus::not_attempted;return out;}
  const auto &group=groups_[index];out.failure=group.failure;
  if(!group.result){out.status=SearchGenerationReadStatus::failed_group;return out;}
  if(source==SearchReferenceSource::complete&&info_.retention!=SearchRetentionStatus::complete){out.status=SearchGenerationReadStatus::capacity_exceeded;return out;}
  out.result=*group.result;
  if(source==SearchReferenceSource::complete){
    if(group.keys){auto traversal=group.keys->keys();auto copied=copy_search_references(traversal,limits);out.result->references=std::move(copied.references);out.result->limited=copied.limited;}
    else {out.result->references.clear();out.result->limited=false;}
  }else{
    std::size_t bytes=0,count=0;
    for(const auto &key:out.result->references){if(count==limits.max_results||key.size()>limits.max_reference_bytes-bytes)break;bytes+=key.size();++count;}
    out.result->limited|=count<out.result->references.size();out.result->references.resize(count);
  }
  out.status=SearchGenerationReadStatus::ok;return out;
}
}
