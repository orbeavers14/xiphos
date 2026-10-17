// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/search_policy.hpp"
#include <glib.h>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstdint>
#include "legacy_sidebar_search_text.inc"
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 for(const auto &text:std::vector<std::string>{"","Original","two words"," leading","trailing ","a  b","  ","a\tb","a\nb","a +b","café λόγος","name:value","quoted \"phrase\""})
  for(bool indexed:{false,true})check(sidebar_search_text(text,indexed)==frozen_sidebar_search_text(text,indexed),"Shared text exactly matches frozen sidebar spaces, tabs, signs and Unicode bytes");
 std::uint32_t random=7319;const std::string alphabet=" a+b\t:\"café";
 for(unsigned n=0;n<1000;++n){std::string text;for(unsigned i=0;i<n%67;++i){random=random*1664525u+1013904223u;text+=alphabet[random%alphabet.size()];}check(sidebar_search_text(text,true)==frozen_sidebar_search_text(text,true),"Deterministic byte/spacing corpus matches frozen AND text transform");}
 check(sidebar_search_text("Original",true)=="+Original "&&sidebar_search_text("a  b ",true)=="+a + +b  ","Indexed single-term workaround and repeated/trailing spaces stay exact");
 try{(void)sidebar_search_text(std::string("bad\0query",9),true);throw std::runtime_error("Expected NUL rejection");}catch(const std::invalid_argument &){}
 std::cout<<"Shared indexed sidebar text preparation matches frozen legacy bytes passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
