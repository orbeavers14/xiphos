// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/search_policy.hpp"
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
struct Widgets {bool rb_exact_phrase,cb_include_strongs,cb_include_morphs,cb_include_footnotes;}search1;
#define GTK_TOGGLE_BUTTON(value) (value)
bool gtk_toggle_button_get_active(bool value){return value;}
std::vector<std::pair<std::string,bool>> calls;
void set_search_global_option(const char *name,bool value){calls.emplace_back(name,value);}
#define check_search_global_options frozen_global_options
#include "legacy_search_text_options.inc"
#undef check_search_global_options
#include "legacy_search_text_options_actual.inc"
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 for(int bits=0;bits<8;++bits)for(bool phrase:{false,true}){
  search1={phrase,bool(bits&1),bool(bits&2),bool(bits&4)};
  calls.clear();frozen_global_options();const auto expected=calls;
  calls.clear();check_search_global_options();check(calls==expected,"Retained annotation option names/order/checkbox values match frozen complete function");
  check(calls.size()==(phrase?0:3),"Exact phrase skips option mutation while other modes explicitly set all three");
 }
 // Legacy exact phrase preserves the last nonphrase state even if the visible
 // checkboxes have changed. Native sessions must materialize this state privately.
 search1={false,true,false,true};calls.clear();check_search_global_options();std::map<std::string,bool> state(calls.begin(),calls.end());
 search1={true,false,true,false};calls.clear();check_search_global_options();
 check(calls.empty()&&state["Strong's Numbers"]&&!state["Morphological Tags"]&&state["Footnotes"],"Phrase checkbox values do not reset prior search annotation state");
 std::cout<<"Shared C++11 search text settings preserve legacy names/order and sticky phrase state passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
