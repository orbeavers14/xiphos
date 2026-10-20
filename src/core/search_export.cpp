// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "xiphos/core/export_item.hpp"
#include "search_generation.hpp"
#include "search_export_selection.hpp"
#include "search_control.hpp"
#include <stdexcept>
namespace xiphos::core {
namespace {
using Rows=detail::SearchExportRows;
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
  detail::SearchControlScope progress(control);detail::validate_search_export_output(request.title,request.max_bytes);
  Rows rows;SearchTextExport out;
  static_cast<SearchExportMetadata &>(out)=detail::resolve_search_export_rows(search_generation_.get(),request.generation,
    SearchExportSelection::visible,request.rows,maximum_search_export_rows,false,control,rows);
  out.includes_scripture=request.include_scripture;
  if(out.status==SearchExportStatus::ok)out=render_text(request.title,request.max_bytes,rows,std::move(out),control,[&](const auto &module,const auto &key,std::size_t bytes){
    return reader_.read_stripped(module,key,module_engine_options(module),bytes);
  });
  progress.finish(out.status==SearchExportStatus::cancelled);return out;
}
SearchTextExport StudySession::export_search_all_text(SearchAllTextExportRequest request,std::shared_ptr<SearchControl> control){
  detail::SearchControlScope progress(control);detail::validate_search_export_output(request.title,request.max_bytes);
  Rows rows;SearchTextExport out;
  static_cast<SearchExportMetadata &>(out)=detail::resolve_search_export_rows(search_generation_.get(),request.generation,
    SearchExportSelection::all_retained,{},request.max_rows,request.allow_partial_source,control,rows);
  out.includes_scripture=request.include_scripture;
  if(out.status==SearchExportStatus::ok)out=render_text(request.title,request.max_bytes,rows,std::move(out),control,[&](const auto &module,const auto &key,std::size_t bytes){
    return reader_.read_stripped(module,key,module_engine_options(module),bytes);
  });
  progress.finish(out.status==SearchExportStatus::cancelled);return out;
}
}
