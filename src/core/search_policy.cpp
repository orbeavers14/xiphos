// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/search_policy.hpp"
#include <stdexcept>
namespace xiphos { namespace core {
std::string sidebar_search_text(std::string text, bool indexed) {
  if (text.find('\0') != std::string::npos) throw std::invalid_argument("Search text contains NUL");
  if (!indexed) return text;
  std::string prepared="+";
  for (std::size_t i=0;i<text.size();++i) {
    prepared+=text[i];
    if (text[i]==' ' && i+1<text.size()) prepared+='+';
  }
  prepared+=' ';
  return prepared;
}
AttributeSearchText attribute_search_text(const std::string &text,AttributeSearch kind) {
  if(text.find('\0')!=std::string::npos)throw std::invalid_argument("Attribute search text contains NUL");
  switch(kind){
    case AttributeSearch::lemma:return {"Word//Lemma./"+text,true};
    case AttributeSearch::morphology:return {"Word//Morph/"+text,true};
    case AttributeSearch::footnote:return {"Footnote//body/"+text,false};
  }
  throw std::invalid_argument("Unknown attribute search kind");
}
std::array<SearchTextSetting,3> search_text_settings(const SearchTextOptions &options) {
  return {{{"Strong's Numbers",options.strongs},{"Morphological Tags",options.morphology},{"Footnotes",options.footnotes}}};
}
}}
