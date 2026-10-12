// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
xc_status wait(xc_request *request) {
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
 xc_status status;
 while((status=xc_request_poll(request))==XC_PENDING) { check(std::chrono::steady_clock::now()<deadline,"hover source deadline"); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
 return status;
}
int main(int argc,char **argv) {
 try {
  check(argc==2,"generated note fixture required");
  const std::string prefix="passagestudy.jsp?action=showNote&type=n&value=1";
  unsigned comparisons=0;
  {
   StudyWorker worker(argv[1]); const auto tab=worker.create_tab().get(); worker.enable_module_preferences(tab).get();
   (void)worker.visit(tab,"OptionOther","John3:19").get();
   const auto expected=worker.study_note(tab,"OptionOther","John3:19","1").get();
   for(const auto *module:{"", "OptionOther", "MissingModule"}) for(const auto *passage:{"", "Jn3", "John", "John3:19"}) {
    const auto uri=prefix+"&module="+module+"&passage="+passage;
    const auto parsed=std::get<StudyNoteLink>(contextual_hover_link(uri,"OptionOther","John3:19"));
    check(parsed.reference=="John3:19","short decoded passage uses committed source before normalization");
    const auto note=std::get<StudyNote>(worker.hover_study_link_at(tab,"OptionOther","John3:19",uri).get());
    check(note.module==expected.module && note.reference==expected.reference && note.html==expected.html,"missing/unavailable module and short passage match characterized source rules"); ++comparisons;
   }
   const auto omitted=std::get<StudyNote>(worker.hover_study_link_at(tab,"OptionOther","John3:19",prefix).get());
   check(omitted.html==expected.html,"omitted module/passage use committed source");
   const auto short_raw=std::get<StudyNoteLink>(contextual_study_link(prefix+"&module=OptionOther&passage=Jn3","OptionOther"));
   check(short_raw.reference=="Jn3","existing raw contextual parser remains strict/no fallback");
   bool raw_empty_rejected=false; try { (void)study_note_link(prefix,"OptionOther"); } catch(const std::invalid_argument &) { raw_empty_rejected=true; }
   check(raw_empty_rejected,"existing independent note parser still requires explicit source passage");
   const auto explicit_source=std::get<StudyNote>(worker.hover_study_link_at(tab,"OptionOther","John3:19",prefix+"&module=OptionTest&passage=John3:16").get());
   check(explicit_source.module=="OptionTest" && explicit_source.reference=="John 3:16","installed explicit module/full passage are not overridden");
   const auto unicode=std::get<StudyNoteLink>(contextual_hover_link(prefix+"&module=OptionOther&passage=%C3%A9%C3%A9","OptionOther","John3:19"));
   check(unicode.reference=="John3:19","legacy short predicate counts decoded UTF-8 bytes");
   bool five_bytes_retained=false;
   const auto full=std::get<StudyNoteLink>(contextual_hover_link(prefix+"&module=OptionOther&passage=Jude1","OptionOther","John3:19")); five_bytes_retained=full.reference=="Jude1";
   check(five_bytes_retained,"five-byte passage threshold remains explicit");
   for(const auto *suffix:{"&passage=%00", "&passage=John3:16-19", "&passage=John3:16&passage=John3:19", "&unknown=1"}) {
    bool failed=false; try { (void)worker.hover_study_link_at(tab,"OptionOther","John3:19",prefix+suffix).get(); } catch(const std::invalid_argument &) { failed=true; }
    check(failed,"malformed/unsupported note inputs still reject before silent reinterpretation");
   }
   check(worker.history(tab).get().entries.size()==1 && worker.dictionary_history(tab).get().entries.empty(),"source fallbacks/errors do not visit histories");
  }
  xc_worker *worker=nullptr; xc_request *request=nullptr;
  check(xc_worker_open(argv[1],&worker)==XC_OK,"C hover source worker");
  check(xc_worker_create_tab(worker,&request)==XC_OK&&wait(request)==XC_OK,"C hover source tab"); const auto tab=xc_request_tab(request); xc_request_release(request);
  const auto uri=prefix+"&module=MissingModule&passage=Jn3";
  check(xc_worker_hover_study_link_at(worker,tab,"OptionOther","John3:19",uri.c_str(),&request)==XC_OK&&wait(request)==XC_OK,"C source fallback completion");
  auto *note=xc_request_take_note(request); check(note&&std::string(xc_note_module(note))=="OptionOther"&&std::string(xc_note_reference(note))=="John 3:19","C copied fallback source metadata");
  xc_request_release(request); xc_worker_close(worker);
  check(std::string(xc_note_html(note)).find("Original other note")!=std::string::npos,"fallback snapshot survives close"); xc_note_release(note);
  std::cout<<"Main hover source fallback checks passed across "<<comparisons<<" module/passage combinations\n";
 } catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
