// SPDX-License-Identifier: GPL-2.0-or-later
#include "passage_uri_test_host.hpp"
int main(){try {
 // Exhaust short strings against the literal retained split, including empty
 // anchors, repeated markers, UTF-8 bytes and hash-after-bang precedence.
 std::size_t anchor_cases=0;
 const std::string alphabet="ab/#!";
 std::function<void(std::string,int)> anchors = [&](std::string input,int remaining) {
  std::vector<char> expected(input.begin(),input.end()), actual_key=expected;
  expected.push_back(0);actual_key.push_back(0);
  char *old=std::strchr(expected.data(),'#');
  if(!old)old=std::strchr(expected.data(),'!');
  if(old)*old++='\0';
  char *now=xiphos::core::passage_uri_anchor(actual_key.data());
  check(bool(old)==bool(now),"Anchor presence differs");
  check(expected==actual_key,"Anchor split changes retained bytes");
  if(old)check(old-expected.data()==now-actual_key.data() && std::strcmp(old,now)==0,"Anchor ownership/offset differs");
  ++anchor_cases;
  if(remaining)for(char c:alphabet)anchors(input+c,remaining-1);
 };
 anchors("",6);anchors("café/文字",2);
 std::size_t comparisons=0;
 for(const char *main:{"Text","Commentary","Missing"})for(const char *module:{"","Text","T","文字 café","Commentary","Percom","Dictionary","D","Book","Prayer","Missing","Gone"})
 for(const char *reference:{"John3:16","John3:16-18","John3:16;17","John3:16,17","John3:16#tag","John3:16!note","John3:16!earlier#later","café"})
 for(bool clicked:{false,true})for(bool commentary:{false,true})for(int notebook:{0,1})for(bool has_tag:{false,true})for(bool renders:{false,true})for(bool navigates:{false,true}) {
  tag=has_tag;render=renders;nav=navigates;const std::string uri=std::string("sword://")+module+"/"+reference;
  const auto expected=run(frozen::sword_uri,uri,clicked,main,commentary,notebook,false);
  check(run(actual::sword_uri,uri,clicked,main,commentary,notebook,false)==expected,"Complete URI callback trace differs from frozen behavior");++comparisons;
 }
 for(const auto *uri:{"sword://Text/John3:16","sword://Missing/John3:16","sword://Commentary/John3:16","sword://Text"}) {
  tag=false;render=true;nav=true;
  const auto expected=run(frozen::sword_uri,uri,true,"Text",true,1,true);
  check(run(actual::sword_uri,uri,true,"Text",true,1,true)==expected,"URI fallback/reentry trace differs");++comparisons;
 }
 // Keyed URI hover passes the entire decoded path to rendering. Click alone
 // strips anchors; dictionary list punctuation never selects the Bible sidebar.
 types["Daily"]=DICTIONARY_TYPE;types["LongDictionaryNameX"]=DICTIONARY_TYPE;types["LongDictionaryName20"]=DICTIONARY_TYPE;
 aliases["DailyAlias"]="Daily";aliases["L20"]="LongDictionaryName20";
 for(const auto *module:{"D","DailyAlias","LongDictionaryNameX","LongDictionaryName20","L20",""})
 for(const auto *key:{"alpha-beta;gamma,delta/part#tag!note?query","café 文字","literal%20key","plus+key"})for(bool clicked:{false,true}) {
  const std::string uri=std::string("sword://")+module+"/"+key;
  const auto expected=run(frozen::sword_uri,uri,clicked,"Text",true,1,false);
  check(run(actual::sword_uri,uri,clicked,"Text",true,1,false)==expected,"Keyed dictionary URI full callback trace differs");++comparisons;
 }
 const std::string raw_key="alpha-beta;gamma,delta/part#tag!note?query";
 const auto dictionary_hover=run(actual::sword_uri,"sword://D/"+raw_key,false,"Text",true,1,false);
 check(!dictionary_hover.history && std::find(dictionary_hover.calls.begin(),dictionary_hover.calls.end(),"render:Dictionary:"+raw_key)!=dictionary_hover.calls.end(),"Dictionary hover renders entire decoded raw key without history");
 const auto long_hover=run(actual::sword_uri,"sword://LongDictionaryName20/alpha",false,"Text",true,1,false);
 check(std::none_of(long_hover.calls.begin(),long_hover.calls.end(),[](const std::string &call){return call.compare(0,7,"render:")==0;}),"Retained hover misses separators beyond its 20-byte scan");
 const auto missing=run(actual::sword_uri,"sword://Missing/John3:16",true,"Text",false,0,false);
 check(missing.history&&missing.buffers==0,"Unknown URI routes through main navigation and records history");
 const auto range=run(actual::sword_uri,"sword://T/John3:16-18",true,"Text",false,0,false);
 check(!range.history&&range.buffers==1,"Bible range routes sidebar before final history flag and retains observed legacy split leak");
 std::cout<<"Complete retained URI/preview dispatch: "<<comparisons<<" frozen/actual callback and state traces agree; "<<anchor_cases<<" anchor splits agree\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
