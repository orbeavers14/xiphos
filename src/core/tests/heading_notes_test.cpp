// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/heading_notes.hpp"
#include "xiphos/core/headings.hpp"
#include "xiphos/core/entry_attribute.hpp"
#include "xiphos/core/note_rendering.hpp"
#include <swmgr.h>
#include <swmodule.h>
#include <swfilter.h>
#include <rawcom.h>
#include <markupfiltmgr.h>
#include <iostream>
#include <limits>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
struct Fault : sword::SWFilter {
  bool body;
  explicit Fault(bool value) : body(value) {}
  char processText(sword::SWBuf &text, const sword::SWKey *, const sword::SWModule *) override {
    if (std::string(text.c_str()).find(body ? "Original heading note." : "Original first heading") != std::string::npos)
      throw std::runtime_error("injected heading renderer failure");
    return 0;
  }
};
struct Callback : sword::SWFilter {
  std::function<void(sword::SWBuf &, const sword::SWModule &)> apply;
  explicit Callback(std::function<void(sword::SWBuf &, const sword::SWModule &)> value) : apply(std::move(value)) {}
  char processText(sword::SWBuf &text, const sword::SWKey *, const sword::SWModule *module) override {
    apply(text, *module); return 0;
  }
};
struct Attached {
  sword::SWModule &module; sword::SWFilter &filter;
  Attached(sword::SWModule &m, sword::SWFilter &f) : module(m), filter(f) { module.addRenderFilter(&filter); }
  ~Attached() { module.removeRenderFilter(&filter); }
};
int main(int argc, char **argv) {
  try {
    check(argc == 3, "original-prose source and private capture fixture required");
    const std::filesystem::path root(argv[2]);
    std::filesystem::create_directories(root);
    std::filesystem::copy(argv[1], root, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    {
      std::ofstream file(root / "mods.d/reordered.conf");
      file << "[OSISBefore]\nDataPath=./data/\nModDrv=RawCom\nSourceType=OSIS\nEncoding=UTF-8\nGlobalOptionFilter=OSISFootnotes\nGlobalOptionFilter=OSISHeadings\n"
           << "[ThMLBefore]\nDataPath=./thml/\nModDrv=RawCom\nSourceType=ThML\nEncoding=UTF-8\nGlobalOptionFilter=ThMLFootnotes\nGlobalOptionFilter=ThMLHeadings\n";
    }
    {
      sword::RawCom writer((root / "data").string().c_str());
      writer.setKey("John5:1");
      writer.setEntry("<div subType=\"x-preverse\"><title>Original mixed heading <note n=\"a\" type=\"study\">Original study heading note.</note><note n=\"x\" type=\"crossReference\"><reference osisRef=\"John.4.1\">John4:1</reference></note><note type=\"study\"></note></title></div>Original body <note n=\"body\" type=\"study\">Original body note.</note>");
    }
    HeadingNoteSource held;
    unsigned cases = 0;
    {
      sword::SWMgr manager(root.string().c_str(), true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
      manager.setGlobalOption("Headings", "On");
      for (const auto *name : {"Notes", "ThMLNotes"}) {
        auto &module = *manager.getModule(name);
        for (bool labels : {false, true}) for (bool footnotes : {false, true}) {
          manager.setGlobalOption("Footnotes", footnotes ? "On" : "Off");
          set_render_note_numbers(module, labels);
          for (std::size_t fragment = 0; fragment < (std::string(name) == "Notes" ? 1u : 2u); ++fragment) {
            module.setKey("John4:1"); module.renderText();
            const auto original = module.getEntryAttributes();
            const auto *identity = module.getKey(); const std::string reference = module.getKeyText();
            const auto source = capture_preverse_heading_notes(module, fragment);
            check(source.notes.size() == 1 && source.notes.count("1") == 1,
                  "isolated SDK fragment numbering starts locally and excludes source/body bookkeeping");
            const auto &note = source.notes.at("1");
            check(note.module == name && note.reference == reference && note.identifier == "1" &&
                  note.label == (fragment ? "" : "a") && note.html.find(fragment ? "Original unlabeled heading note." : "Original heading note.") != std::string::npos,
                  "owned fragment note body/metadata belong to the heading, never the colliding body note");
            check(source.html.find("value=&") == std::string::npos &&
                  (!footnotes || source.html.find("value=1&") != std::string::npos),
                  "option filtering supplies a real local SDK marker identifier before heading rendering");
            check(source.reported_note_label == entry_attribute(module, "Footnote", "1", "n", false),
                  "copied cleanup-point label retains legacy suppression independently of heading labels");
            check(module.getEntryAttributes() == original && module.getKey() == identity && std::string(module.getKeyText()) == reference,
                  "success restores all refreshed source attributes and key identity");
            const auto metadata = capture_preverse_heading_notes(module, fragment, false);
            check(metadata.html == source.html && metadata.notes.at("1").html.empty() && metadata.notes.at("1").label == note.label &&
                  module.getEntryAttributes() == original, "metadata-only capture avoids note-body rendering and restores source state");
            const auto body = render_study_note(module, reference, "1");
            check(body.html.find("Original body note.") != std::string::npos && body.html.find("Original heading note.") == std::string::npos,
                  "ordinary note lookup still resolves the original body namespace after a heading capture");
            held = source; ++cases;
          }
        }
        manager.setGlobalOption("Footnotes", "On"); module.setKey("John4:1"); module.renderText();
        const auto original = module.getEntryAttributes();
        for (bool body : {false, true}) {
          Fault fault(body); Attached attached(module, fault); bool failed = false;
          if (body) check(capture_preverse_heading_notes(module, 0, false).notes.at("1").html.empty(),
                          "metadata-only capture never invokes a note-body renderer");
          try { capture_preverse_heading_notes(module, 0); } catch (const std::runtime_error &) { failed = true; }
          check(failed && module.getEntryAttributes() == original && module.isProcessEntryAttributes(),
                "heading-render and note-body exceptions restore complete source attributes");
        }
        module.setProcessEntryAttributes(false);
        bool disabled = false;
        try { capture_preverse_heading_notes(module, 0); } catch (const std::invalid_argument &) { disabled = true; }
        check(disabled && !module.isProcessEntryAttributes() && module.getEntryAttributes() == original,
              "disabled attribute processing rejects without guessing state or changing source metadata");
        module.setProcessEntryAttributes(true);
        module.getEntryAttributes()["Probe"]["Sentinel"]["value"] = "Original sentinel";
        const auto incoming = module.getEntryAttributes();
        {
          Callback failure([](sword::SWBuf &text, const sword::SWModule &) {
            if (std::string(text.c_str()).find("Original body ") != std::string::npos) throw std::runtime_error("initial source render failure");
          });
          Attached attached(module, failure); bool failed = false;
          try { capture_preverse_heading_notes(module, 0); } catch (const std::runtime_error &) { failed = true; }
          check(failed && module.isProcessEntryAttributes() && module.getEntryAttributes() == incoming,
                "initial source-refresh exceptions restore incoming attributes and processing state");
        }
        module.renderText();
        for (const auto &injection : {std::string("<title unfinished"),
             std::string("<title><note swordFootnote=\"9\">Original</note><note>Other</note></title>"),
             std::string("<title><note swordFootnote=\"999\">Original</note></title>"), std::string("Original\0heading", 16)}) {
          Callback inject([&](sword::SWBuf &, const sword::SWModule &owner) {
            auto &value = owner.getEntryAttributes()["Heading"]["Preverse"]["0"];
            value.resize(injection.size());
            for (std::size_t i = 0; i < injection.size(); ++i) value[i] = injection[i];
          });
          Attached attached(module, inject); bool failed = false;
          try { capture_preverse_heading_notes(module, 0); } catch (const std::invalid_argument &) { failed = true; }
          check(failed && module.isProcessEntryAttributes(),
                "malformed, mixed, unresolved and NUL heading inputs reject with restored SDK processing");
        }
        module.renderText();
        bool missing = false;
        try { capture_preverse_heading_notes(module, std::numeric_limits<std::size_t>::max()); } catch (const std::out_of_range &) { missing = true; }
        check(missing && module.getEntryAttributes() == original, "absent fragment rejects without a scan or partial attribute mutation");
        const auto legacy = render_preverse_heading_fragments(module);
        check(!legacy.empty() && legacy.front().html.find("value=&") != std::string::npos,
              "existing legacy heading collection retains its exact unmodified path");
        module.setKey("John4:4"); missing = false;
        try { capture_preverse_heading_notes(module, 0); } catch (const std::out_of_range &) { missing = true; }
        check(missing, "headingless entry does not reuse a prior source fragment");
      }
      // Reversed SDK option-filter order must retain existing IDs/bodies and
      // exclude the verse body notes from the captured heading namespace.
      manager.setGlobalOption("Footnotes", "On");
      for (const auto *name : {"OSISBefore", "ThMLBefore"}) {
        auto &module = *manager.getModule(name);
        for (std::size_t fragment = 0; fragment < (std::string(name) == "OSISBefore" ? 1u : 2u); ++fragment) {
          module.setKey("John4:1"); module.renderText(); const auto original = module.getEntryAttributes();
          const auto source = capture_preverse_heading_notes(module, fragment);
          check(source.notes.size() == 1 && source.html.find("value=&") == std::string::npos &&
                source.notes.begin()->second.html.find(fragment ? "Original unlabeled heading note." : "Original heading note.") != std::string::npos &&
                module.getEntryAttributes() == original,
                "already-filtered heading IDs retain their original bodies without importing source/body note collisions");
          ++cases;
        }
      }
      for (const auto *name : {"Notes", "OSISBefore"}) {
        auto &module = *manager.getModule(name); module.setKey("John5:1"); module.renderText();
        const auto original = module.getEntryAttributes();
        const auto source = capture_preverse_heading_notes(module, 0);
        check(source.notes.size() == 3 && source.notes.at("1").html.find("Original study heading note.") != std::string::npos &&
              source.notes.at("2").type == "crossReference" && source.notes.at("2").reference_list.find("John.4.1") != std::string::npos &&
              source.notes.at("3").type == "study" && source.notes.at("3").html.empty() && module.getEntryAttributes() == original,
              "mixed heading study/cross-reference notes retain lists and distinguish empty body from unavailable metadata");
        ++cases;
      }
      auto &thml = *manager.getModule("ThMLNotes"); thml.setKey("John4:2");
      const auto utf8 = capture_preverse_heading_notes(thml, 1);
      check(utf8.notes.at("1").label == "β" && utf8.notes.at("1").html.find("Original note.") != std::string::npos,
            "Unicode heading labels remain copied metadata while HTML uses SDK encoding");
    }
    check(held.notes.at("1").html.find("Original unlabeled heading note.") != std::string::npos,
          "owned heading source and bodies outlive later renders and manager destruction");
    std::cout << "Isolated heading-note capture, source restoration and ownership passed: " << cases << " cases\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
