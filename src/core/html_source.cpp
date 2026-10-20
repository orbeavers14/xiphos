// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include <glib.h>
#include <stdexcept>
namespace xiphos::core {
namespace {
bool valid_text(const std::string &text){return text.find('\0')==std::string::npos&&g_utf8_validate(text.data(),static_cast<gssize>(text.size()),nullptr);}
}
EntryHtmlSource StudySession::capture_entry_html(const std::string &module,const std::string &reference,const HtmlSourceLimits &limits){
  if(!limits.max_bytes||limits.max_bytes>maximum_html_source_bytes||module.empty()||module.size()>4096||reference.empty()||reference.size()>4096||
     !valid_text(module)||!valid_text(reference))throw std::invalid_argument("Invalid HTML source request");
  if(module.size()>limits.max_bytes)throw std::length_error("HTML source metadata exceeds output capacity");
  // Read the sized returned SDK buffer before any C-string truncation, then use
  // existing cleanup/numbering/layout without publishing learned preferences.
  auto entry=read_entry_impl(module,reference,false,false,&limits.max_bytes);
  std::size_t bytes=0;
  const std::string *fields[]={&module,&entry.reference,&entry.html,&entry.lexical.css};
  for(const auto *text:fields){
    if(text->size()>limits.max_bytes-bytes)throw std::length_error("HTML source exceeds aggregate copied capacity");
    if(!valid_text(*text))throw std::runtime_error("HTML source is not NUL-free UTF-8");
    bytes+=text->size();
  }
  return {module,std::move(entry.reference),std::move(entry.html),std::move(entry.lexical.css),entry.lexical.requested,entry.lexical.applied,bytes};
}
}
