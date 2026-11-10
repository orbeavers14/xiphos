// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include "xiphos/core/c_api.h"
#include "xiphos/core/session.hpp"
#include "xiphos/core/worker.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <stdexcept>
using namespace xiphos::core;
void check(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
static xc_status wait(xc_request *request) {
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
 xc_status status;
 while ((status = xc_request_poll(request)) == XC_PENDING) {
  check(std::chrono::steady_clock::now() < deadline, "Passage export timed out");
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
 }
 return status;
}
int main(int argc,char **argv) {
 try {
  check(argc==2,"generated reference fixture required");
  ParsedReferenceList retained;
  unsigned comparisons=0;
  {
   Reader reader(argv[1]); StudySession session(reader);
   (void)session.visit("OptionOther","John3:23");
   for(const auto *module:{"OptionTest","OptionOther"}) for(const auto *context:{"John3:16","Genesis1:1"})
    for(const auto *expression:{"John3:16","John3:16-19;John3:16","16-18;20","John3:36-4:2","Jude","","not-a-reference","Gen1:1-Rev22:21","Gen1:1-Gen50:26;Exod1:1-Exod40:38"}) {
     const auto targets=reader.reference_targets(module,expression,context);
     const auto full=reader.read_reference_list(module,expression,context);
     check(targets.context==full.context && targets.limited==full.limited && targets.skipped==full.skipped && targets.references.size()==full.entries.size(),"targets/full snapshots have matching context/order/guard metadata");
     for(std::size_t i=0;i<targets.references.size();++i) check(targets.references[i]==full.entries[i].reference,"each target matches existing rendered snapshot reference");
     if(std::string(expression)=="Gen1:1-Rev22:21" || std::string(expression)=="Gen1:1-Gen50:26;Exod1:1-Exod40:38") check(targets.limited&&targets.references.size()==2000,"existing large-range cap retained without body payloads");
     const auto html=display_reference_links(targets);
     check((html.find("xc-reference-limit")!=std::string::npos)==targets.limited && html.find("Back to "+targets.context)!=std::string::npos,
           "link-only preview discloses truncation and retains committed-context return link");
     ++comparisons;
    }
   EngineOptions enabled; enabled.strongs=true; enabled.footnotes=true;
   session.set_module_engine_options("OptionOther",enabled);
   retained=session.reference_targets("OptionOther","16-19;16","John3:23");
   session.set_module_engine_options("OptionOther",EngineOptions{});
   check(session.reference_targets("OptionOther","16-19;16","John3:23").references==retained.references,"targets do not depend on rendering flags");
   check(retained.references.size()==5 && retained.references.front()==retained.references.back() && session.history().entries().size()==1 && session.dictionary_history().entries().empty(),"owned target order/duplicates and no history visit");
   (void)reader.read("OptionOther","Genesis 1:1");
   const auto live = reader.navigation_target("OptionOther","2").reference;
   const auto exported = session.export_passage_text("OptionOther","16-19;16","John3:23");
   const auto first = exported.text.find("John 3:16\t");
   check(exported.verse_count==5 && exported.context=="John 3:23" && first!=std::string::npos &&
         exported.text.find("John 3:16\t",first+1)>exported.text.find("John 3:19\t") &&
         exported.text.find("John 3:16\t",first+1)!=std::string::npos,
         "Passage export retains provenance, range order and duplicate verses");
   check(session.export_passage_text("OptionOther","16-19;16","John3:23",exported.text.size()).text==exported.text,
         "Exact passage byte limit succeeds");
   bool limit=false;
   try { (void)session.export_passage_text("OptionOther","16-19;16","John3:23",exported.text.size()-1); }
   catch (const std::length_error &) { limit=true; }
   check(limit && reader.navigation_target("OptionOther","2").reference==live &&
         session.history().entries().size()==1,"Passage export and limit failure preserve SDK position and history");
   limit=false;
   try { (void)session.export_passage_text("OptionOther","Gen1:1-Rev22:21","John3:23"); }
   catch (const std::length_error &) { limit=true; }
   check(limit,"Passage export refuses a truncated range");
   const auto rich = session.export_passage_html("OptionOther","16-19;16","John3:23");
   const auto htmlFirst=rich.html.find("John 3:16");
   check(rich.verse_count==5 && rich.html.find("<!DOCTYPE html>")==0 &&
         rich.html.find("default-src 'none'")!=std::string::npos &&
         rich.html.find("John 3:16",rich.html.find("John 3:19"))!=std::string::npos &&
         htmlFirst!=std::string::npos && reader.navigation_target("OptionOther","2").reference==live,
         "Passage HTML is complete, passive, ordered and leaves the live key unchanged");
   check(session.export_passage_html("OptionOther","16-19;16","John3:23",rich.html.size()).html==rich.html,
         "Exact passage HTML cap succeeds");
   limit=false;
   try { (void)session.export_passage_html("OptionOther","16-19;16","John3:23",rich.html.size()-1); }
   catch (const std::length_error &) { limit=true; }
   check(limit,"One byte short refuses the complete passage HTML document");
   for(const auto &expression:{std::string("16\0ignored",10)}) {
    bool failed=false; try { (void)reader.reference_targets("OptionOther",expression,"John3:23"); } catch(const std::invalid_argument &) { failed=true; }
    check(failed,"embedded NUL expression rejected rather than truncated");
   }
   bool failed=false; try { (void)reader.reference_targets("OptionOther","16",""); } catch(const std::invalid_argument &) { failed=true; }
   check(failed,"empty target context rejected");
   failed=false; try { (void)reader.reference_targets("Missing","16","John3:23"); } catch(const std::out_of_range &) { failed=true; }
   check(failed,"missing target module rejected");
   (void)reader.read("OptionOther","John3:19");
   check(retained.references.size()==5,"targets survive later rendering");
  }
  check(retained.references.size()==5 && retained.context=="John 3:23","owned targets survive session/reader close");
  xc_worker *worker=nullptr;
  xc_request *request=nullptr;
  check(xc_worker_open(argv[1],&worker)==XC_OK &&
        xc_worker_create_tab(worker,&request)==XC_OK && wait(request)==XC_OK,"Open C passage export tab");
  const auto tab=xc_request_tab(request);
  xc_request_release(request);
  std::string expression="16;16";
  check(xc_worker_export_passage_text(worker,tab,"OptionOther",expression.c_str(),"John3:23",
        XC_PASSAGE_TEXT_EXPORT_MAX_BYTES,&request)==XC_OK,"Queue owned C passage export");
  expression.clear();
  check(wait(request)==XC_OK,"Complete C passage export");
  auto *owned=xc_request_take_passage_text_export(request);
  check(owned&&!xc_request_take_passage_text_export(request),"Passage export is take-once");
  xc_request_release(request);
  check(xc_worker_export_passage_text(worker,tab,"OptionOther","16","John3:23",1,&request)==XC_OK &&
        wait(request)!=XC_OK && !xc_request_take_passage_text_export(request),"Failed export publishes no partial text");
  xc_request_release(request);
  check(xc_worker_export_passage_html(worker,tab,"OptionOther","16;16","John3:23",
        XC_PASSAGE_TEXT_EXPORT_MAX_BYTES,&request)==XC_OK && wait(request)==XC_OK,"Queue and complete C passage HTML");
  auto *rich=xc_request_take_passage_html_export(request);
  check(rich&&!xc_request_take_passage_html_export(request),"Passage HTML result is take-once");
  xc_request_release(request);
  xc_worker_close(worker);
  check(std::string(xc_passage_text_export_expression(owned))=="16;16" &&
        std::string(xc_passage_text_export_module(owned))=="OptionOther" &&
        std::string(xc_passage_text_export_context(owned))=="John 3:23" &&
        xc_passage_text_export_verse_count(owned)==2 && xc_passage_text_export_size(owned)>0 &&
        std::string(xc_passage_text_export_text(owned)).find("John 3:16\t")!=std::string::npos,
        "C passage bytes and provenance survive worker shutdown");
  check(std::string(xc_passage_html_export_module(rich))=="OptionOther" &&
        std::string(xc_passage_html_export_context(rich))=="John 3:23" &&
        std::string(xc_passage_html_export_expression(rich))=="16;16" &&
        xc_passage_html_export_verse_count(rich)==2 && xc_passage_html_export_size(rich)>0 &&
        std::string(xc_passage_html_export_html(rich)).find("</body></html>")!=std::string::npos,
        "C passage HTML provenance and bytes survive worker shutdown");
  xc_passage_html_export_release(rich);
  xc_passage_text_export_release(owned);
  std::cout<<"Reference targets agree with existing rendered snapshots across "<<comparisons<<" module/context/expression cases\n";
 } catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
