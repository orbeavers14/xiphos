// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "xiphos/core/export_item.hpp"
#include "search_export_selection.hpp"
#include "search_control.hpp"
#include "html_document.hpp"
#include <glib.h>
#include <array>
#include <memory>
#include <stdexcept>
namespace xiphos::core {
SearchHtmlExport StudySession::export_search_html(SearchHtmlExportRequest request,std::shared_ptr<SearchControl> control){
  detail::SearchControlScope progress(control);detail::validate_search_export_output(request.title,request.max_bytes);
  detail::SearchExportRows rows;SearchHtmlExport out;
  static_cast<SearchExportMetadata &>(out)=detail::resolve_search_export_rows(search_generation_.get(),request.generation,
    request.selection,request.rows,request.max_rows,request.allow_partial_source,control,rows);
  out.includes_scripture=request.include_scripture;
  auto finish=[&](SearchExportStatus status,std::optional<std::size_t> row=std::nullopt){
    out.status=status;out.failed_row=row;progress.finish(status==SearchExportStatus::cancelled);return out;
  };
  if(out.status!=SearchExportStatus::ok)return finish(out.status,out.failed_row);
  const std::unique_ptr<gchar,decltype(&g_free)> escaped(g_markup_escape_text(request.title.data(),static_cast<gssize>(request.title.size())),g_free);
  const std::string title=escaped.get();
  const auto prefix=std::string("<!DOCTYPE html><html><head><meta charset=\"UTF-8\">")+
    "<meta http-equiv=\"Content-Security-Policy\" content=\"default-src 'none'; style-src 'unsafe-inline'; img-src 'none'; base-uri 'none'; form-action 'none'\">"+
    "<title>"+title+"</title>";
  const bool partial=out.generation.cancelled||out.generation.failed_groups||out.generation.group_count!=out.generation.requested_count;
  const auto middle=std::string("</head><body><h1>")+title+"</h1><p>"+
    (out.selection==SearchExportSelection::visible?"Selected search passages.":"All returned search matches.")+"</p>"+
    (partial?"<p>Source search stopped or had module failures; only returned matches are included.</p>":"")+"<ul>";
  const std::string suffix="</ul></body></html>";
  std::size_t used=prefix.size()+middle.size()+suffix.size();
  if(used>request.max_bytes)return finish(SearchExportStatus::output_limit);
  std::string body,css;std::array<bool,4> styles{};
  for(std::size_t i=0;i<rows.size();++i){
    if(control&&control->cancel_requested())return finish(SearchExportStatus::cancelled,i);
    const auto &row=rows[i];const auto description=row.second+" "+row.first;
    const auto empty=format_export_item(description,row.first,row.second,"",true,out.includes_scripture);
    if(empty.size()>request.max_bytes-used)return finish(SearchExportStatus::output_limit,i);
    std::string scripture;
    if(out.includes_scripture){
      EntryHtmlSource source;
      try{source=capture_entry_html(row.first,row.second,{request.max_bytes});}
      catch(const std::length_error &){return finish(SearchExportStatus::output_limit,i);}
      catch(const std::out_of_range &){return finish(SearchExportStatus::module_not_found,i);}
      catch(const std::invalid_argument &){return finish(SearchExportStatus::engine_error,i);}
      catch(const std::runtime_error &){return finish(SearchExportStatus::engine_error,i);}
      const auto prepared=detail::prepare_export_html(source,request.max_bytes);
      if(prepared.limited)return finish(SearchExportStatus::output_limit,i);
      if(prepared.issue!=HtmlExportIssue::none){out.issue=prepared.issue;return finish(SearchExportStatus::unsupported_html,i);}
      if(prepared.style&&!styles[prepared.style]){
        const auto extra=prepared.css.size()+(css.empty()?15:0); // style element once.
        if(extra>request.max_bytes-used)return finish(SearchExportStatus::output_limit,i);
        used+=extra;css+=prepared.css;styles[prepared.style]=true;
      }
      scripture="<div class=\"xiphos-export-style-"+std::to_string(prepared.style)+"\">"+prepared.html+"</div>";
    }
    if(control&&control->cancel_requested())return finish(SearchExportStatus::cancelled,i);
    const auto item=format_export_item(description,row.first,row.second,scripture,true,out.includes_scripture);
    if(item.size()>request.max_bytes-used)return finish(SearchExportStatus::output_limit,i);
    used+=item.size();body+=item;
    if(control)detail::SearchControlAccess::progress(*control,static_cast<unsigned>((i+1)*100/rows.size()));
  }
  if(control&&control->cancel_requested())return finish(SearchExportStatus::cancelled);
  // Final owned allocation occurs before publishing any payload to the caller.
  auto document=prefix+(css.empty()?std::string{}:"<style>"+css+"</style>")+middle+body+suffix;
  if(document.size()!=used||document.size()>request.max_bytes)throw std::logic_error("HTML document accounting mismatch");
  if(control&&control->cancel_requested())return finish(SearchExportStatus::cancelled);
  out.html=std::move(document);out.row_count=rows.size();progress.finish(false);return out;
}
}
