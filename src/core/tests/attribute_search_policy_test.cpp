// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/search_policy.hpp"
#include "../sdk_search.hpp"
#include <glib.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdint>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
struct Widgets { bool rb_strongs,rb_morphs,rb_footnotes; } search1;
#define GTK_TOGGLE_BUTTON(value) (value)
bool gtk_toggle_button_get_active(bool value){return value;}
#define XI_message(...) do {} while(false)
const int SpecialSearchFlag=xiphos::core::detail::sdk_search_whole_entry;
struct Prepared { std::string expression;int flags;bool present; };
Prepared frozen(const char *search_string,int search_type,int search_params){
 const char *attribute_search_string=nullptr;
#include "legacy_attribute_search.inc"
 Prepared result{attribute_search_string?attribute_search_string:"",search_params,attribute_search_string!=nullptr};g_free(const_cast<char *>(attribute_search_string));return result;
}
Prepared actual(const char *search_string,int search_type,int search_params){
 const char *attribute_search_string=nullptr;
#include "legacy_attribute_search_actual.inc"
 Prepared result{attribute_search_string?attribute_search_string:"",search_params,attribute_search_string!=nullptr};g_free(const_cast<char *>(attribute_search_string));return result;
}
void compare(const std::string &text){
 for(int bits=0;bits<8;++bits)for(int type:{-3,-1})for(int flags:{0,1,2,SpecialSearchFlag}){
  search1={bool(bits&1),bool(bits&2),bool(bits&4)};const auto old=frozen(text.c_str(),type,flags),now=actual(text.c_str(),type,flags);
  check(old.expression==now.expression&&old.flags==now.flags&&old.present==now.present,"Actual retained dialog adapter preserves complete frozen grammar/flag/selection behavior");
  if(type==-3&&bits){const auto kind=bits&1?AttributeSearch::lemma:bits&2?AttributeSearch::morphology:AttributeSearch::footnote;const auto core=attribute_search_text(text,kind);check(core.expression==old.expression&&(flags|(core.whole_entry?SpecialSearchFlag:0))==old.flags,"Owned policy equals frozen attribute prefix and whole-entry flags");}
 }
}
int main(){try{
 for(const auto &text:std::vector<std::string>{"","G140","G1401","N-ASF"," a  b ","Word//Lemma/G140","café α β","<w>Original</w>","a/b.*?","\\bG140\\b","first\nsecond",std::string(1000,'x')})compare(text);
 std::uint32_t state=4171;const std::string alphabet=" G0140:/.*?\\\tα";
 for(unsigned n=0;n<200;++n){std::string text;for(unsigned j=0;j<n%43;++j){state=state*1664525u+1013904223u;text+=alphabet[state%alphabet.size()];}compare(text);}
 check(attribute_search_text("G140",AttributeSearch::lemma).expression=="Word//Lemma./G140"&&attribute_search_text("N-ASF",AttributeSearch::morphology).whole_entry&&!attribute_search_text("Original",AttributeSearch::footnote).whole_entry,"Exact attribute paths and whole-entry policy");
 try{attribute_search_text(std::string("bad\0query",9),AttributeSearch::lemma);throw std::logic_error("Expected NUL rejection");}catch(const std::invalid_argument &){}
 try{attribute_search_text("Original",static_cast<AttributeSearch>(99));throw std::logic_error("Expected kind rejection");}catch(const std::invalid_argument &){}
 std::cout<<"Owned C++11 attribute query and actual legacy dialog flags match frozen behavior passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
