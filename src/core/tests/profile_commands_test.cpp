// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/profile.hpp"
#include "xiphos/core/c_api.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <clocale>
using namespace xiphos::core;
namespace {
unsigned checks=0;
void check(bool condition,const char *message) { ++checks; if(!condition) throw std::runtime_error(message); }
template<class F> void rejected(F operation) { bool failed=false; try { operation(); } catch(const std::invalid_argument &) { failed=true; } check(failed,"invalid profile operation rejected"); }
void write(const std::filesystem::path &file,const std::string &value) { std::ofstream out(file,std::ios::binary); out.exceptions(std::ios::badbit|std::ios::failbit); out<<value; out.close(); }
std::string bytes(const std::filesystem::path &file) { std::ifstream in(file,std::ios::binary); return std::string((std::istreambuf_iterator<char>(in)),{}); }
xc_status wait(xc_request *request) {
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
 xc_status status;
 while((status=xc_request_poll(request))==XC_PENDING) {
  check(std::chrono::steady_clock::now()<deadline,"profile command deadline");
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
 }
 return status;
}
}
int main(int argc,char **argv) {
 try {
  check(argc==3,"dictionary/temporary profile roots required");
  const std::filesystem::path root=std::filesystem::u8path(argv[2]); std::filesystem::create_directories(root);
  const auto file=root/std::filesystem::u8path("profile-café.xml"),saved=root/"saved.xml",invalid=root/"invalid.xml";
  const std::string original="<Xiphos><lexicons><greek>DictionaryOne</greek><hebrew>UnavailableHebrew</hebrew><greek_morph_nt>MorphGreek</greek_morph_nt><hebrew_morph>MorphHebrew</hebrew_morph></lexicons><locale><special>tr_TR.UTF-8</special></locale><unknown>retain</unknown></Xiphos>";
  write(file,original); write(invalid,"<Xiphos>"); std::filesystem::remove(saved);
  const std::string previous_locale=std::setlocale(LC_ALL,nullptr);
  std::future<std::optional<LexiconPreferences>> surviving;
  {
   StudyWorker worker(argv[1]); const auto first=worker.create_tab().get(),second=worker.create_tab().get();
   (void)worker.visit(first,"OriginalBible","John3:16").get();
   (void)worker.visit_dictionary(first,"DictionaryOne","alpha").get();
   EngineOptions engine; engine.strongs=true;
   worker.set_module_engine_options(first,"OriginalBible",engine).get();
   rejected([&] { worker.save_lexicon_profile(second,saved.u8string()).get(); });
   check(!std::filesystem::exists(saved),"unconfigured save rejected before file creation");
   std::string owned_path=file.u8string(); auto imported=worker.import_lexicon_profile(first,owned_path); owned_path.clear();
   const auto choices=imported.get();
   check(choices && choices->greek=="DictionaryOne" && choices->hebrew=="UnavailableHebrew" && choices->greek_morph_ot=="Packard","copied import input and complete legacy defaults");
   check(!worker.lexicon_preferences(second).get(),"other tab remains unconfigured");
   check(worker.module_engine_options(first,"OriginalBible").get()->strongs,"XML lexicon import preserves engine preferences");
   check(worker.read_lexical(first,"OriginalBible","John3:16","xiphos.url?action=showStrongs&type=Greek&value=03056",XIPHOS_LEXICON_MAIN).get().entry.module=="DictionaryOne","imported selection drives shared lexical lookup");
   const auto history=worker.history(first).get(); const auto dictionary=worker.dictionary_history(first).get();
   check(history.entries.size()==1 && dictionary.entries.size()==1,"import/read do not visit histories");
   rejected([&] { (void)worker.import_lexicon_profile(first,invalid.u8string()).get(); });
   check(worker.lexicon_preferences(first).get()->greek=="DictionaryOne","failed import preserves prior complete choices");
   check(worker.history(first).get().entries.size()==history.entries.size() && worker.dictionary_history(first).get().entries.size()==dictionary.entries.size(),"failure preserves both histories and queue");
   worker.save_lexicon_profile(first,saved.u8string()).get();
   check(read_study_profile(saved.u8string()).lexicons.hebrew=="UnavailableHebrew","configured unavailable names survive save");
   check(bytes(file)==original,"import source remains byte exact");
   worker.close_tab(second).get();
   const auto closed_file=root/"closed.xml"; std::filesystem::remove(closed_file);
   rejected([&] { worker.save_lexicon_profile(second,closed_file.u8string()).get(); });
   rejected([&] { (void)worker.import_lexicon_profile(second,file.u8string()).get(); });
   check(!std::filesystem::exists(closed_file),"closed tab rejected before file access/mutation");
   surviving=worker.import_lexicon_profile(first,file.u8string());
  }
  check(surviving.get()->greek=="DictionaryOne","accepted copied import result survives worker drain/shutdown");
  xc_worker *worker=nullptr; xc_request *request=nullptr;
  check(xc_worker_open(argv[1],&worker)==XC_OK,"C profile worker");
  check(xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"C profile tab");
  const auto tab=xc_request_tab(request); xc_request_release(request);
  check(xc_worker_import_lexicon_profile(worker,tab,file.u8string().c_str(),&request)==XC_OK && wait(request)==XC_OK,"C import request completes");
  check(!xc_request_take_entry(request),"wrong result kind cannot consume lexicons");
  auto *names=xc_request_take_lexicons(request); check(names && xc_lexicons_configured(names) && !xc_request_take_lexicons(request),"configured snapshot transfers once");
  xc_request_release(request);
  check(std::string(xc_lexicons_greek(names))=="DictionaryOne" && std::string(xc_lexicons_hebrew(names))=="UnavailableHebrew","copied imported names");
  const auto c_saved=root/"c-saved.xml";
  check(xc_worker_save_lexicon_profile(worker,tab,c_saved.u8string().c_str(),&request)==XC_OK && wait(request)==XC_OK,"C save completion");
  check(!xc_request_take_lexicons(request),"void save cannot transfer lexicon snapshot"); xc_request_release(request);
  check(read_study_profile(c_saved.u8string()).lexicons.greek=="DictionaryOne","C save uses shared writer");
  check(xc_worker_import_lexicon_profile(worker,tab,invalid.u8string().c_str(),&request)==XC_OK && wait(request)==XC_INVALID_ARGUMENT,"C asynchronous parse error"); xc_request_release(request);
  check(xc_worker_lexicon_preferences(worker,tab,&request)==XC_OK && wait(request)==XC_OK,"C queue continues after import failure");
  auto *unchanged=xc_request_take_lexicons(request); check(unchanged && std::string(xc_lexicons_greek(unchanged))=="DictionaryOne","C failed import retains state"); xc_lexicons_release(unchanged); xc_request_release(request);
  check(xc_worker_import_lexicon_profile(worker,tab,nullptr,&request)==XC_INVALID_ARGUMENT && !request,"null import path clears output");
  check(xc_worker_save_lexicon_profile(worker,tab,"",&request)==XC_INVALID_ARGUMENT && !request,"empty save path clears output");
  check(xc_worker_import_lexicon_profile(nullptr,tab,file.u8string().c_str(),&request)==XC_INVALID_ARGUMENT && !request,"null worker rejected");
  check(xc_worker_import_lexicon_profile(worker,tab,file.u8string().c_str(),nullptr)==XC_INVALID_ARGUMENT,"null observer output rejects without accepting operation");
  check(xc_worker_import_lexicon_profile(worker,tab,file.u8string().c_str(),&request)==XC_OK,"accepted unobserved import"); xc_request_release(request);
  const auto drain=root/"drained-save.xml"; std::filesystem::remove(drain);
  check(xc_worker_save_lexicon_profile(worker,tab,drain.u8string().c_str(),&request)==XC_OK,"accepted unobserved save"); xc_request_release(request);
  xc_worker_close(worker);
  check(read_study_profile(drain.u8string()).lexicons.greek=="DictionaryOne","dropped observers do not cancel accepted FIFO file work");
  check(std::string(xc_lexicons_greek(names))=="DictionaryOne","snapshot survives worker/request close"); xc_lexicons_release(names);
  check(previous_locale==std::setlocale(LC_ALL,nullptr),"imported saved locale never applied");
  std::cout<<"Profile worker/C API: "<<checks<<" checks passed\n";
 } catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
