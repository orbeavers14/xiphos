// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/book_selection.hpp"
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F call){try{call();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected book selection rejection");}
namespace actual_legacy {
using gchar=char;
struct Settings {
 bool havebook=true,showtexts=false,showpreview=false,showcomms=false,showdicts=false;
 char *book_mod=nullptr,*book_key=nullptr;
 unsigned long book_offset=0;
} settings;
std::map<std::string,std::string> values;
std::variant<std::uint64_t,std::string> address;
struct Module {void display(){}} module;
struct Backend {
 Module *display_mod=&module;
 bool is_module(const char *){return true;}
 void set_module(const char *){}
 void set_treekey(unsigned long offset){address=static_cast<std::uint64_t>(offset);}
 unsigned long treekey_set_key(const char *key){address=std::string(key);return 0;}
} instance;
Backend *backend=&instance;
char *g_strdup(const char *value){auto *copy=static_cast<char *>(std::malloc(std::strlen(value)+1));std::strcpy(copy,value);return copy;}
void gui_reassign_strdup(char **out,char *value){std::free(*out);*out=g_strdup(value);}
void xml_set_value(const char *,const char *section,const char *key,const char *value){values[std::string(section)+"/"+key]=value;}
char *xml_get_value(const char *section,const char *key){return const_cast<char *>(values.at(std::string(section)+"/"+key).c_str());}
void main_check_unlock(const char *,bool){}
void main_setup_navbar_book(const char *,unsigned long){}
void gui_update_tab_struct(...){}
#define TRUE true
#define FALSE false
#define XI_message(value) ((void)0)
#include "legacy_book_display_actual.inc"
#undef XI_message
#undef TRUE
#undef FALSE
std::variant<std::uint64_t,std::string> run(const char *key){
 std::free(settings.book_mod);settings.book_mod=nullptr;settings.book_key=nullptr;values.clear();
 main_display_book("Book",key);return address;
}
}
int main(){
 try{
  for(const auto &key:{std::string("0"),std::string("1"),std::string("2"),std::string("3"),std::string("4"),std::string("12"),std::string("0012junk"),std::string("12.5"),std::string("2147483647"),std::string("/Part One"),std::string("/123 Numeric Title"),std::string("Part One"),std::string(""),std::string(" +12"),std::string("-4"),std::string("/café α")}){
    const SavedBookSelection saved{std::string("Book"),key,std::string("24")};
    const auto candidate=saved_book_target(saved,BookRestoreMode::display_key);
    check(candidate && candidate->module=="Book" && candidate->address==actual_legacy::run(key.c_str()),"Candidate matches extracted actual legacy display selection branch");
    const auto direct=book_display_target("Book",key);
    check(direct.module=="Book"&&direct.address==candidate->address,"Direct display policy reuses saved-display rules and complete retained branch");
  }
  SavedBookSelection saved{std::string("Book"),std::nullopt,std::nullopt};
  check(saved_book_target(saved,BookRestoreMode::display_key)->address==actual_legacy::run(nullptr),"Missing key matches null display key -> offset four");
  for(const auto &text:{"","0","1","3","4","12","0012tail","  +12tail","no digits","-0","-no digits","\t\n24"}){
    saved.offset_text=text;
    // Defined legacy settings atol -> formatted unsigned offset -> display.
    const auto number=std::atol(text);check(number>=0,"Frozen initial-offset fixture remains in defined nonnegative range");
    const auto display=std::to_string(static_cast<unsigned long>(number));
    check(saved_book_target(saved,BookRestoreMode::initial_window)->address==actual_legacy::run(display.c_str()),"Initial offset settings/display sequence matches extracted legacy display");
  }
  saved.key="/Part One";saved.offset_text="24";
  check(std::get<std::string>(saved_book_target(saved,BookRestoreMode::display_key)->address)=="/Part One" && std::get<std::uint64_t>(saved_book_target(saved,BookRestoreMode::initial_window)->address)==24,"Distinct restore contexts retain stale-offset versus newer-path behavior");
  saved.key="";saved.offset_text=std::nullopt;
  check(std::get<std::string>(saved_book_target(saved,BookRestoreMode::display_key)->address).empty() && std::get<std::uint64_t>(saved_book_target(saved,BookRestoreMode::initial_window)->address)==4,"Explicit empty path differs from missing initial offset");
  for(const auto &bad:{std::string("2147483648"),std::string(4097,'0'),std::string("4\0tail",6)})rejects([&]{book_display_target("Book",bad);});
  rejects([&]{book_display_target("","/Part One");});
  rejects([&]{book_display_target(std::string("Book\0tail",9),"/Part One");});
  saved.key="6";check(std::get<std::uint64_t>(saved_book_target(saved,BookRestoreMode::display_key)->address)==6,"Candidate does not invent SDK alignment or existence validation");
  for(const auto &bad:{"2147483648","999999999999999999999999999999999999999999"}){
    saved.key=bad;saved.offset_text=bad;
    rejects([&]{saved_book_target(saved,BookRestoreMode::display_key);});rejects([&]{saved_book_target(saved,BookRestoreMode::initial_window);});
  }
  saved.offset_text="-4";rejects([&]{saved_book_target(saved,BookRestoreMode::initial_window);});
  saved.key=std::string("12\0hidden",9);rejects([&]{saved_book_target(saved,BookRestoreMode::display_key);});
  saved.key=std::string(4097,'0');rejects([&]{saved_book_target(saved,BookRestoreMode::display_key);});
  saved.offset_text=std::string("4\0hidden",8);rejects([&]{saved_book_target(saved,BookRestoreMode::initial_window);});
  saved.offset_text=std::string(4097,'0');rejects([&]{saved_book_target(saved,BookRestoreMode::initial_window);});
  saved.module=std::string("Book\0hidden",11);rejects([&]{saved_book_target(saved,BookRestoreMode::display_key);});
  saved.module="Book";rejects([&]{saved_book_target(saved,static_cast<BookRestoreMode>(99));});
  saved.module=std::nullopt;check(!saved_book_target(saved,BookRestoreMode::initial_window),"Missing module yields no book, with no address interpretation");
  saved.module="";rejects([&]{saved_book_target(saved,static_cast<BookRestoreMode>(99));});
  check(!saved_book_target(saved,BookRestoreMode::display_key),"Empty module yields no book");
  auto source=parse_saved_book_selection("<Xiphos><modules><book>Book</book></modules><keys><book>/Part One</book><offset>24</offset></keys></Xiphos>");
  const auto target=saved_book_target(source,BookRestoreMode::display_key);source={};
  check(target->module=="Book" && std::get<std::string>(target->address)=="/Part One","Targets own source values after profile destruction");
  std::free(actual_legacy::settings.book_mod);actual_legacy::settings.book_mod=nullptr;
  std::cout<<"Extracted legacy book display/default and distinct startup target checks passed\n";return 0;
 }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
