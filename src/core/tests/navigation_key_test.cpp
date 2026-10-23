// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/navigation_key.hpp"
#include <rawtext.h>
#include <rawcom.h>
#include <rawfiles.h>
#include <versekey.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
using namespace sword;
enum { TEXT_TYPE=1, COMMENTARY_TYPE=2, PERCOM_TYPE=3 };
static const char *sword_locale=nullptr;
static std::vector<std::string> calls;
struct Harness {
 SWModule *module=nullptr;int kind=0;
 SWModule *get_SWModule(const char *name){calls.push_back("module:"+std::string(name?name:"<null>"));return name&&!std::strcmp(name,"Available")?module:nullptr;}
 int module_type(const char *name){calls.push_back("type:"+std::string(name));return kind;}
};
namespace frozen {struct BackEnd:Harness{char *get_valid_key(const char *,const char *);};
#include "legacy_navigation_key.inc"
}
namespace actual {struct BackEnd:Harness{char *get_valid_key(const char *,const char *);};
#include "legacy_navigation_key_actual.inc"
}
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
struct Outcome {
 std::string returned,position;bool automatic,intros;int error,testament,book,chapter,verse;std::vector<std::string> calls;
 bool operator==(const Outcome &other)const{return returned==other.returned&&position==other.position&&automatic==other.automatic&&intros==other.intros&&error==other.error&&testament==other.testament&&book==other.book&&chapter==other.chapter&&verse==other.verse&&calls==other.calls;}
};
template<class Backend>Outcome run(Backend &backend,VerseKey &key,const char *canon,const char *context,const char *locale,const char *reference,bool automatic,bool intros,int kind){
 key.setVersificationSystem(canon);key.setAutoNormalize(true);key.setIntros(true);key.setText(context);key.setAutoNormalize(automatic);key.setIntros(intros);key.popError();backend.kind=kind;sword_locale=locale;calls.clear();
 std::unique_ptr<char,decltype(&std::free)> result(backend.get_valid_key("Available",reference),&std::free);
 check(bool(result),"Owned retained label returned");
 return {result.get(),key.getText(),key.isAutoNormalize(),key.isIntros(),key.getError(),key.getTestament(),key.getBook(),key.getChapter(),key.getVerse(),calls};
}
int main(int argc,char **argv){try {
 check(argc==2,"Disposable navigation-key root required");const std::string root=argv[1];std::size_t comparisons=0;
 for(int driver=0;driver<3;++driver){
  const auto data=root+(driver==0?"/bible":driver==1?"/commentary":"/personal");
  check((driver==0?RawText::createModule(data.c_str()):driver==1?RawCom::createModule(data.c_str()):RawFiles::createModule(data.c_str()))==0,"Create empty original navigation module");
  std::unique_ptr<SWModule> module;
  if(driver==0)module.reset(new RawText(data.c_str()));else if(driver==1)module.reset(new RawCom(data.c_str()));else module.reset(new RawFiles(data.c_str()));
  VerseKey *key=dynamic_cast<VerseKey *>(module->getKey());check(key!=nullptr,"Retained normalization supports verse-key fixture");
  frozen::BackEnd baseline;actual::BackEnd current;baseline.module=current.module=module.get();
  for(const char *canon:{"KJV","KJVA"})for(const char *context:{"John3:23","Genesis1:1"})
  for(const char *locale:{static_cast<const char *>(nullptr),"","en","EN","fr"})
  for(const char *reference:{"John3:16","John3:16-18","John3:16;17","16-18;20","John3:36-4:2","Genesis0:0","Genesis1:0","John3:999","Rev999:999","Tob1:3","not-a-reference","","café","John3:16!note"})
  for(bool automatic:{false,true})for(bool intros:{false,true})for(int kind:std::vector<int>{TEXT_TYPE,COMMENTARY_TYPE,PERCOM_TYPE,-1}){
   const auto expected=run(baseline,*key,canon,context,locale,reference,automatic,intros,kind);
   const auto actual=run(current,*key,canon,context,locale,reference,automatic,intros,kind);
   check(actual==expected,"Full retained get_valid_key label/key/flags/error/callback state differs from frozen implementation");
   check(actual.automatic==((kind==TEXT_TYPE||kind==COMMENTARY_TYPE)?true:automatic),"Only Bible/regular commentary force normalization; other kinds retain flag");++comparisons;
  }
  for(const char *name:{static_cast<const char *>(nullptr),"Missing"}){
   calls.clear();std::unique_ptr<char,decltype(&std::free)> a(baseline.get_valid_key(name,nullptr),&std::free);const auto trace=calls;calls.clear();std::unique_ptr<char,decltype(&std::free)> b(current.get_valid_key(name,nullptr),&std::free);
   check(std::string(a.get())=="Gen.1.1"&&std::string(b.get())==a.get()&&calls==trace,"Missing module returns unchanged fallback without touching key/input");++comparisons;
  }
  key->setAutoNormalize(false);key->setText("John3:16");
  const auto owned=xiphos::core::navigation_key_text(*key,"John3:17",true,"en");
  const auto expected=std::string(key->getShortText());key->setText("Genesis1:1");check(owned==expected,"Shared returned text owns bytes after key mutation");
 }
 std::cout<<"Complete navigation normalization: "<<comparisons<<" frozen/current traces across RawText/RawCom/RawFiles, KJV/KJVA and flags agree\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
