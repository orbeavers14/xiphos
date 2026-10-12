// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/profile.hpp"
#include <libxml/parser.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>
#include <clocale>
using namespace xiphos::core;
#define _(v) (v)
static xmlDocPtr xml_settings_doc;
#include "legacy_xml_profile_actual.inc"
namespace {
unsigned checks = 0;
void check(bool condition, const char *message) { ++checks; if (!condition) throw std::runtime_error(message); }
std::optional<std::string> original(const char *key, const char *fallback, bool reject_empty) {
  auto *text = xml_get_value("lexicons",key);
  std::optional<std::string> value;
  if(text) { value = std::string(text); xmlFree(text); }
  if (!value || (reject_empty && value->empty())) return std::string(fallback);
  return value;
}
void compare(const std::string &xml) {
  xml_settings_doc=xmlReadMemory(xml.data(),static_cast<int>(xml.size()),nullptr,nullptr,XML_PARSE_NONET);
  check(xml_settings_doc!=nullptr,"actual legacy fixture parses");
  const auto profile=parse_study_profile(xml);
  check(profile.lexicons.greek==original("greek","StrongsGreek",false),"Greek matches actual getter/default");
  check(profile.lexicons.hebrew==original("hebrew","StrongsHebrew",false),"Hebrew matches actual getter/default");
  check(profile.lexicons.greek_morph_nt==original("greek_morph_nt","Robinson",true),"NT morphology matches");
  check(profile.lexicons.greek_morph_ot==original("greek_morph_ot","Packard",true),"OT morphology matches");
  check(profile.lexicons.hebrew_morph==original("hebrew_morph","OSHM",true),"Hebrew morphology matches");
  auto *locale=xml_get_value("locale","special");
  const auto expected_locale=locale ? std::optional<std::string>(locale) : std::nullopt;
  check(profile.saved_locale==expected_locale,"locale matches actual getter without application");
  xmlFree(locale); xmlFreeDoc(xml_settings_doc);
}
void rejected(const std::string &source) {
  bool failed=false;
  try { (void)parse_study_profile(source); } catch(const std::invalid_argument &) { failed=true; }
  check(failed,"unsupported/invalid input rejected");
}
unsigned external_loads = 0;
xmlParserInputPtr no_external_load(const char *, const char *, xmlParserCtxtPtr) { ++external_loads; return nullptr; }
std::string bytes(const std::filesystem::path &path) {
  std::ifstream input(path,std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(input)),{});
}
}
int main(int argc,char **argv) {
  try {
    check(argc==2,"disposable fixture path required");
    const auto root=std::filesystem::u8path(argv[1]);
    std::filesystem::create_directories(root);
    const std::string previous_locale=std::setlocale(LC_ALL,nullptr);
    compare("<Xiphos/>");
    const std::string keys[]={"greek","hebrew","greek_morph_nt","greek_morph_ot","hebrew_morph"};
    const std::string values[]={"","UnavailableModule","CAFÉ","Greek &amp; Hebrew","<![CDATA[<Lexicon>]]>"," ","<part>child</part>","&#x130;","a<part>child</part>b"};
    for(const auto &key:keys) for(const auto &value:values) {
      compare("<Xiphos><!--keep--><other attr='preserve'>unrelated</other><lexicons><"+key+">"+value+"</"+key+"><"+key+">second</"+key+"></lexicons><lexicons><"+key+">third</"+key+"></lexicons><locale><special>tr_TR.UTF-8</special></locale></Xiphos>");
    }
    compare("<Xiphos><lexicons/><lexicons><greek>Later</greek></lexicons><locale><special/></locale></Xiphos>");
    std::string utf16="\xff\xfe";
    for(char16_t c:std::u16string(u"<?xml version='1.0' encoding='UTF-16'?><Xiphos><lexicons><greek>CAFÉ</greek></lexicons></Xiphos>")) { utf16.push_back(static_cast<char>(c&255)); utf16.push_back(static_cast<char>(c>>8)); }
    compare(utf16);
    rejected(utf16+std::string(2,'\0'));
    rejected(utf16.substr(0,utf16.size()-1));
    const auto owned=parse_study_profile("<Xiphos><lexicons><greek>Copied</greek></lexicons></Xiphos>");
    (void)parse_study_profile("<Xiphos><lexicons><greek>Other</greek></lexicons></Xiphos>");
    check(owned.lexicons.greek=="Copied","owned values outlive parser and later reads");
    for(const auto &bad:{"", "<Xiphos>", "<Other/>", "<Xiphos><lexicons><greek>&missing;</greek></lexicons></Xiphos>", "<Xiphos/>trailing"}) rejected(bad);
    rejected("<Xiphos><lexicons><greek>"+std::string(1,static_cast<char>(0xff))+"</greek></lexicons></Xiphos>");
    rejected(std::string("<Xiphos/>\0trailing",18));
    rejected(std::string(16*1024*1024+1,'x'));
    const auto loader=xmlGetExternalEntityLoader();
    xmlSetExternalEntityLoader(no_external_load);
    const int previous_substitution=xmlSubstituteEntitiesDefault(1);
    rejected("<!DOCTYPE Xiphos SYSTEM 'file:///unavailable-test-only.dtd'><Xiphos/>");
    rejected("<!DOCTYPE Xiphos [<!ENTITY x SYSTEM 'file:///unavailable-test-only.txt'>]><Xiphos><lexicons><greek>&x;</greek></lexicons></Xiphos>");
    rejected("<!DOCTYPE Xiphos [<!ENTITY x 'internal'>]><Xiphos/>");
    xmlSubstituteEntitiesDefault(previous_substitution);
    xmlSetExternalEntityLoader(loader);
    check(external_loads==0,"no external loader invoked even with global substitution enabled");
    const auto file=root/std::filesystem::u8path("profile-café.xml");
    const std::string source="<Xiphos><!--retain--><lexicons><greek>Unavailable</greek></lexicons><unknown>keep</unknown></Xiphos>";
    { std::ofstream output(file,std::ios::binary); output<<source; }
    const auto permissions=std::filesystem::status(file).permissions();
    check(read_study_profile(file.u8string()).lexicons.greek=="Unavailable","explicit UTF-8 file lookup");
    check(bytes(file)==source && std::filesystem::status(file).permissions()==permissions,"read retains exact file bytes and permissions");
    for(const auto &path:{std::string(),(root/"missing.xml").u8string(),root.u8string(),std::string("invalid\0path",12)}) {
      bool failed=false; try { (void)read_study_profile(path); } catch(const std::exception &) { failed=true; }
      check(failed,"invalid path rejected");
    }
    std::future<StudyProfile> jobs[8];
    for(unsigned i=0;i<8;++i) jobs[i]=std::async(std::launch::async,[i] { return parse_study_profile("<Xiphos><lexicons><greek>Window"+std::to_string(i)+"</greek></lexicons></Xiphos>"); });
    for(unsigned i=0;i<8;++i) check(jobs[i].get().lexicons.greek=="Window"+std::to_string(i),"parser contexts and copied results isolated");
    check(previous_locale==std::setlocale(LC_ALL,nullptr),"saved locale never changes process locale");
    std::cout<<"Shared profile reader: "<<checks<<" checks passed\n";
  } catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
