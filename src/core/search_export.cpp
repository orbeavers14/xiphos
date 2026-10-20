// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "xiphos/core/export_item.hpp"
#include "search_generation.hpp"
#include "search_control.hpp"
#include <glib.h>
#include <algorithm>
#include <stdexcept>
namespace xiphos::core {
namespace {
using Rows=std::vector<std::pair<std::string,std::string>>;
void validate_output(const std::string &title,std::size_t bytes){
  if(!bytes||bytes>maximum_search_export_bytes||title.size()>32768||title.find('\0')!=std::string::npos||
     !g_utf8_validate(title.data(),static_cast<gssize>(title.size()),nullptr))throw std::invalid_argument("Invalid search text export options");
}
SearchTextExport fail_export(SearchTextExport out,SearchExportStatus status,std::optional<std::size_t> row=std::nullopt){
  out.status=status;out.failed_row=row;out.row_count=0;out.text.clear();return out;
}
bool stopped(const std::shared_ptr<SearchControl> &control){return control&&control->cancel_requested();}
// One document policy for explicit visible selections and complete returned keys.
// The owning session supplies configured SDK reads, never a frontend renderer.
template<class Read>SearchTextExport render_text(const std::string &title,std::size_t cap,const Rows &rows,
  SearchTextExport out,const std::shared_ptr<SearchControl> &control,Read read){
  if(title.size()+3>cap)return fail_export(std::move(out),SearchExportStatus::output_limit);
  std::string staged=title+"\n\n";
  for(std::size_t i=0;i<rows.size();++i){
    if(stopped(control))return fail_export(std::move(out),SearchExportStatus::cancelled,i);
    const auto &row=rows[i];const auto description=row.second+" "+row.first;
    const auto base=10+description.size()+row.first.size()+row.second.size();
    const auto remaining=cap-staged.size()-1; // Reserve final document newline.
    if(base>remaining)return fail_export(std::move(out),SearchExportStatus::output_limit,i);
    std::string scripture;
    if(out.includes_scripture){
      try {scripture=read(row.first,row.second,remaining-base);}
      catch(const std::length_error &){return fail_export(std::move(out),SearchExportStatus::output_limit,i);}
      catch(const std::out_of_range &){return fail_export(std::move(out),SearchExportStatus::module_not_found,i);}
      catch(const std::invalid_argument &){return fail_export(std::move(out),SearchExportStatus::engine_error,i);}
      catch(const std::runtime_error &){return fail_export(std::move(out),SearchExportStatus::engine_error,i);}
    }
    if(stopped(control))return fail_export(std::move(out),SearchExportStatus::cancelled,i);
    const auto item=format_export_item(description,row.first,row.second,scripture,false,out.includes_scripture);
    if(item.size()>remaining)return fail_export(std::move(out),SearchExportStatus::output_limit,i);
    staged+=item;
    if(control)detail::SearchControlAccess::progress(*control,static_cast<unsigned>((i+1)*100/rows.size()));
  }
  staged+='\n';
  if(stopped(control))return fail_export(std::move(out),SearchExportStatus::cancelled);
  out.status=SearchExportStatus::ok;out.row_count=rows.size();out.text=std::move(staged);return out;
}
}
SearchTextExport StudySession::export_search_visible_text(SearchTextExportRequest request,std::shared_ptr<SearchControl> control){
  detail::SearchControlScope progress(control);validate_output(request.title,request.max_bytes);
  if(request.rows.empty()||request.rows.size()>maximum_search_export_rows)throw std::invalid_argument("Invalid visible search selection");
  SearchTextExport out;out.generation=search_generation();out.includes_scripture=request.include_scripture;
  auto finish=[&](SearchTextExport value){progress.finish(value.status==SearchExportStatus::cancelled);return value;};
  if(!request.generation||request.generation!=out.generation.id)return finish(fail_export(std::move(out),SearchExportStatus::stale));
  if(stopped(control))return finish(fail_export(std::move(out),SearchExportStatus::cancelled));
  Rows rows;rows.reserve(request.rows.size());std::size_t reference_bytes=0;
  for(std::size_t i=0;i<request.rows.size();++i){
    const auto row=search_generation_->visible_row(request.rows[i].group,request.rows[i].row);
    if(!row)return finish(fail_export(std::move(out),SearchExportStatus::invalid_selection,i));
    if(row->first.size()+row->second.size()>maximum_search_reference_bytes-reference_bytes)return finish(fail_export(std::move(out),SearchExportStatus::output_limit,i));
    reference_bytes+=row->first.size()+row->second.size();rows.push_back(*row);
  }
  return finish(render_text(request.title,request.max_bytes,rows,std::move(out),control,[&](const auto &module,const auto &key,std::size_t bytes){
    return reader_.read_stripped(module,key,module_engine_options(module),bytes);
  }));
}
SearchTextExport StudySession::export_search_all_text(SearchAllTextExportRequest request,std::shared_ptr<SearchControl> control){
  detail::SearchControlScope progress(control);validate_output(request.title,request.max_bytes);
  if(!request.max_rows||request.max_rows>maximum_search_export_rows)throw std::invalid_argument("Invalid complete search export row budget");
  SearchTextExport out;out.generation=search_generation();out.includes_scripture=request.include_scripture;
  out.selection=SearchExportSelection::all_retained;
  auto finish=[&](SearchTextExport value){progress.finish(value.status==SearchExportStatus::cancelled);return value;};
  if(!request.generation||request.generation!=out.generation.id)return finish(fail_export(std::move(out),SearchExportStatus::stale));
  if(stopped(control))return finish(fail_export(std::move(out),SearchExportStatus::cancelled));
  if(out.generation.retention!=SearchRetentionStatus::complete)return finish(fail_export(std::move(out),SearchExportStatus::capacity_exceeded));
  if(!request.allow_partial_source&&(out.generation.cancelled||out.generation.failed_groups||out.generation.group_count!=out.generation.requested_count))
    return finish(fail_export(std::move(out),SearchExportStatus::source_incomplete));
  Rows rows;std::size_t reference_bytes=0;
  for(std::size_t group=0;group<out.generation.group_count;++group){
    if(stopped(control))return finish(fail_export(std::move(out),SearchExportStatus::cancelled));
    // Read at least one row to distinguish an empty group at exact budget from
    // more matches. A clipped complete group always rejects the whole document.
    const auto available=request.max_rows-rows.size();
    const auto value=search_generation_->read(request.generation,group,SearchReferenceSource::complete,
      {std::max(std::size_t{1},available),maximum_search_reference_bytes});
    if(value.status==SearchGenerationReadStatus::failed_group)continue; // Explicit partial-source opt-in above.
    if(value.status!=SearchGenerationReadStatus::ok||!value.result)return finish(fail_export(std::move(out),SearchExportStatus::engine_error));
    if(value.result->limited||value.result->references.size()>available)return finish(fail_export(std::move(out),SearchExportStatus::output_limit));
    for(const auto &key:value.result->references){
      const auto &module=value.requested->module;
      if(module.size()+key.size()>maximum_search_reference_bytes-reference_bytes)return finish(fail_export(std::move(out),SearchExportStatus::output_limit));
      reference_bytes+=module.size()+key.size();rows.emplace_back(module,key);
    }
  }
  return finish(render_text(request.title,request.max_bytes,rows,std::move(out),control,[&](const auto &module,const auto &key,std::size_t bytes){
    return reader_.read_stripped(module,key,module_engine_options(module),bytes);
  }));
}
}
