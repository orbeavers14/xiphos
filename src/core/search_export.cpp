// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "xiphos/core/export_item.hpp"
#include "search_generation.hpp"
#include "search_export_selection.hpp"
#include "search_control.hpp"
#include "search_export_text.hpp"
#include <stdexcept>
namespace xiphos::core {
SearchTextExport StudySession::export_search_visible_text(SearchTextExportRequest request,std::shared_ptr<SearchControl> control){
  detail::SearchControlScope progress(control);detail::validate_search_export_output(request.title,request.max_bytes);
  detail::SearchExportRows rows;SearchTextExport out;
  static_cast<SearchExportMetadata &>(out)=detail::resolve_search_export_rows(search_generation_.get(),request.generation,
    SearchExportSelection::visible,request.rows,maximum_search_export_rows,false,control,rows,request.include_search_context);
  out.includes_scripture=request.include_scripture;
  if(out.status==SearchExportStatus::ok)out=detail::render_search_export_text(request.title,request.max_bytes,rows,std::move(out),control,[&](const auto &module,const auto &key,std::size_t bytes){
    return reader_.read_stripped(module,key,module_engine_options(module),bytes);
  },true,request.include_search_context);
  progress.finish(out.status==SearchExportStatus::cancelled);return out;
}
SearchTextExport StudySession::export_search_all_text(SearchAllTextExportRequest request,std::shared_ptr<SearchControl> control){
  detail::SearchControlScope progress(control);detail::validate_search_export_output(request.title,request.max_bytes);
  detail::SearchExportRows rows;SearchTextExport out;
  static_cast<SearchExportMetadata &>(out)=detail::resolve_search_export_rows(search_generation_.get(),request.generation,
    SearchExportSelection::all_retained,{},request.max_rows,request.allow_partial_source,control,rows,request.include_search_context);
  out.includes_scripture=request.include_scripture;
  if(out.status==SearchExportStatus::ok)out=detail::render_search_export_text(request.title,request.max_bytes,rows,std::move(out),control,[&](const auto &module,const auto &key,std::size_t bytes){
    return reader_.read_stripped(module,key,module_engine_options(module),bytes);
  },true,request.include_search_context);
  progress.finish(out.status==SearchExportStatus::cancelled);return out;
}
}
