// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/profile.hpp"
#include "preference_file.hpp"
#include <libxml/parser.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
namespace {
unsigned checks=0;
void check(bool condition,const char *message) { ++checks; if(!condition) throw std::runtime_error(message); }
std::string bytes(const std::filesystem::path &path) { return detail::read_preference_file(path); }
void write(const std::filesystem::path &path,const std::string &value) { std::ofstream output(path,std::ios::binary); output.exceptions(std::ios::badbit|std::ios::failbit); output<<value; output.close(); }
std::string node(xmlDocPtr document,const char *path) {
  // Dump exact unaffected fragments for this fixed fixture, independent of the
  // production getter. No XPath/alternate schema implementation is introduced.
  auto *root=xmlDocGetRootElement(document);
  for(auto *item=root->children;item;item=item->next) if(!xmlStrcmp(item->name,BAD_CAST path)) {
    auto *buffer=xmlBufferCreate(); if(!buffer) throw std::bad_alloc();
    xmlNodeDump(buffer,document,item,0,0);
    std::string result(reinterpret_cast<const char *>(xmlBufferContent(buffer)));
    xmlBufferFree(buffer); return result;
  }
  return {};
}
}
int main(int argc,char **argv) {
 try {
  check(argc==2,"disposable fixture path required");
  const auto root=std::filesystem::u8path(argv[1]); std::filesystem::create_directories(root);
  const auto file=root/std::filesystem::u8path("profile-café.xml");
  const std::string original="<?xml version='1.0'?><Xiphos version='saved'><!--keep--><other attr='saved'><child>CAFÉ &amp; peace</child></other><locale><special>tr_TR.UTF-8</special></locale><lexicons attr='keep'><greek meta='retain'>old</greek><greek>duplicate</greek><defaultdictionary>Saved</defaultdictionary><usedefaultdict>1</usedefaultdict><custom>retain</custom></lexicons><lexicons><hebrew>later-section</hebrew><hebrew>later-duplicate</hebrew></lexicons></Xiphos>";
  LexiconPreferences preferences;
  preferences.greek="CAFÉ & <Greek>"; preferences.hebrew="UnavailableHebrew";
  preferences.greek_morph_nt="NT & morphology"; preferences.greek_morph_ot="OT<parse>"; preferences.hebrew_morph="İ Morph";
  write(file,original);
  std::filesystem::permissions(file,std::filesystem::perms::owner_read|std::filesystem::perms::owner_write);
  const auto permissions=std::filesystem::status(file).permissions();
  save_lexicon_profile(file.u8string(),preferences);
  const auto saved=bytes(file);
  const auto decoded=read_study_profile(file.u8string());
  check(decoded.lexicons.greek==preferences.greek && decoded.lexicons.hebrew==preferences.hebrew && decoded.lexicons.greek_morph_nt==preferences.greek_morph_nt && decoded.lexicons.greek_morph_ot==preferences.greek_morph_ot && decoded.lexicons.hebrew_morph==preferences.hebrew_morph,"all five literal values round trip");
  check(decoded.saved_locale=="tr_TR.UTF-8","saved locale retained without modification");
  check(std::filesystem::status(file).permissions()==permissions,"existing permissions retained");
  auto *before=xmlReadMemory(original.data(),static_cast<int>(original.size()),nullptr,nullptr,XML_PARSE_NONET);
  auto *after=xmlReadMemory(saved.data(),static_cast<int>(saved.size()),nullptr,nullptr,XML_PARSE_NONET);
  check(before&&after,"independent documents parse");
  for(const auto *key:{"other","locale","comment"}) check(node(before,key)==node(after,key),"unrelated subtree/comment preserved");
  xmlFreeDoc(before); xmlFreeDoc(after);
  check(saved.find("<greek meta=\"retain\">CAFÉ &amp; &lt;Greek&gt;</greek>")!=std::string::npos,"edited key attributes retained and text escaped");
  check(saved.find("<greek>duplicate</greek>")!=std::string::npos && saved.find("<hebrew>later-duplicate</hebrew>")!=std::string::npos,"shadowed duplicates retained");
  check(saved.find("<defaultdictionary>Saved</defaultdictionary>")!=std::string::npos && saved.find("<usedefaultdict>1</usedefaultdict>")!=std::string::npos && saved.find("<custom>retain</custom>")!=std::string::npos,"unknown and adjacent lexicon settings retained");
  const auto fresh=root/"new-profile.xml"; std::filesystem::remove(fresh);
  save_lexicon_profile(fresh.u8string(),preferences);
  check(read_study_profile(fresh.u8string()).lexicons.greek==preferences.greek,"new explicit profile creates root/section/keys");
#ifndef _WIN32
  check(std::filesystem::status(fresh).permissions()==(std::filesystem::perms::owner_read|std::filesystem::perms::owner_write),"new profile private permissions");
#endif
  std::string utf16="\xff\xfe";
  for(char16_t c:std::u16string(u"<?xml version='1.0' encoding='UTF-16'?><Xiphos><other>CAFÉ</other></Xiphos>")) { utf16.push_back(static_cast<char>(c&255)); utf16.push_back(static_cast<char>(c>>8)); }
  const auto utf8=serialize_lexicon_profile(utf16,preferences);
  check(parse_study_profile(utf8).lexicons.greek==preferences.greek && utf8.find("<other>CAFÉ</other>")!=std::string::npos,"UTF-16 source saves UTF-8 without losing unrelated Unicode");
  check(utf8.find("encoding=\"UTF-8\"")!=std::string::npos,"serialized encoding declaration matches output bytes");
  auto invalid=preferences;
  const std::optional<std::string> bad_values[]={std::nullopt,std::string(),std::string("bad\0name",8),std::string(1,static_cast<char>(0xff)),std::string(1,'\x01'),std::string(16*1024*1024+1,'x')};
  for(const auto &bad:bad_values) {
    invalid.greek=bad;
    bool failed=false; try { save_lexicon_profile(file.u8string(),invalid); } catch(const std::exception &) { failed=true; }
    check(failed && bytes(file)==saved,"invalid/unrepresentable choice leaves original bytes intact");
  }
  for(const auto &bad:{std::string(),std::string("<Xiphos>"),std::string("<Other/>"),std::string("<!DOCTYPE Xiphos><Xiphos/>")}) {
    write(fresh,bad); bool failed=false;
    try { save_lexicon_profile(fresh.u8string(),preferences); } catch(const std::invalid_argument &) { failed=true; }
    check(failed&&bytes(fresh)==bad,"invalid existing profile not replaced, including empty file");
  }
  for(const auto &path:{std::string(),root.u8string(),(root/"missing-parent/profile.xml").u8string(),std::string("bad\0path",8)}) {
    bool failed=false; try { save_lexicon_profile(path,preferences); } catch(const std::exception &) { failed=true; }
    check(failed && bytes(file)==saved,"invalid destination does not overwrite profile");
  }
  check(!std::filesystem::exists(root/"missing-parent"),"missing parent not silently created");
#ifndef _WIN32
  const auto link=root/"linked-profile.xml"; std::filesystem::remove(link);
  std::filesystem::create_symlink(file.filename(),link); save_lexicon_profile(link.u8string(),preferences);
  check(std::filesystem::is_symlink(link) && read_study_profile(file.u8string()).lexicons.greek==preferences.greek,"existing symlink alias/target retained");
  std::filesystem::permissions(file,std::filesystem::perms::owner_read);
  const auto readonly=bytes(file); bool failed=false;
  try { save_lexicon_profile(file.u8string(),preferences); } catch(const std::runtime_error &) { failed=true; }
  check(failed&&bytes(file)==readonly,"read-only destination retained"); std::filesystem::permissions(file,permissions);
  const auto broken=root/"broken-profile.xml"; std::filesystem::remove(broken); std::filesystem::create_symlink("no-target.xml",broken);
  failed=false; try { save_lexicon_profile(broken.u8string(),preferences); } catch(const std::invalid_argument &) { failed=true; }
  check(failed&&std::filesystem::is_symlink(broken)&&!std::filesystem::exists(root/"no-target.xml"),"dangling symlink not replaced");
#endif
  const auto changed=root/"changed-during-save.xml"; write(changed,"original"); bool changed_rejected=false;
  try { detail::replace_preference_file(changed.u8string(),[&](const std::filesystem::path &temporary,const std::string &,bool) { write(temporary,"prepared"); write(changed,"external-update"); }); } catch(const std::runtime_error &) { changed_rejected=true; }
  check(changed_rejected&&bytes(changed)=="external-update","observed external update not overwritten");
  for(const auto &entry:std::filesystem::directory_iterator(root)) check(entry.path().filename().u8string().find(".xiphos-preferences-")!=0,"temporary directories removed after success/failure");
  std::cout<<"Profile save: "<<checks<<" preservation/round-trip/failure checks passed\n";
 } catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
