// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/dictionary.hpp"
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>
#include "xiphos/core/engine_manager.hpp"
#include <markupfiltmgr.h>
#include <rawld.h>
#include <rawtext.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
xc_status wait(xc_request *request) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  xc_status status;
  while ((status = xc_request_poll(request)) == XC_PENDING) {
    check(std::chrono::steady_clock::now() < deadline, "C dictionary request deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return status;
}
namespace actual_legacy_dictionary {
using gint = int; using gchar = char;
struct Settings { bool havedict = true; const char *DictWindowModule = "DictionaryOne"; const char *dictkey = nullptr; } settings;
struct Entry { std::string text, html; unsigned activations=0; } entry;
struct Widgets { Entry *entry_dict = &entry; } widgets;
struct Backend {
  sword::SWMgr *manager = nullptr; sword::SWModule *display_mod = nullptr;
  void set_module_key(const char *id, const char *key) { display_mod=manager->getModule(id); display_mod->setKey(key); }
  char *get_module_key() { const auto *text=display_mod->getKeyText(); auto *copy=static_cast<char *>(std::malloc(std::strlen(text)+1)); std::strcpy(copy,text); return copy; }
} instance;
Backend *backend = &instance;
gchar *g_strdup(const char *value) { auto *copy=static_cast<gchar *>(std::malloc(std::strlen(value)+1)); std::strcpy(copy,value); return copy; }
void g_free(void *value) { std::free(value); }
void gtk_entry_set_text(Entry *target,const char *value) { target->text=value; }
void gtk_widget_activate(Entry *target) { ++target->activations; target->html=render_entry(*backend->display_mod,target->text.c_str()); }
struct GtkWidget { std::string label, action_key; std::vector<std::unique_ptr<GtkWidget>> children; };
GtkWidget *gtk_menu_new() { return new GtkWidget; }
GtkWidget *gtk_menu_item_new_with_label(const char *text) { auto *item=new GtkWidget; item->label=text; return item; }
void gtk_widget_show(GtkWidget *) {}
void gtk_container_add(GtkWidget *menu,GtkWidget *item) { menu->children.emplace_back(item); }
void g_signal_connect(GtkWidget *item,const char *,void *,char *key) { item->action_key=key; std::free(key); }
const char *gtk_entry_get_text(Entry *target) { return target->text.c_str(); }
std::string configured_module="DictionaryOne", configured_key;
void xml_set_value(const char *,const char *,const char *name,const char *value) { if (!std::strcmp(name,"dict")) configured_module=value; else if (!std::strcmp(name,"dictionary")) configured_key=value; }
char *xml_get_value(const char *group,const char *) { return const_cast<char *>((!std::strcmp(group,"modules")?configured_module:configured_key).c_str()); }
void main_check_unlock(const char *,bool) {}
void ignore_log(const char *,...) {}
#define GTK_ENTRY(value) (value)
#define GTK_CONTAINER(value) (value)
#define G_OBJECT(value) (value)
#define G_CALLBACK(value) nullptr
#define XI_message(message) ignore_log message
#define TRUE true
#include "legacy_dictionary_adapter.inc"
#include "legacy_dictionary_choices_actual.inc"
#undef GTK_ENTRY
#undef GTK_CONTAINER
#undef G_OBJECT
#undef G_CALLBACK
#undef XI_message
#undef TRUE
}
int main(int argc, char **argv) {
  try {
    check(argc == 2, "dictionary fixture root required"); const std::filesystem::path root(argv[1]);
    std::filesystem::create_directories(root / "mods.d");
    const std::map<std::string,std::string> features{{"DictionaryOne","GreekDef"},{"DictionaryTwo","HebrewDef"},
      {"MorphGreek","GreekParse"},{"MorphHebrew","HebrewParse"},{"GenericDictionary",""},
      {"OriginalDevotional","DailyDevotion"},{"CaseFeature","greekdef"},{"MultipleFeatures","GreekParse"}};
    for (const auto &fixture : features) {
      const auto *id=fixture.first.c_str();
      std::filesystem::create_directories(root / id);
      const auto path = (root / id / "lexicon").string();
      check(sword::RawLD::createModule(path.c_str()) == 0, "create original dictionary");
      sword::RawLD writer(path.c_str());
      writer.setKey("alpha"); writer.setEntry((std::string("<p>Original ") + id + " café <hi type=\"italic\">alpha</hi>.</p>").c_str());
      writer.setKey("café"); writer.setEntry("<p>Original Unicode key definition.</p>");
      writer.setKey("03056"); writer.setEntry("<p>Original word definition.</p>");
      writer.setKey("N-NSM"); writer.setEntry("<p>Original morphology definition.</p>");
      writer.setKey("G03056"); writer.setEntry("<q who=\"Jesus\">Original keyed word definition.</q>");
      std::ofstream config(root / "mods.d" / (std::string(id) + ".conf"));
      config << '[' << id << "]\nDataPath=./" << id << "/lexicon\nModDrv=RawLD\nSourceType=OSIS\nEncoding=UTF-8\n"
                "Description=Original dictionary fixture\nLang=en\nGlobalOptionFilter=OSISRedLetterWords\n";
      if (!fixture.second.empty()) config << "Feature=" << fixture.second << '\n';
      if (fixture.first=="MultipleFeatures") config << "Feature=HebrewDef\n";
    }
    std::filesystem::create_directories(root / "bible");
    check(sword::RawText::createModule((root / "bible").string().c_str()) == 0, "create non-dictionary fixture");
    { sword::RawText writer((root / "bible").string().c_str()); writer.setKey("John3:16"); writer.setEntry("Original Bible fixture: <w lemma=\"strong:G03056\" morph=\"robinson:N-NSM\">word</w>."); }
    { std::ofstream config(root / "mods.d/bible.conf"); config << "[OriginalBible]\nDataPath=./bible/\nModDrv=RawText\nSourceType=OSIS\nEncoding=UTF-8\nFeature=GreekDef\nGlobalOptionFilter=OSISStrongs\nGlobalOptionFilter=OSISMorph\n"; }
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    const auto path = root.string();
    std::map<std::string, DictionaryEntry> expected;
    const std::vector<std::string> keys = {"alpha", "ALPHA", "café", "G03056", "G3056", "between", "zzzz"};
    std::string plainWord, redWord;
    std::map<std::pair<std::string,int>,DictionaryEntry> adjacent;
    std::map<std::string,std::vector<std::string>> nearby;
    std::map<std::string,unsigned> expectedRoles;
    std::map<std::string,bool> expectedDictionary;
    {
      EngineManager legacy(path.c_str(), true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
      for (const auto &item : legacy.Modules) {
        auto &m=*item.second; const auto *feature=m.getConfigEntry("Feature");
        const bool dictionary=std::string(m.getType())=="Lexicons / Dictionaries" && (!feature || std::string(feature)!="DailyDevotion");
        unsigned roles=0;
        // The legacy preferences comparator tests exact first Feature equality.
        if (dictionary && feature) {
          if (std::string(feature)=="GreekDef") roles=XIPHOS_LEXICON_GREEK_DEF;
          if (std::string(feature)=="HebrewDef") roles=XIPHOS_LEXICON_HEBREW_DEF;
          if (std::string(feature)=="GreekParse") roles=XIPHOS_LEXICON_GREEK_PARSE;
          if (std::string(feature)=="HebrewParse") roles=XIPHOS_LEXICON_HEBREW_PARSE;
        }
        expectedRoles[m.getName()]=roles; expectedDictionary[m.getName()]=dictionary;
      }
      auto *module = legacy.getModule("DictionaryOne"); check(module != nullptr, "load original dictionary");
      for (const auto &key : keys) {
        // Same setKey/renderText sequence as BackEnd::get_render_text.
        const auto html = render_entry(*module, key.c_str());
        expected.emplace(key, DictionaryEntry{"DictionaryOne", module->getKeyText(), html});
      }
      for (const auto &key : keys) for (int direction : {-1, 1}) {
        // Frozen sequence from main_dictionary_button_clicked and activated
        // main_dictionary_entry_changed, including boundary/nearest-key cases.
        module->setKey(key.c_str());
        if (direction == -1) (*module)--; else (*module)++;
        const std::string target = module->getKeyText();
        const auto html = render_entry(*module, target.c_str());
        adjacent.emplace(std::make_pair(key,direction),DictionaryEntry{"DictionaryOne",module->getKeyText(),html});
      }
      for (const auto &key : keys) {
        (void)render_entry(*module,key.c_str());
        for (int i=0;i<5;++i) (*module)--;
        auto &values=nearby[key];
        for (int i=0;i<9;++i) { (*module)++; values.emplace_back(module->getKeyText()); }
      }
      actual_legacy_dictionary::instance.manager=&legacy;
      for (const auto &item : adjacent) {
        actual_legacy_dictionary::settings.dictkey=item.first.first.c_str();
        const auto before=actual_legacy_dictionary::entry.activations;
        actual_legacy_dictionary::main_dictionary_button_clicked(item.first.second==-1?0:1);
        check(actual_legacy_dictionary::entry.text==item.second.key && actual_legacy_dictionary::entry.html==item.second.html &&
          actual_legacy_dictionary::entry.activations==before+1,"actual legacy key adapter equals frozen SDK sequence");
      }
      for (const auto &key : keys) {
        actual_legacy_dictionary::entry.text=key;
        std::unique_ptr<actual_legacy_dictionary::GtkWidget> menu(actual_legacy_dictionary::main_dictionary_drop_down_new(
          const_cast<char *>("DictionaryOne"),const_cast<char *>(key.c_str())));
        check(menu && menu->children.size()==9,"actual dictionary chooser retains nine entries");
        for (std::size_t i=0;i<menu->children.size();++i)
          check(menu->children[i]->label==nearby.at(key)[i] && menu->children[i]->action_key==nearby.at(key)[i],
            "actual legacy chooser labels/actions equal frozen SDK order including repeats");
      }
      const auto activations=actual_legacy_dictionary::entry.activations;
      actual_legacy_dictionary::settings.havedict=false;
      actual_legacy_dictionary::main_dictionary_button_clicked(1);
      actual_legacy_dictionary::settings.havedict=true; actual_legacy_dictionary::settings.DictWindowModule=nullptr;
      actual_legacy_dictionary::main_dictionary_button_clicked(0);
      check(actual_legacy_dictionary::entry.activations==activations,"actual legacy dictionary guards retain no-op behavior");
      actual_legacy_dictionary::settings.DictWindowModule="DictionaryOne";
      actual_legacy_dictionary::settings.dictkey=nullptr; actual_legacy_dictionary::instance.manager=nullptr;
      legacy.setGlobalOption("Words of Christ in Red", "Off"); plainWord = render_entry(*module, "G03056");
      legacy.setGlobalOption("Words of Christ in Red", "On"); redWord = render_entry(*module, "G03056");
      check(plainWord != redWord && redWord.find("wordsOfJesus") != std::string::npos, "dictionary options affect actual output");
    }
    DictionaryEntry owned;
    {
      Reader reader(path);
      for (const auto &m : reader.modules()) check(m.dictionary==expectedDictionary.at(m.id) && m.lexicon_roles==expectedRoles.at(m.id),
        "core dictionary/role metadata equals legacy type and first Feature comparator");
      for (const auto &key : keys) {
        const auto result = reader.read_dictionary("DictionaryOne", key);
        check(result.module == "DictionaryOne" && result.key == expected.at(key).key && result.html == expected.at(key).html,
              "dictionary key/render behavior equals direct legacy SDK calls");
      }
      for (const auto &key : keys) {
        const auto choices=reader.dictionary_choices("DictionaryOne",key);
        check(choices.module=="DictionaryOne" && choices.keys==nearby.at(key),"owned core dictionary chooser equals frozen/actual order");
      }
      for (const auto &item : adjacent) {
        const auto result = reader.navigate_dictionary("DictionaryOne",item.first.first,item.first.second);
        check(result.key == item.second.key && result.html == item.second.html,
          "dictionary key navigation equals legacy SDK sequence including boundaries");
      }
      for (int direction : {0, 2, -2}) {
        bool invalid=false;
        try { reader.navigate_dictionary("DictionaryOne","alpha",direction); } catch (const std::invalid_argument &) { invalid=true; }
        check(invalid,"invalid dictionary direction rejected");
      }
      for (const auto &key : {std::string(),std::string("alpha\0ignored",13)}) {
        bool invalid=false;
        try { reader.navigate_dictionary("DictionaryOne",key,1); } catch (const std::invalid_argument &) { invalid=true; }
        check(invalid,"invalid dictionary navigation key rejected");
      }
      StudySession first(reader), second(reader);
      auto options = engine_options_from_preferences({}); options.words_in_red = true;
      first.set_module_engine_options("DictionaryOne", options); options.words_in_red = false;
      second.set_module_engine_options("DictionaryOne", options);
      first.visit("OriginalBible", "John3:16");
      check(first.read_dictionary("DictionaryOne", "G03056").html == redWord && second.read_dictionary("DictionaryOne", "G03056").html == plainWord,
            "dictionary output uses each tab's module options");
      const auto next = reader.navigate_dictionary("DictionaryOne","CAFÉ",1);
      const auto redNext=first.navigate_dictionary("DictionaryOne","CAFÉ",1);
      const auto plainNext=second.navigate_dictionary("DictionaryOne","CAFÉ",1);
      check(next.key == expected.at("G03056").key && redNext.html==redWord && plainNext.html==plainWord,
        "dictionary navigation uses separate tab options");
      check(first.history().entries().size() == 1 && second.history().entries().empty(), "dictionary previews do not visit Bible history");
      check(first.read_dictionary("DictionaryTwo", "alpha").html.find("DictionaryTwo") != std::string::npos, "dictionary module context is explicit");
      bool rejected = false; try { reader.read_dictionary("DictionaryOne", ""); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "empty dictionary key rejected");
      rejected = false; try { reader.read_dictionary("OriginalBible", "John3:16"); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "verse module cannot masquerade as dictionary");
      rejected = false; try { reader.read_dictionary("Missing", "alpha"); } catch (const std::out_of_range &) { rejected = true; }
      check(rejected, "missing dictionary rejected");
      owned = reader.read_dictionary("DictionaryOne", "alpha");
    }
    check(owned.html.find("café") != std::string::npos, "dictionary snapshot owns Unicode content after reader teardown");
    std::future<DictionaryEntry> accepted;
    {
      StudyWorker worker(path); const auto tab = worker.create_tab().get();
      worker.enable_module_preferences(tab).get();
      std::string key = "alpha"; accepted = worker.read_dictionary(tab, "DictionaryOne", key); key.clear();
      bool rejected = false; try { (void)worker.read_dictionary(tab, "Missing", "alpha").get(); } catch (const std::out_of_range &) { rejected = true; }
      check(rejected && worker.history(tab).get().entries.empty(), "dictionary worker error preserves history and queue");
      const auto choices=worker.dictionary_choices(tab,"DictionaryOne","alpha").get();
      check(choices.keys==nearby.at("alpha") && worker.dictionary_history(tab).get().entries.empty(),"worker chooser has no history side effect");
      const auto navigation=worker.navigate_dictionary(tab,"DictionaryOne","alpha",1).get();
      check(navigation.key==adjacent.at({"alpha",1}).key && worker.history(tab).get().entries.empty(),
        "worker dictionary navigation returns copied result without Bible visit");
      worker.close_tab(tab).get(); rejected = false;
      try { (void)worker.read_dictionary(tab, "DictionaryOne", "alpha").get(); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "closed dictionary tab rejected");
    }
    check(accepted.get().html == owned.html, "copied dictionary request/result survives worker shutdown");
    {
      xc_reader *reader=nullptr; check(xc_open(path.c_str(),&reader)==XC_OK,"C synchronous dictionary catalog");
      for (std::size_t i=0;i<xc_module_count(reader);++i) {
        const std::string id=xc_module_id(reader,i);
        check((xc_module_is_dictionary(reader,i)!=0)==expectedDictionary.at(id) && xc_module_lexicon_roles(reader,i)==expectedRoles.at(id),"C synchronous dictionary roles");
      }
      check(!xc_module_is_dictionary(reader,xc_module_count(reader)) && !xc_module_lexicon_roles(reader,xc_module_count(reader)) &&
            !xc_module_is_dictionary(nullptr,0) && !xc_module_lexicon_roles(nullptr,0),"C synchronous metadata bounds"); xc_close(reader);
    }
    xc_dictionary_entry *retained = nullptr;
    xc_dictionary_choices *retainedChoices = nullptr;
    xc_request *acceptedRequest = nullptr;
    {
      xc_worker *worker = nullptr; xc_request *request = nullptr;
      check(xc_worker_open(path.c_str(), &worker) == XC_OK, "C dictionary worker");
      check(xc_worker_modules(worker,&request)==XC_OK && wait(request)==XC_OK,"C dictionary catalog");
      auto *catalog=xc_request_take_catalog(request); xc_request_release(request);
      for (std::size_t i=0;i<xc_catalog_count(catalog);++i) {
        const std::string id=xc_catalog_id(catalog,i);
        check((xc_catalog_is_dictionary(catalog,i)!=0)==expectedDictionary.at(id) && xc_catalog_lexicon_roles(catalog,i)==expectedRoles.at(id),"C dictionary catalog roles");
      }
      check(!xc_catalog_is_dictionary(catalog,xc_catalog_count(catalog)) && !xc_catalog_lexicon_roles(catalog,xc_catalog_count(catalog)) &&
            !xc_catalog_is_dictionary(nullptr,0) && !xc_catalog_lexicon_roles(nullptr,0),"C catalog metadata bounds"); xc_catalog_release(catalog);
      check(xc_worker_create_tab(worker, &request) == XC_OK && wait(request) == XC_OK, "C dictionary tab");
      const auto tab = xc_request_tab(request);
      check(!xc_request_take_dictionary_entry(request), "C tab result cannot be dictionary");
      xc_request_release(request);
      check(xc_worker_set_module_engine_options(worker, tab, "DictionaryOne", XC_ENGINE_WORDS_IN_RED, &request) == XC_OK &&
            wait(request) == XC_OK, "C dictionary settings"); xc_request_release(request);
      std::string key = "G03056";
      check(xc_worker_read_dictionary(worker, tab, "DictionaryOne", key.c_str(), &request) == XC_OK, "C dictionary submission"); key.clear();
      check(!xc_request_take_dictionary_entry(request) && wait(request) == XC_OK && !xc_request_take_entry(request) &&
            !xc_request_take_note(request), "C dictionary pending/type separation");
      retained = xc_request_take_dictionary_entry(request);
      check(retained && !xc_request_take_dictionary_entry(request) && std::string(xc_dictionary_entry_module(retained)) == "DictionaryOne" &&
            !std::string(xc_dictionary_entry_key(retained)).empty() && std::string(xc_dictionary_entry_html(retained)) == redWord,
            "C dictionary copied arguments, settings and take-once ownership"); xc_request_release(request);
      check(xc_worker_navigate_dictionary(worker,tab,"DictionaryOne","CAFÉ",1,&request)==XC_OK && wait(request)==XC_OK,
        "C dictionary navigation submission");
      auto *navigated=xc_request_take_dictionary_entry(request);
      check(navigated && !xc_request_take_dictionary_entry(request) && std::string(xc_dictionary_entry_html(navigated))==redWord,
        "C dictionary navigation options and take-once result");
      xc_request_release(request); xc_dictionary_entry_release(navigated);
      check(xc_worker_navigate_dictionary(worker,tab,"DictionaryOne","alpha",0,&request)==XC_INVALID_ARGUMENT && !request,
        "C dictionary navigation invalid direction prequeue");
      check(xc_worker_dictionary_choices(worker,tab,"DictionaryOne","alpha",&request)==XC_OK && wait(request)==XC_OK,"C dictionary chooser");
      auto *choices=xc_request_take_dictionary_choices(request);
      check(choices && !xc_request_take_dictionary_choices(request) && xc_dictionary_choices_count(choices)==9 &&
        std::string(xc_dictionary_choices_module(choices))=="DictionaryOne" && !xc_dictionary_choices_at(choices,9) &&
        !xc_dictionary_choices_module(nullptr) && !xc_dictionary_choices_count(nullptr),"C chooser owned kind/bounds");
      for (std::size_t i=0;i<9;++i) check(std::string(xc_dictionary_choices_at(choices,i))==nearby.at("alpha")[i],"C copied chooser keys");
      xc_request_release(request); retainedChoices=choices;
      for (const char *id : std::initializer_list<const char *>{nullptr, ""}) {
        check(xc_worker_read_dictionary(worker, tab, id, "alpha", &request) == XC_INVALID_ARGUMENT && !request, "C invalid dictionary module");
        check(xc_worker_read_dictionary(worker, tab, "DictionaryOne", id, &request) == XC_INVALID_ARGUMENT && !request, "C invalid dictionary key");
      }
      check(xc_worker_read_dictionary(worker, tab, "DictionaryOne", "alpha", nullptr) == XC_INVALID_ARGUMENT, "C missing dictionary output");
      check(xc_worker_read_dictionary(worker, tab, "Missing", "alpha", &request) == XC_OK && wait(request) == XC_MODULE_NOT_FOUND &&
            !xc_request_take_dictionary_entry(request), "C missing dictionary status"); xc_request_release(request);
      check(xc_worker_read_dictionary(worker, tab, "OriginalBible", "John3:16", &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT,
            "C verse module type rejected"); xc_request_release(request);
      check(xc_worker_history(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "C queue continues after dictionary failures");
      auto *history = xc_request_take_history(request); check(history && xc_history_count(history) == 0, "C dictionary preserves history");
      xc_history_release(history); xc_request_release(request);
      check(xc_worker_read_dictionary(worker, tab, "DictionaryOne", "alpha", &acceptedRequest) == XC_OK, "C retained dictionary observer");
      check(xc_worker_close_tab(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "C dictionary close tab"); xc_request_release(request);
      check(xc_worker_read_dictionary(worker, tab, "DictionaryOne", "alpha", &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT,
            "C closed dictionary tab rejected"); xc_request_release(request);
      check(xc_worker_dictionary_choices(worker,tab,"DictionaryOne","alpha",&request)==XC_OK && wait(request)==XC_INVALID_ARGUMENT,
        "closed dictionary chooser tab rejected"); xc_request_release(request);
      xc_worker_close(worker);
    }
    check(std::string(xc_dictionary_choices_at(retainedChoices,0))==nearby.at("alpha")[0],"C chooser snapshot survives worker/request teardown");
    xc_dictionary_choices_release(retainedChoices);
    check(wait(acceptedRequest) == XC_OK, "C accepted dictionary observed after worker close");
    auto *afterClose = xc_request_take_dictionary_entry(acceptedRequest); xc_request_release(acceptedRequest);
    check(afterClose && std::string(xc_dictionary_entry_html(afterClose)).find("café") != std::string::npos &&
          std::string(xc_dictionary_entry_html(retained)) == redWord, "C dictionary handles survive worker/request teardown");
    xc_dictionary_entry_release(afterClose); xc_dictionary_entry_release(retained);
    check(!xc_request_take_dictionary_entry(nullptr) && !xc_dictionary_entry_module(nullptr) &&
          !xc_dictionary_entry_key(nullptr) && !xc_dictionary_entry_html(nullptr), "C NULL dictionary handles safe");
    xc_dictionary_entry_release(nullptr);
    std::cout << "Shared dictionary key/render/context/lifetime checks passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
