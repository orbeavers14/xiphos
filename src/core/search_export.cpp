// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "xiphos/core/export_item.hpp"
#include "search_generation.hpp"
#include "search_control.hpp"
#include <glib.h>
#include <stdexcept>
namespace xiphos::core {
SearchTextExport StudySession::export_search_visible_text(SearchTextExportRequest request,std::shared_ptr<SearchControl> control){
  detail::SearchControlScope progress(control);
  if(request.rows.empty()||request.rows.size()>maximum_search_export_rows||!request.max_bytes||request.max_bytes>maximum_search_export_bytes||
     request.title.size()>32768||request.title.find('\0')!=std::string::npos||!g_utf8_validate(request.title.data(),static_cast<gssize>(request.title.size()),nullptr))
    throw std::invalid_argument("Invalid visible search text export request");
  SearchTextExport out;out.generation=search_generation();out.includes_scripture=request.include_scripture;
  auto fail=[&](SearchExportStatus status,std::optional<std::size_t> row=std::nullopt){
    out.status=status;out.failed_row=row;progress.finish(status==SearchExportStatus::cancelled);return out;
  };
  if(!request.generation||request.generation!=out.generation.id)return fail(SearchExportStatus::stale);
  if(control&&control->cancel_requested())return fail(SearchExportStatus::cancelled);
  // Resolve/validate every ordinal before SDK reads. Reordering and duplicate
  // identities are deliberate; no lookup through the tab's final last scope.
  std::vector<std::pair<std::string,std::string>> rows;rows.reserve(request.rows.size());
  std::size_t reference_bytes=0;
  for(std::size_t i=0;i<request.rows.size();++i){
    const auto row=search_generation_->visible_row(request.rows[i].group,request.rows[i].row);
    if(!row)return fail(SearchExportStatus::invalid_selection,i);
    if(row->first.size()+row->second.size()>maximum_search_reference_bytes-reference_bytes)return fail(SearchExportStatus::output_limit,i);
    reference_bytes+=row->first.size()+row->second.size();rows.push_back(*row);
  }
  // English/localized title supplied as plain text, no fabricated result counts.
  // Item description matches legacy advanced export: key followed by module.
  if(request.title.size()+3>request.max_bytes)return fail(SearchExportStatus::output_limit);
  std::string staged=request.title+"\n\n";
  for(std::size_t i=0;i<rows.size();++i){
    if(control&&control->cancel_requested())return fail(SearchExportStatus::cancelled,i);
    const auto &row=rows[i];
    const auto description=row.second+" "+row.first;
    const auto base=10+description.size()+row.first.size()+row.second.size();
    // Reserve the final document newline as well as the exact item overhead.
    const auto remaining=request.max_bytes-staged.size()-1;
    if(base>remaining)return fail(SearchExportStatus::output_limit,i);
    std::string scripture;
    if(request.include_scripture){
      try {scripture=reader_.read_stripped(row.first,row.second,module_engine_options(row.first),remaining-base);}
      catch(const std::length_error &){return fail(SearchExportStatus::output_limit,i);}
      catch(const std::out_of_range &){return fail(SearchExportStatus::module_not_found,i);}
      catch(const std::invalid_argument &){return fail(SearchExportStatus::engine_error,i);}
      catch(const std::runtime_error &){return fail(SearchExportStatus::engine_error,i);}
    }
    if(control&&control->cancel_requested())return fail(SearchExportStatus::cancelled,i);
    const auto item=format_export_item(description,row.first,row.second,scripture,false,request.include_scripture);
    if(item.size()>remaining)return fail(SearchExportStatus::output_limit,i);
    staged+=item;
    if(control)detail::SearchControlAccess::progress(*control,static_cast<unsigned>((i+1)*100/rows.size()));
  }
  staged+='\n';
  if(control&&control->cancel_requested())return fail(SearchExportStatus::cancelled);
  out.status=SearchExportStatus::ok;out.row_count=rows.size();out.text=std::move(staged);
  progress.finish(false);return out;
}
}
