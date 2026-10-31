// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/annotations.hpp"
#include "xml_document.hpp"
#include "preference_file.hpp"
#include <glib.h>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
namespace xiphos::core {
namespace {
std::optional<int> annotation_integer(const std::string &text) {
  std::size_t at=0;while(at<text.size()&&g_ascii_isspace(text[at]))++at;
  bool negative=false;if(at<text.size()&&(text[at]=='-'||text[at]=='+'))negative=text[at++]=='-';
  const auto maximum=static_cast<unsigned long long>(std::numeric_limits<int>::max())+(negative?1:0);
  unsigned long long value=0;
  while(at<text.size()&&text[at]>='0'&&text[at]<='9'){
    const auto digit=static_cast<unsigned>(text[at++]-'0');if(value>(maximum-digit)/10)return std::nullopt;value=value*10+digit;
  }
  const auto signed_value=negative?-static_cast<long long>(value):static_cast<long long>(value);
  return static_cast<int>(signed_value);
}
void annotation_account(std::size_t &bytes,const std::string &text,std::size_t maximum) {
  detail::validate_xml_text(text,maximum);
  if(text.size()>maximum_annotation_snapshot_bytes-bytes)throw std::invalid_argument("Annotation snapshot exceeds byte bounds");bytes+=text.size();
}
}
std::optional<AnnotationAddress> decode_annotation_label(const std::string &label) {
  detail::validate_xml_text(label,maximum_annotation_label_bytes);
  const auto last=label.rfind('.');if(last==std::string::npos||!last)return std::nullopt;
  const auto previous=label.rfind('.',last-1);if(previous==std::string::npos)return std::nullopt;
  const auto space=label.find(' ');if(space==std::string::npos||space>=previous)return std::nullopt;
  const auto chapter=annotation_integer(label.substr(previous+1,last-previous-1)),verse=annotation_integer(label.substr(last+1));
  if(!chapter||!verse)return std::nullopt;
  const long long scaled=static_cast<long long>(*chapter)*1000;
  if(scaled<std::numeric_limits<int>::min()||scaled>std::numeric_limits<int>::max())return std::nullopt;
  const long long packed=scaled+*verse;
  if(packed<std::numeric_limits<int>::min()||packed>std::numeric_limits<int>::max())return std::nullopt;
  return AnnotationAddress{label.substr(0,space),label.substr(space+1,previous-space-1),*chapter,*verse,static_cast<int>(packed)};
}
std::string annotation_note_html(const std::string &note) {
  detail::validate_xml_text(note,maximum_annotation_note_bytes);std::string result;result.reserve(note.size());
  for(char c:note){switch(c){case '&':result+="&amp;";break;case '<':result+="&lt;";break;case '>':result+="&gt;";break;
    case '\n':result+="<br />";break;case '"':result+="&quot;";break;case '\'':result+="&apos;";break;default:result+=c;}}
  return result;
}
AnnotationProfile parse_annotations(const std::string &source) {
  auto document=detail::parse_xml_document(source,"Xiphos");AnnotationProfile result;xmlNode *section=nullptr;
  for(auto *node=xmlDocGetRootElement(document.get())->children;node;node=node->next)
    if(!xmlStrcmp(node->name,BAD_CAST "osisrefmarkedverses")){if(!section)section=node;else ++result.shadowed_sections;}
  if(!section)return result;
  auto attribute=[&](xmlNode *node,const char *name,std::size_t maximum)->std::optional<std::string>{
    const std::unique_ptr<xmlChar,decltype(xmlFree)> text(xmlGetProp(node,BAD_CAST name),xmlFree);if(!text)return std::nullopt;
    std::string copied(reinterpret_cast<const char *>(text.get()));annotation_account(result.snapshot_bytes,copied,maximum);return copied;
  };
  std::size_t child=0;bool visible=true;
  for(auto *node=section->children;node;node=node->next,++child){
    const std::unique_ptr<xmlChar,decltype(xmlFree)> label(xmlGetProp(node,BAD_CAST "label"),xmlFree);
    if(!label&&visible){result.legacy_stop_child=child;visible=false;}
    if(node->type!=XML_ELEMENT_NODE)continue;
    if(result.records.size()==maximum_annotation_records)throw std::invalid_argument("Too many annotation records");
    AnnotationRecord record;record.element=reinterpret_cast<const char *>(node->name);annotation_account(result.snapshot_bytes,record.element,maximum_annotation_label_bytes);
    if(label){record.label=reinterpret_cast<const char *>(label.get());annotation_account(result.snapshot_bytes,*record.label,maximum_annotation_label_bytes);record.address=decode_annotation_label(*record.label);
      if(record.address){annotation_account(result.snapshot_bytes,record.address->module,maximum_annotation_label_bytes);annotation_account(result.snapshot_bytes,record.address->book,maximum_annotation_label_bytes);}}
    record.note=attribute(node,"list",maximum_annotation_note_bytes);record.legacy_visible=visible;result.records.push_back(std::move(record));
  }
  return result;
}
AnnotationProfile read_annotations(const std::string &path) {
  if(path.empty()||path.find('\0')!=std::string::npos)throw std::invalid_argument("Annotation path is invalid");const auto file=std::filesystem::u8path(path);
  if(!std::filesystem::is_regular_file(file))throw std::invalid_argument("Annotations require a regular selected file");
  return parse_annotations(detail::read_preference_file(file,detail::maximum_xml_document_bytes));
}
AnnotationCache legacy_annotation_cache(const AnnotationProfile &profile,const std::string &module,const std::string &book) {
  if(module.empty()||book.empty()||profile.records.size()>maximum_annotation_records)throw std::invalid_argument("Invalid annotation cache context");
  AnnotationCache result;result.module=module;result.book=book;annotation_account(result.snapshot_bytes,module,maximum_annotation_label_bytes);annotation_account(result.snapshot_bytes,book,maximum_annotation_label_bytes);
  std::size_t input_bytes=0;std::map<int,VerseAnnotation> entries;
  // Validate all copied raw fields, including records beyond the visible prefix.
  bool blocked=false;
  for(const auto &record:profile.records){
    if(!record.legacy_visible)blocked=true;
    else if(blocked||!record.label)throw std::invalid_argument("Invalid annotation cursor prefix");
    annotation_account(input_bytes,record.element,maximum_annotation_label_bytes);if(record.label)annotation_account(input_bytes,*record.label,maximum_annotation_label_bytes);if(record.note)annotation_account(input_bytes,*record.note,maximum_annotation_note_bytes);
  }
  for(std::size_t i=0;i<profile.records.size();++i){const auto &record=profile.records[i];
    if(!record.legacy_visible){if(record.label)result.incomplete=true;continue;}
    if(!record.label)break;const auto address=decode_annotation_label(*record.label);
    if(!address){result.malformed_record=i;result.incomplete=true;break;}
    if((!address->module.empty()&&g_ascii_strcasecmp(address->module.c_str(),module.c_str()))||g_ascii_strcasecmp(address->book.c_str(),book.c_str()))continue;
    auto html=annotation_note_html(record.note.value_or(""));auto found=entries.find(address->packed);
    const auto prior=found==entries.end()?0:found->second.html.size();
    const auto retained=result.snapshot_bytes-prior;
    if(html.size()>maximum_annotation_snapshot_bytes-retained)throw std::invalid_argument("Annotation HTML cache exceeds bounds");
    result.snapshot_bytes=retained+html.size();entries[address->packed]={address->chapter,address->verse,address->packed,i,std::move(html)};
  }
  result.entries.reserve(entries.size());for(auto &entry:entries)result.entries.push_back(std::move(entry.second));return result;
}
}
