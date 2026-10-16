// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include "xiphos/core/engine_manager.hpp"
#include "book_tree_source.hpp"
#include <rawgenbook.h>
#include <rawld.h>
#include <markupfiltmgr.h>
#include <swfilter.h>
#include <swoptfilter.h>
#include <treekeyidx.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace xiphos::core;
namespace fs = std::filesystem;
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
template<class Error,class F> void rejects(F call) {
  try { call(); } catch (const Error &) { return; }
  throw std::runtime_error("Expected typed book-entry error");
}
std::map<std::string,std::string> bytes(const fs::path &root) {
  std::map<std::string,std::string> result;
  for (const auto &item : fs::recursive_directory_iterator(root)) if (item.is_regular_file()) {
    std::ifstream input(item.path(),std::ios::binary); check(bool(input),"Read module bytes");
    result.emplace(fs::relative(item.path(),root).generic_string(),std::string(std::istreambuf_iterator<char>(input),{}));
  }
  return result;
}
struct Fault : sword::SWFilter {
  char processText(sword::SWBuf &,const sword::SWKey *,const sword::SWModule *) override {
    throw std::runtime_error("injected book render failure");
  }
};
struct MoveKey : sword::SWFilter {
  char processText(sword::SWBuf &,const sword::SWKey *,const sword::SWModule *module) override {
    dynamic_cast<sword::TreeKeyIdx &>(*module->getKey()).setOffset(8);
    return 0;
  }
};
int main(int argc,char **argv) {
  try {
    check(argc==2,"Original-prose fixture root required"); const fs::path root(argv[1]);
    fs::create_directories(root/"mods.d"); fs::create_directories(root/"data");
    for (const auto *id : {"OSISBook","ThMLBook"}) {
      const auto prefix=(root/"data"/id).string();
      check(sword::RawGenBook::createModule(prefix.c_str())==0,"Create original book");
      {
        sword::RawGenBook writer(prefix.c_str());
        writer.setKey("/Part One");
        writer.setEntry(std::string(id)=="OSISBook" ?
          "<p>Original café <q who=\"Jesus\">colored word</q><note n=\"a\">Original book note.</note></p>" :
          "<p>Original café <i>emphasis</i><note n=\"a\">Original book note.</note><img src=\"local.png\" /></p>");
        writer.setKey("/Part One/Leaf"); writer.setEntry("Original leaf prose.");
        writer.setKey("/Empty Branch/Leaf"); writer.setEntry("Original branch leaf.");
        std::unique_ptr<sword::SWKey> linked(writer.createKey());
        dynamic_cast<sword::TreeKeyIdx &>(*linked).setOffset(4);
        writer.setKey("/Linked"); writer.linkEntry(linked.get());
      }
      std::ofstream conf(root/"mods.d"/(std::string(id)+".conf"));
      const bool osis=std::string(id)=="OSISBook";
      conf << '[' << id << "]\nDataPath=./data/" << id << "\nModDrv=RawGenBook\nEncoding=UTF-8\nSourceType=" << (osis?"OSIS":"ThML")
           << "\nDescription=Original book rendering fixture\nGlobalOptionFilter=" << (osis?"OSISFootnotes":"ThMLFootnotes") << '\n';
      if (osis) conf << "GlobalOptionFilter=OSISRedLetterWords\n";
    }
    check(sword::RawLD::createModule((root/"data/dictionary").string().c_str())==0,"Create non-book fixture");
    { std::ofstream conf(root/"mods.d/dictionary.conf"); conf << "[Dictionary]\nDataPath=./data/dictionary\nModDrv=RawLD\n"; }
    { std::ofstream conf(root/"mods.d/verse-book.conf"); conf << "[VerseBook]\nDataPath=./data/OSISBook\nModDrv=RawGenBook\nKeyType=VerseKey\n"; }
    fs::remove(root/"mods.d/modules-conf.cache");
    const auto before=bytes(root/"data");
    BookEntry retained;
    {
      Reader reader(root.string());
      EngineManager legacy(root.string().c_str(),true,new sword::MarkupFilterMgr(sword::FMT_XHTML),false,false);
      std::map<std::string,std::string> baseline;
      for (const auto *id : {"OSISBook","ThMLBook"}) {
        auto &module=*legacy.getModule(id);
        auto &key=dynamic_cast<sword::TreeKeyIdx &>(*module.getKey());
        for (bool notes : {false,true}) for (bool red : {false,true}) {
          EngineOptions options; options.footnotes=notes; options.words_in_red=red;
          for (unsigned offset : {0u,4u,8u,12u,16u,20u}) {
            apply_engine_options(legacy,options);
            key.setOffset(offset);
            const std::string expected=module.renderText().c_str(); // frozen legacy live tree render
            const auto result=reader.read_book(id,offset,options);
            check(result.tree.module==id && result.tree.node.offset==offset,"Requested book and exact node identity");
            check(result.html==expected,"Configured book HTML matches frozen SDK output exactly");
            check(result.tree.node.path==key.getText(),"Rendered HTML paired with requested canonical path");
            check(result.has_entry==module.hasEntry(&key),"SDK entry presence, including structural nodes");
            if (offset==4 && !notes && !red) baseline[id]=result.html;
            if (offset==4) check((result.html.find("action=showNote")!=std::string::npos)==notes,"Configured SDK book note-marker visibility");
            if (offset==4 && !notes && red && std::string(id)=="OSISBook")
              check(result.html!=baseline[id],"Red-letter option changes the real SDK book output");
            if (offset==20) check(result.tree.node.path=="/Linked" && result.html.find("Original café")!=std::string::npos,"Linked content keeps alias node rather than target identity");
            if (offset==12) check(result.html.empty() && !result.has_entry,"Structural-only branch renders empty rather than nearby prose");
            if (offset==0) check(result.tree.node.path.empty() && result.html.empty() && !result.has_entry,"Root is not silently clamped to first entry");
          }
        }
        check(reader.read_book(id,4,{}).html==baseline[id],"Configured reads do not inherit preceding option choices");
        const auto *identity=module.getKey();
        for (bool processing : {false,true}) {
          module.setProcessEntryAttributes(processing);
          const auto saved_filters=legacy.option_filters();
          std::vector<std::string> saved;
          for (auto *filter : saved_filters) saved.emplace_back(filter->getOptionValue());
          Fault fault; module.addRenderFilter(&fault);
          rejects<std::runtime_error>([&]{
            EngineOptions options; options.footnotes=true; options.words_in_red=true;
            ScopedEngineOptions scope(legacy,options);
            detail::capture_book_entry(module,4);
          });
          module.removeRenderFilter(&fault);
          check(module.isProcessEntryAttributes()==processing && module.getKey()==identity && key.getOffset()==4,"Render failure preserves key identity/requested selection and processing state");
          for (std::size_t i=0;i<saved.size();++i) check(saved_filters[i]->getOptionValue()==saved[i],"Render failure restores every shared engine filter");
          check(detail::capture_book_entry(module,8).html=="Original leaf prose.","Read recovers after injected render failure");
          check(module.isProcessEntryAttributes()==processing,"Healthy book render preserves initial processing flag");
        }
        MoveKey move; module.addRenderFilter(&move);
        rejects<std::runtime_error>([&]{detail::capture_book_entry(module,4);});
        module.removeRenderFilter(&move);
        check(detail::capture_book_entry(module,4).tree.node.offset==4,"Changed render source rejects and later read recovers");
        key.setOffset(8);
        rejects<std::out_of_range>([&]{detail::capture_book_entry(module,40000);});
        check(key.getOffset()==8,"Invalid source rejected before changing live selection");
      }
      rejects<std::invalid_argument>([&]{reader.read_book("Dictionary",0,{});});
      rejects<std::out_of_range>([&]{reader.read_book("Missing",0,{});});
      rejects<std::invalid_argument>([&]{reader.read_book(std::string("OSISBook\0bad",12),4,{});});
      rejects<std::invalid_argument>([&]{reader.read_book("OSISBook",1,{});});
      rejects<std::out_of_range>([&]{reader.read_book("OSISBook",40000,{});});
      retained=reader.read_book("OSISBook",4,{});
      check(reader.read_book("OSISBook",8,{}).html=="Original leaf prose.","Later read has independent content");
    }
    check(retained.tree.node.path=="/Part One" && retained.html.find("Original café")!=std::string::npos && retained.tree.first_child->path=="/Part One/Leaf","Book entry/source ownership survives reader destruction");
    check(bytes(root/"data")==before,"All module data bytes and filenames survive rendering, links and failures");
    std::cout << "Configured book HTML/source, linked/empty entries, failure recovery and module-byte checks passed\n";
    return 0;
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
