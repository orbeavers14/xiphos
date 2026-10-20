// SPDX-License-Identifier: GPL-2.0-or-later
#include "sdk_text.hpp"
#include <iostream>
#include <vector>
using namespace xiphos::core::detail;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
sword::SWBuf buffer(const std::string &text){sword::SWBuf value;value.setSize(text.size());for(std::size_t i=0;i<text.size();++i)value[static_cast<unsigned long>(i)]=text[i];return value;}
template<class E,class F>void rejects(F call){try{call();}catch(const E &){return;}throw std::runtime_error("Expected SDK text rejection");}
int main(){try{
 for(const std::string text:{std::string{},std::string("Original café λόγος"),std::string(65536,'x')}){
  check(copy_sdk_text(buffer(text),text.size())==text,"Sized SDK text preserves every valid byte");
  check(copy_sdk_text(buffer(text+std::string(1,'\0')),text.size())==text,"One counted terminal NUL is SDK storage, not copied query/scripture content");
  if(!text.empty())rejects<std::length_error>([&]{copy_sdk_text(buffer(text+std::string(1,'\0')),text.size()-1);});
 }
 for(const auto text:std::vector<std::string>{std::string("bad\0tail",8),std::string("two\0\0",5),std::string("\xff",1),std::string("\xe2\x82",2)}){
  rejects<std::runtime_error>([&]{copy_sdk_text(buffer(text),65536);});
  rejects<std::runtime_error>([&]{copy_sdk_text(buffer(text+std::string(1,'\0')),65536);});
 }
 std::cout<<"Sized SDK text, single terminal convention, exact bytes and malformed content rejection passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
