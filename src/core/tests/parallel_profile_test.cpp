// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/profile.hpp"
#include <glib.h>
#include <libxml/parser.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace xiphos::core;
#define _(text) (text)
#define XI_message(...) ((void)0)
static xmlDocPtr xml_settings_doc;
#include "legacy_xml_profile_actual.inc"
// The retained C list setter uses a local named new; rename only that token
// while compiling its unchanged helper group in this C++ test host.
#define new legacy_new
#include "legacy_xml_search_list_save_actual.inc"
#undef new
struct Settings{gchar **parallel_list=nullptr;gchar *parallel_set_names=nullptr,*parallel_set_current=nullptr,*fnconfigure=nullptr;}settings;
using GtkWidget=void;
static struct ParallelSelect{void *sets_combo=nullptr;}parallel_select;
static std::optional<std::string> active_text;
static std::vector<std::string> events;
static gchar *gtk_combo_box_text_get_active_text(void *){return active_text?g_strdup(active_text->c_str()):nullptr;}
static void gtk_combo_box_text_remove_all(void *){events.push_back("remove-all");}
static void gtk_combo_box_text_append_text(void *,const char *text){events.push_back(std::string("append:")+text);}
static void gtk_combo_box_set_active(void *,int index){events.push_back("active:"+std::to_string(index));}
static void xml_save_settings_doc(char *){events.push_back("save");}
#define GTK_CHECK_VERSION(a,b,c) 1
#define GTK_COMBO_BOX_TEXT(value) (value)
#define GTK_COMBO_BOX(value) (value)
#define g_signal_handlers_block_by_func(object,callback,data) events.push_back("block")
#define g_signal_handlers_unblock_by_func(object,callback,data) events.push_back("unblock")
namespace frozen {
#include "legacy_parallel_sets.inc"
void startup(){gchar *parallels;
#include "legacy_parallel_startup.inc"
}
#include "legacy_parallel_delete.inc"
}
namespace actual {
#include "legacy_parallel_sets_actual.inc"
void startup(){gchar *parallels;
#include "legacy_parallel_startup_actual.inc"
}
#include "legacy_parallel_delete_actual.inc"
}
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class F>static void rejects(F f){try{f();}catch(const std::invalid_argument &){return;}throw std::runtime_error("Expected saved parallel rejection");}
static std::optional<std::string> copied(char *p){if(!p)return {};std::string result=p;xmlFree(p);return result;}
static std::optional<std::vector<std::string>> tokens(char **p){if(!p)return {};std::vector<std::string> result;for(std::size_t i=0;p[i];++i)result.push_back(p[i]);g_strfreev(p);return result;}
struct Startup{std::optional<std::vector<std::string>> modules;std::optional<std::string> names,current;std::string xml;bool operator==(const Startup &other)const{return modules==other.modules&&names==other.names&&current==other.current&&xml==other.xml;}};
static Startup startup(const std::string &source,void (*handler)()){
 xml_settings_doc=xmlReadMemory(source.data(),static_cast<int>(source.size()),nullptr,nullptr,XML_PARSE_NONET);check(xml_settings_doc,"Legacy XML fixture parses");settings={};handler();
 xmlChar *text=nullptr;int size=0;xmlDocDumpMemory(xml_settings_doc,&text,&size);
 Startup result{tokens(settings.parallel_list),copied(settings.parallel_set_names),copied(settings.parallel_set_current),{reinterpret_cast<char *>(text),static_cast<std::size_t>(size)}};xmlFree(text);xmlFreeDoc(xml_settings_doc);xml_settings_doc=nullptr;return result;
}
struct Deleted{std::string names,current,xml;std::vector<std::string> events;bool operator==(const Deleted &other)const{return names==other.names&&current==other.current&&xml==other.xml&&events==other.events;}};
static Deleted remove_set(const std::string &source,const char *names,const char *current,const std::optional<std::string> &display,void (*handler)(GtkWidget *,gpointer)){
 xml_settings_doc=xmlReadMemory(source.data(),static_cast<int>(source.size()),nullptr,nullptr,XML_PARSE_NONET);settings={};settings.parallel_set_names=g_strdup(names);settings.parallel_set_current=g_strdup(current);active_text=display;events.clear();handler(nullptr,nullptr);
 xmlChar *text=nullptr;int size=0;xmlDocDumpMemory(xml_settings_doc,&text,&size);Deleted result{settings.parallel_set_names?settings.parallel_set_names:"<null>",settings.parallel_set_current?settings.parallel_set_current:"<null>",{reinterpret_cast<char *>(text),static_cast<std::size_t>(size)},events};xmlFree(text);xmlFreeDoc(xml_settings_doc);g_free(settings.parallel_set_names);g_free(settings.parallel_set_current);return result;
}
static std::size_t comparisons=0;
static SavedParallelProfile compare(const std::string &source){
 const auto profile=parse_saved_parallel_profile(source);const auto expected=startup(source,frozen::startup);check(startup(source,actual::startup)==expected,"Complete parallel startup blocks differ");
 check(profile.startup_modules==expected.modules&&profile.names_text==expected.names&&profile.current_key==expected.current,"Owned parallel startup matches retained migration/current-set ordering");
 xml_settings_doc=xmlReadMemory(source.data(),static_cast<int>(source.size()),nullptr,nullptr,XML_PARSE_NONET);
 for(const auto &set:profile.sets){check(set.modules==tokens(frozen::get_parallel_set(set.key.c_str()))&&set.modules==tokens(actual::get_parallel_set(set.key.c_str())),"Named module data matches actual XML/scalar and frozen get_parallel_set semantics");auto *label=frozen::key_to_name(set.key.c_str());check(set.display_name==label,"Owned display preserves legacy underscore conversion");g_free(label);}
 xmlFreeDoc(xml_settings_doc);xml_settings_doc=nullptr;++comparisons;return profile;
}
static std::string bytes(const std::filesystem::path &path){std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
int main(int argc,char **argv){try{
 check(argc==2,"Disposable profile root required");const auto root=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(root);
 compare("<Xiphos/>");
 for(const auto *current:{"","A","Missing","Empty","Study_Set","Orphan"})for(const auto *configured:{"","Text,Notes","A,,B,",",,"})for(const auto *names:{"","A","A,A,,Missing,","Study_Set,A"}){
  const std::string source=std::string("<Xiphos><!--keep--><modules><parallel_set_names>")+names+"</parallel_set_names><parallel_set_current>"+current+"</parallel_set_current><parallels>"+configured+"</parallels><int1>OldA</int1><int2/><int3>OldB,OldA</int3><parallel_set_A>Text,,Notes,Text,</parallel_set_A><parallel_set_A>Shadow</parallel_set_A><parallel_set_Empty/><parallel_set_Study_Set>MissingModule</parallel_set_Study_Set><unknown keep='yes'/></modules><modules><parallel_set_Orphan>TextAlt</parallel_set_Orphan><parallel_set_A>Later</parallel_set_A></modules></Xiphos>";
  compare(source);
 }
 for(const auto *content:{""," ","&#x130;","<![CDATA[A,,B,]]>","<part>child</part>"})compare(std::string("<Xiphos><modules><parallel_set_names>A</parallel_set_names><parallel_set_A>")+content+"</parallel_set_A></modules></Xiphos>");
 std::size_t deletions=0;
 for(const auto *names:{"A","A,B,A","Study_Set,A","A,,Missing,"})for(const auto *current:{"A","Study_Set","Missing",""})for(const auto &display:std::vector<std::optional<std::string>>{std::nullopt,"New set...","A","Study Set","Study_Set","Missing"}){
  const std::string source="<Xiphos><modules><parallel_set_names>"+std::string(names)+"</parallel_set_names><parallel_set_current>"+current+"</parallel_set_current><parallel_set_A>Text</parallel_set_A><parallel_set_A>Shadow</parallel_set_A><parallel_set_Study_Set>TextAlt</parallel_set_Study_Set></modules></Xiphos>";
  check(remove_set(source,names,current,display,frozen::on_parallel_set_delete_clicked)==remove_set(source,names,current,display,actual::on_parallel_set_delete_clicked),"Complete retained parallel deletion callback trace differs");++deletions;
 }
 const std::string spaced="<Xiphos><modules><parallel_set_names>Study_Set,A</parallel_set_names><parallel_set_current>Study_Set</parallel_set_current><parallel_set_Study_Set>Text</parallel_set_Study_Set></modules></Xiphos>";
 const auto wrong=remove_set(spaced,"Study_Set,A","Study_Set","Study Set",actual::on_parallel_set_delete_clicked);check(wrong.names=="Study_Set,A"&&wrong.current=="Study_Set"&&wrong.xml.find("parallel_set_Study_Set")!=std::string::npos,"Display-name deletion fails to remove raw underscored key; native edits must use raw identity");
 const auto duplicate=compare("<Xiphos><modules><parallel_set_names>A,A,</parallel_set_names></modules><modules><parallel_set_A>Text,,Notes,</parallel_set_A></modules></Xiphos>");
 check(duplicate.sets.size()==3&&duplicate.sets[0].modules==std::vector<std::string>({"Text","","Notes",""})&&duplicate.sets[1].key=="A"&&duplicate.sets[2].key.empty()&&!duplicate.sets[2].modules,"Duplicates/blank names and trailing blank modules remain visible");
 const auto legacy=compare("<Xiphos><modules><int1>A</int1><int3>B,A</int3></modules></Xiphos>");check(legacy.startup_source==ParallelStartupSource::legacy_slots&&legacy.startup_modules_text=="A,B,A"&&legacy.legacy_slots[2]=="B,A","Read-only legacy slot migration is derived without rewriting source");
 const auto active=compare("<Xiphos><modules><parallels>Old</parallels><parallel_set_current>A</parallel_set_current><parallel_set_A>New,New</parallel_set_A></modules></Xiphos>");check(active.startup_source==ParallelStartupSource::named_set&&active.active_modules_text=="Old"&&active.startup_modules_text=="New,New","Named activation keeps raw configured list distinct from startup selection");
 check(saved_parallel_module_names("").empty()&&saved_parallel_module_names(",")==std::vector<std::string>({"",""})&&parallel_set_key("Study Set")==parallel_set_key("Study_Set")&&parallel_set_display_name("Study_Set")=="Study Set","Legacy split/name policies retain empty behavior and collision");
 std::string maximum="A";for(std::size_t i=1;i<maximum_saved_parallel_modules;++i)maximum+=",A";check(saved_parallel_module_names(maximum).size()==maximum_saved_parallel_modules,"Storage accepts more than rendering limit up to its own bound");rejects([&]{saved_parallel_module_names(maximum+",A");});
 rejects([]{saved_parallel_module_names(std::string(4097,'x'));});rejects([]{saved_parallel_module_names(std::string("A\0B",3));});rejects([]{parallel_set_key(std::string(1,static_cast<char>(0xff)));});
 std::string many="<Xiphos><modules><parallel_set_names>A";for(std::size_t i=1;i<maximum_saved_parallel_sets;++i)many+=",A";check(parse_saved_parallel_profile(many+"</parallel_set_names></modules></Xiphos>").sets.size()==maximum_saved_parallel_sets,"Exact named-set count bound");rejects([&]{parse_saved_parallel_profile(many+",A</parallel_set_names></modules></Xiphos>");});
 const std::string largeValue=std::string(4096,'x')+","+std::string(4096,'y')+","+std::string(4096,'z')+","+std::string(4096,'w');rejects([&]{parse_saved_parallel_profile(many+"</parallel_set_names><parallel_set_A>"+largeValue+"</parallel_set_A></modules></Xiphos>");});
 for(const auto &bad:std::vector<std::string>{"", "<Wrong/>","<Xiphos>","<!DOCTYPE Xiphos [<!ENTITY x 'hidden'>]><Xiphos/>",std::string("<Xiphos/>\0tail",14)})rejects([&]{parse_saved_parallel_profile(bad);});
 std::string utf16="\xff\xfe";for(char16_t c:std::u16string(u"<?xml version='1.0' encoding='UTF-16'?><Xiphos><modules><parallel_set_names>café</parallel_set_names><parallel_set_café>Text,文字</parallel_set_café></modules></Xiphos>")){utf16.push_back(static_cast<char>(c&255));utf16.push_back(static_cast<char>(c>>8));}check(compare(utf16).sets[0].modules==std::vector<std::string>({"Text","文字"}),"Declared encoding becomes owned UTF-8");
 const auto file=root/std::filesystem::u8path("parallel-café.xml");const std::string source="<Xiphos><!--original--><modules><parallel_set_names>A</parallel_set_names><parallel_set_A>Text,Missing</parallel_set_A></modules><unknown/></Xiphos>";{std::ofstream out(file,std::ios::binary);out<<source;}const auto permissions=std::filesystem::status(file).permissions();const auto loaded=read_saved_parallel_profile(file.u8string());check(loaded.sets[0].modules==std::vector<std::string>({"Text","Missing"})&&bytes(file)==source&&std::filesystem::status(file).permissions()==permissions,"Explicit parallel profile read preserves source bytes and permissions");

 // Native edits use raw keys and preserve semantic unknown XML data.
 auto edit=[](const std::string &xml,ParallelProfileAction action,const std::string &key,
              std::vector<std::string> modules={},const std::string &replacement="",bool current=false){
   return serialize_parallel_profile_edit(xml,{action,key,replacement,std::move(modules),current});
 };
 const std::string editable="<Xiphos custom='yes'><!--keep--><modules><parallel_set_names>Study_Set,A,Study_Set,</parallel_set_names><parallel_set_current>Study_Set</parallel_set_current><parallels>Old</parallels><parallel_set_Study_Set mark='keep'>Text,Notes</parallel_set_Study_Set><parallel_set_Study_Set>Shadow</parallel_set_Study_Set><parallel_set_A/><unknown flag='yes'/></modules><other>retain</other></Xiphos>";
 const auto renamed_xml=edit(editable,ParallelProfileAction::rename,"Study_Set",{},"Renamed");
 const auto renamed=parse_saved_parallel_profile(renamed_xml);
 check(renamed.names_text=="Renamed,A,Renamed,"&&renamed.current_key=="Renamed"&&renamed.sets[0].module_text=="Text,Notes"&&renamed.active_modules_text=="Old","Rename tracks all duplicate raw names/current without activating");
 for(const auto *kept:{"custom=\"yes\"","<!--keep-->","mark=\"keep\"","<parallel_set_Study_Set>Shadow","<unknown flag=\"yes\"/>","<other>retain</other>"})check(renamed_xml.find(kept)!=std::string::npos,"Rename preserves unrelated data and shadow definitions");
 check(edit(editable,ParallelProfileAction::rename,"Study_Set",{},"Study_Set")==editable,"Identity rename preserves original bytes");
 const auto removed=parse_saved_parallel_profile(edit(editable,ParallelProfileAction::remove,"Study_Set"));
 check(removed.names_text=="A,"&&(!removed.current_key||removed.current_key->empty())&&removed.active_modules_text=="Old","Raw deletion removes duplicate names and clears current without selecting a fallback");
 check(edit(editable,ParallelProfileAction::remove,"Orphan")==editable,"Unlisted deletion preserves unknown definitions and bytes");
 const auto saved_xml=edit(editable,ParallelProfileAction::save,"Study_Set",{"Text&<>","文字","","Text&<>"},"",true);
 const auto saved=parse_saved_parallel_profile(saved_xml);
 check(saved.names_text=="Study_Set,A,Study_Set,"&&saved.sets[0].modules==std::vector<std::string>({"Text&<>","文字","","Text&<>"})&&saved.active_modules_text=="Text&<>,文字,,Text&<>","Save preserves duplicate labels, literal XML text and module ordering");
 const auto created=parse_saved_parallel_profile(edit("<Xiphos><other/></Xiphos>",ParallelProfileAction::save,"New",{"Text"},"",true));
 check(created.names_text=="New"&&created.current_key=="New"&&created.startup_modules_text=="Text","Explicit save creates missing modules section");
 const auto appended=parse_saved_parallel_profile(edit(editable,ParallelProfileAction::save,"New",{"Notes"}));
 check(appended.names_text=="Study_Set,A,Study_Set,,New"&&appended.current_key=="Study_Set","Append retains trailing blank name and existing current");
 const auto activated=parse_saved_parallel_profile(edit(editable,ParallelProfileAction::activate,"Study_Set"));
 check(activated.current_key=="Study_Set"&&activated.active_modules_text=="Text,Notes","Activation updates configured modules");
 const auto empty_activation=parse_saved_parallel_profile(edit(editable,ParallelProfileAction::activate,"A"));
 check(empty_activation.current_key=="A"&&empty_activation.active_modules_text=="Old","Empty definition activation retains old configured modules");
 const auto missing=parse_saved_parallel_profile(edit("<Xiphos><modules><parallel_set_names>A</parallel_set_names><parallels>Old</parallels></modules></Xiphos>",ParallelProfileAction::rename,"A",{},"B"));
 check(missing.names_text=="B"&&!missing.sets[0].module_text,"Rename of listed missing definition does not invent modules");
 const std::string later="<Xiphos><modules><parallel_set_names>A</parallel_set_names></modules><modules><parallel_set_A>Later</parallel_set_A></modules></Xiphos>";
 check(parse_saved_parallel_profile(edit(later,ParallelProfileAction::save,"A",{"Changed"})).sets[0].module_text=="Changed","Save updates first effective definition across duplicate sections");
 check(edit(later,ParallelProfileAction::remove,"A").find("<parallel_set_A>Later</parallel_set_A>")!=std::string::npos,"Remove retains later-section definition as legacy does");
 for(const auto &key:std::vector<std::string>{"","names","current","A,B","A B","A:B",std::string(1,'\1'),std::string(4097,'x')})rejects([&]{edit(editable,ParallelProfileAction::save,key,{"Text"});});
 for(const auto &token:std::vector<std::string>{"A,B",std::string(1,'\1'),std::string("A\0B",3),std::string(4097,'x')})rejects([&]{edit(editable,ParallelProfileAction::save,"New",{token});});
 rejects([&]{edit(editable,ParallelProfileAction::rename,"Study_Set",{},"A");});
 rejects([&]{edit(editable+" ",ParallelProfileAction::activate,"Missing");});
 rejects([&]{edit("<Xiphos><modules><parallel_set_names>A</parallel_set_names><parallel_set_Orphan/></modules></Xiphos>",ParallelProfileAction::rename,"A",{},"Orphan");});
 rejects([&]{edit(editable,ParallelProfileAction::rename,"Missing",{},"New");});
 rejects([&]{edit(editable,static_cast<ParallelProfileAction>(99),"A");});
 rejects([&]{edit(editable,ParallelProfileAction::remove,"A",{"unexpected"});});
 rejects([&]{edit(editable,ParallelProfileAction::save,"A",{},"unexpected");});
 rejects([&]{edit(editable,ParallelProfileAction::save,"A",std::vector<std::string>(maximum_saved_parallel_modules+1,"x"));});
 // Compare raw-key native deletion with the entire retained callback on its supported identity domain.
 for(const auto *names:{"A","A,B,A","A,,Missing,"}){
   const std::string input="<Xiphos><modules><parallel_set_names>"+std::string(names)+"</parallel_set_names><parallel_set_current>A</parallel_set_current><parallel_set_A>Text</parallel_set_A></modules></Xiphos>";
   const auto old=remove_set(input,names,"A","A",actual::on_parallel_set_delete_clicked);
   const auto native=parse_saved_parallel_profile(edit(input,ParallelProfileAction::remove,"A"));
   check(native.names_text.value_or("")==old.names&&native.current_key==parse_saved_parallel_profile(old.xml).current_key,"Native raw deletion matches complete legacy callback");
 }
 const auto destination=root/"parallel-edited.xml";std::filesystem::remove(destination);
 const auto committed=edit_parallel_profile(destination.u8string(),{ParallelProfileAction::save,"Saved","",{"Text","Notes"},true});
 check(committed.startup_modules_text=="Text,Notes"&&read_saved_parallel_profile(destination.u8string()).names_text==committed.names_text,"Missing file save returns verified committed snapshot");
 const auto original_bytes=bytes(destination);const auto original_permissions=std::filesystem::status(destination).permissions();
 rejects([&]{edit_parallel_profile(destination.u8string(),{ParallelProfileAction::rename,"Saved","names",{},false});});
 check(bytes(destination)==original_bytes&&std::filesystem::status(destination).permissions()==original_permissions,"Rejected edit preserves bytes and permissions");
 const auto final=edit_parallel_profile(destination.u8string(),{ParallelProfileAction::rename,"Saved","Renamed",{},false});
 check(final.current_key=="Renamed"&&final.sets[0].modules==std::vector<std::string>({"Text","Notes"})&&std::filesystem::status(destination).permissions()==original_permissions,"Atomic rename retains modules and file permissions");
 rejects([&]{edit_parallel_profile((root/"missing-edit.xml").u8string(),{ParallelProfileAction::activate,"Saved","",{},false});});
 std::cout<<"Saved parallel profiles: "<<comparisons<<" XML/startup and "<<deletions<<" complete delete comparisons, raw names/duplicates/nulls, legacy migration, bounded owned data and read-only files passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
