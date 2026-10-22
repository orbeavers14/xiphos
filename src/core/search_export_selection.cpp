// SPDX-License-Identifier: GPL-2.0-or-later
#include "search_export_selection.hpp"
#include "search_generation.hpp"
#include <glib.h>
#include <algorithm>
#include <map>
#include <stdexcept>
namespace xiphos::core::detail {
void validate_search_export_output(const std::string &title,std::size_t bytes){
  if(!bytes||bytes>maximum_search_export_bytes||title.size()>32768||title.find('\0')!=std::string::npos||
     !g_utf8_validate(title.data(),static_cast<gssize>(title.size()),nullptr))throw std::invalid_argument("Invalid search export options");
}
SearchExportMetadata resolve_search_export_rows(const SearchGeneration *generation,std::uint64_t expected,
  SearchExportSelection selection,const std::vector<SearchVisibleRow> &selected,std::size_t max_rows,
  bool allow_partial,const std::shared_ptr<SearchControl> &control,SearchExportRows &rows,bool include_context){
  rows.clear();
  if(!max_rows||max_rows>maximum_search_export_rows||
     (selection!=SearchExportSelection::visible&&selection!=SearchExportSelection::all_retained)||
     (selection==SearchExportSelection::visible&&(selected.empty()||selected.size()>maximum_search_export_rows))||
     (selection==SearchExportSelection::all_retained&&!selected.empty()))throw std::invalid_argument("Invalid search export selection");
  SearchExportMetadata out;out.selection=selection;if(generation)out.generation=generation->info();
  auto fail=[&](SearchExportStatus status,std::optional<std::size_t> row=std::nullopt){out.status=status;out.failed_row=row;rows.clear();return out;};
  if(!expected||expected!=out.generation.id)return fail(SearchExportStatus::stale);
  if(control&&control->cancel_requested())return fail(SearchExportStatus::cancelled);
  std::map<std::size_t,std::shared_ptr<const std::string>> contexts;
  auto context=[&](std::size_t group)->std::shared_ptr<const std::string>{
    if(!include_context)return {};
    auto &cached=contexts[group];
    if(!cached)cached=std::make_shared<const std::string>(generation->export_header(group));
    return cached;
  };
  std::size_t bytes=0;
  if(selection==SearchExportSelection::visible){
    if(selected.size()>max_rows)return fail(SearchExportStatus::output_limit,max_rows);
    rows.reserve(selected.size());
    for(std::size_t i=0;i<selected.size();++i){
      const auto row=generation->visible_row(selected[i].group,selected[i].row);
      if(!row)return fail(SearchExportStatus::invalid_selection,i);
      if(row->first.size()+row->second.size()>maximum_search_reference_bytes-bytes)return fail(SearchExportStatus::output_limit,i);
      bytes+=row->first.size()+row->second.size();rows.push_back({row->first,row->second,selected[i].group,context(selected[i].group)});
    }
  }else{
    if(out.generation.retention!=SearchRetentionStatus::complete)return fail(SearchExportStatus::capacity_exceeded);
    if(!allow_partial&&(out.generation.cancelled||out.generation.failed_groups||out.generation.group_count!=out.generation.requested_count))
      return fail(SearchExportStatus::source_incomplete);
    for(std::size_t group=0;group<out.generation.group_count;++group){
      if(control&&control->cancel_requested())return fail(SearchExportStatus::cancelled);
      const auto available=max_rows-rows.size();
      const auto value=generation->read(expected,group,SearchReferenceSource::complete,{std::max(std::size_t{1},available),maximum_search_reference_bytes});
      if(value.status==SearchGenerationReadStatus::failed_group)continue;
      if(value.status!=SearchGenerationReadStatus::ok||!value.result)return fail(SearchExportStatus::engine_error);
      if(value.result->limited||value.result->references.size()>available)return fail(SearchExportStatus::output_limit);
      for(const auto &key:value.result->references){
        const auto &module=value.requested->module;
        if(module.size()+key.size()>maximum_search_reference_bytes-bytes)return fail(SearchExportStatus::output_limit);
        bytes+=module.size()+key.size();rows.push_back({module,key,group,context(group)});
      }
    }
  }
  out.status=SearchExportStatus::ok;return out;
}
}
