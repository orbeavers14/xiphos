// SPDX-License-Identifier: GPL-2.0-or-later
#include <glib.h>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
using Store=std::map<std::pair<std::string,std::string>,std::string>;
static Store store;
static std::vector<std::string> calls;
static gchar *xml_get_value(const gchar *section,const gchar *key){calls.push_back(std::string("get:")+section+":"+key);const auto found=store.find({section,key});return found==store.end()?nullptr:g_strdup(found->second.c_str());}
static void xml_set_or_create_value(const gchar *section,const gchar *key,const gchar *value){calls.push_back(std::string("set:")+section+":"+key+":"+value);store[{section,key}]=value;}
namespace frozen {
#include "legacy_parallel_sets.inc"
}
namespace actual {
#include "legacy_parallel_sets_actual.inc"
}
using Get=char **(*)(const char *);using Save=void (*)(const char *,char **);
struct Trace {Store store;std::vector<std::string> calls,modules;bool present;bool operator==(const Trace &other)const{return store==other.store&&calls==other.calls&&modules==other.modules&&present==other.present;}};
static Trace run(Get get,Save save,const char *name,const std::vector<std::string> &modules,bool pointer,bool write,const std::string &existing){
 store={{{"modules","unrelated"},"Original preserved setting"}};
 if(name)store[{"modules",std::string("parallel_set_")+name}]=existing;
 calls.clear();std::vector<gchar *> input;for(const auto &module:modules)input.push_back(const_cast<char *>(module.c_str()));input.push_back(nullptr);
 if(write)save(name,pointer?input.data():nullptr);
 auto *result=get(name);Trace trace{store,calls,{},result!=nullptr};
 if(result)for(std::size_t i=0;result[i];++i)trace.modules.push_back(result[i]);g_strfreev(result);return trace;
}
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 std::size_t comparisons=0;
 const std::vector<std::vector<std::string>> sets={{},{"TextA"},{"A","B","A"},{"A","","B"},{"Missing","A"},{"A,B","文字 café"},{"A","A","A","A","A","A","A","A","A","A"}};
 for(const char *name:std::vector<const char *>{nullptr,"","Study Set","Study_Set","文字 café","a,b","<x>&\""})
 for(const auto &modules:sets)for(bool pointer:{false,true})for(bool write:{false,true})for(const auto *existing:{"","A,B","A,,B,","Missing,文字"}){
  check(run(actual::get_parallel_set,actual::save_parallel_set,name,modules,pointer,write,existing)==run(frozen::get_parallel_set,frozen::save_parallel_set,name,modules,pointer,write,existing),"Complete named parallel set trace differs");++comparisons;
 }
 for(const auto &name:std::vector<std::string>{"","Study Set","Study_Set","Study  Set","_"," a_b ","文字 café","a,b","<x>&\""}){
  auto *old=frozen::name_to_key(name.c_str()),*now=actual::name_to_key(name.c_str());check(std::string(old)==now,"Complete name encoding differs");
  auto *old_display=frozen::key_to_name(old),*new_display=actual::key_to_name(now);check(std::string(old_display)==new_display,"Complete name decoding differs");g_free(old);g_free(now);g_free(old_display);g_free(new_display);++comparisons;
 }
 auto empty=run(actual::get_parallel_set,actual::save_parallel_set,"Set",{},true,true,"A,B");
 check(!empty.present&&empty.store.at({"modules","parallel_set_Set"}).empty(),"Saving non-null empty set writes empty value that reload treats as missing");
 auto ordered=run(actual::get_parallel_set,actual::save_parallel_set,"Set",{"A","","A"},true,true,"");check(ordered.modules==std::vector<std::string>({"A","","A"}),"Storage retains raw duplicate/empty tokens and order");
 auto comma=run(actual::get_parallel_set,actual::save_parallel_set,"Set",{"A,B"},true,true,"");check(comma.modules==std::vector<std::string>({"A","B"}),"Comma serialization is unescaped and not lossless for comma-bearing tokens");
 auto *space=actual::name_to_key("Study Set"),*underscore=actual::name_to_key("Study_Set"),*display=actual::key_to_name(underscore);
 check(std::string(space)==underscore&&std::string(display)=="Study Set","Space/underscore names collide and literal underscores are lost on display");g_free(space);g_free(underscore);g_free(display);
 std::cout<<"Complete named parallel storage/name helpers: "<<comparisons<<" traces/comparisons, raw order/empty/comma behavior and naming collisions agree\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
