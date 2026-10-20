// SPDX-License-Identifier: GPL-2.0-or-later
#include "search_results.hpp"
#include "search_scope.hpp"
#include <listkey.h>
#include <versekey.h>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
using namespace xiphos::core::detail;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class Error,class F>void rejects(F call){try{call();}catch(const Error &){return;}throw std::runtime_error("Expected result-copy error");}
struct RangeKey:sword::SWKey {
 std::string range;
 RangeKey(const char *key,std::string value):SWKey(key),range(std::move(value)){}
 sword::SWKey *clone() const override{return new RangeKey(*this);}
 const char *getRangeText() const override{return range.c_str();}
};
int main(){try{
 SearchReferences held;
 {
  sword::ListKey keys;const std::string long_key(300,'L');std::string unicode;for(int i=0;i<100;++i)unicode+="α";
  keys.add(sword::SWKey(long_key.c_str()));keys.add(sword::SWKey(unicode.c_str()));keys.add(sword::SWKey("tail"));
  held=copy_search_references(keys,{});
  check(held.engine_count==3&&held.references==std::vector<std::string>{long_key,unicode,"tail"}&&!held.limited,"Long and Unicode keys copy completely beyond the legacy 128-byte buffer");
  auto exact=copy_search_references(keys,{2,500});check(exact.engine_count==3&&exact.references.size()==2&&exact.limited,"Exact prefix byte budget still reports omitted later keys");
  auto bytes=copy_search_references(keys,{3,499});check(bytes.references==std::vector<std::string>{long_key}&&bytes.limited,"UTF-8 reference that exceeds budget is omitted whole, never split");
  auto none=copy_search_references(keys,{3,299});check(none.references.empty()&&none.limited&&none.engine_count==3,"Oversized first key retains total SDK count with no partial key");
  for(const auto limits:{SearchLimits{0,500},SearchLimits{1,0},SearchLimits{maximum_search_results+1,500},SearchLimits{1,maximum_search_reference_bytes+1}})
   rejects<std::invalid_argument>([&]{copy_search_references(keys,limits);});
 }
 check(held.references[0].size()==300&&held.references[1].size()==200,"Copied key bytes outlive the SDK list");
 sword::VerseKey parser;auto interval=parser.parseVerseList("John1:1-John1:3","",true);
 auto expanded=copy_search_references(interval,{});check(expanded.engine_count==1&&expanded.references==std::vector<std::string>{"John 1:1","John 1:2","John 1:3"}&&!expanded.limited,"One SDK interval element can expose multiple ordered verse references");
 auto prefix=copy_search_references(interval,{2,1024});check(prefix.engine_count==1&&prefix.references.size()==2&&prefix.limited,"Prefix limit bounds expanded keys independently of SDK element count");
 auto exact=copy_search_references(interval,{3,24});check(exact.references.size()==3&&!exact.limited,"Exact result and byte limits do not imply truncation without omitted data");
 sword::ListKey empty;auto absent=copy_search_references(empty,{});check(!absent.engine_count&&absent.references.empty()&&!absent.limited,"Empty SDK list is a complete empty result");
 sword::ListKey invalid;invalid.add(sword::SWKey(""));rejects<std::runtime_error>([&]{copy_search_references(invalid,{});});
 for(const auto &malformed:std::vector<std::string>{std::string("\xff",1),std::string("\xc0\xaf",2),std::string("\xed\xa0\x80",3),std::string("\xe2\x82",2),std::string("\xf4\x90\x80\x80",4)}){
  sword::ListKey bad;bad.add(sword::SWKey(malformed.c_str()));
  rejects<std::runtime_error>([&]{copy_search_references(bad,{});});
  rejects<std::runtime_error>([&]{SearchScope retained(bad);});
  sword::ListKey tail;tail.add(sword::SWKey("Original valid"));tail.add(sword::SWKey(malformed.c_str()));
  const auto prefix=copy_search_references(tail,{1,1024});
  check(prefix.references==std::vector<std::string>{"Original valid"}&&prefix.limited,"Omitted malformed tail is not copied as replacement characters");
  rejects<std::runtime_error>([&]{SearchScope retained(tail);});
  sword::ListKey range;range.add(RangeKey("Original valid",malformed));
  rejects<std::runtime_error>([&]{SearchScope retained(range);});
 }
 sword::ListKey unicode_range;unicode_range.add(RangeKey("αρχή", "αρχή–τέλος"));SearchScope valid_range(unicode_range);
 check(std::string(valid_range.keys().getElement(0)->getRangeText())=="αρχή–τέλος","Valid owned Unicode range metadata remains exact");
 std::cout<<"Owned long/UTF-8 search references, interval counts and exact prefix budgets passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
