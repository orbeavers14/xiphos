// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/editable_document.hpp"
#include "bounded_file.hpp"
#include "preference_file.hpp"
#include <glib.h>
#include <algorithm>
#include <fstream>
#include <memory>
#include <stdexcept>
namespace xiphos::core {
namespace {
void validate_path(const std::string &path){if(path.empty()||path.size()>4096||path.find('\0')!=std::string::npos)throw std::invalid_argument("Invalid editor document path");}
std::string revision(const std::string &text){
  std::unique_ptr<gchar,decltype(&g_free)> value(g_compute_checksum_for_data(G_CHECKSUM_SHA256,reinterpret_cast<const guchar *>(text.data()),text.size()),g_free);
  if(!value)throw std::bad_alloc();return value.get();
}
EditableDocument document(std::string text){
  if(!editable_document_text(text))throw std::invalid_argument("Editor documents require bounded UTF-8 text without NUL");
  auto digest=revision(text);return {std::move(text),std::move(digest)};
}
}
bool editable_document_text(std::string_view text) noexcept {
  return text.size()<=maximum_editable_document_bytes&&text.find('\0')==std::string_view::npos&&
    (text.empty()||g_utf8_validate(text.data(),static_cast<gssize>(text.size()),nullptr));
}
bool editable_document_revision(std::string_view value) noexcept {
  return value.size()==64&&std::all_of(value.begin(),value.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');});
}
EditableDocument read_editable_document(const std::string &path){validate_path(path);if(!std::filesystem::is_regular_file(std::filesystem::u8path(path)))throw std::invalid_argument("Editor source is not a regular file");return document(detail::read_bounded_file(std::filesystem::u8path(path),maximum_editable_document_bytes,"Editor document"));}
EditableDocument save_editable_document(const std::string &path,const std::optional<std::string> &expected,const std::string &text){
  validate_path(path);if(expected&&!editable_document_revision(*expected))throw std::invalid_argument("Invalid editor revision");
  const auto source=std::filesystem::u8path(path);if(std::filesystem::exists(source)&&!std::filesystem::is_regular_file(source))throw std::invalid_argument("Editor destination is not a regular file");
  auto prepared=document(text);
  detail::replace_preference_file(path,[&](const std::filesystem::path &temp,const std::string &original,bool existed){
    if(existed!=expected.has_value()||(existed&&revision(original)!=*expected))throw std::invalid_argument("Document changed; reopen before saving");
    std::ofstream output(temp,std::ios::binary);output.exceptions(std::ios::failbit|std::ios::badbit);output.write(prepared.text.data(),static_cast<std::streamsize>(prepared.text.size()));output.close();
    if(detail::read_bounded_file(temp,maximum_editable_document_bytes,"Staged editor document")!=prepared.text)throw std::runtime_error("Staged editor document bytes differ");
  },maximum_editable_document_bytes);
  return prepared;
}
}
