// SPDX-License-Identifier: GPL-2.0-or-later
#include "sdk_render.hpp"
#include "xiphos/core/render.hpp"
#include "xiphos/core/headings.hpp"
#include "xiphos/core/study_note.hpp"
#include "xiphos/core/entry_attribute.hpp"
#include <swmgr.h>
#include <swmodule.h>
#include <swfilter.h>
#include <markupfiltmgr.h>
#include <iostream>
#include <stdexcept>
#include <utility>
using namespace xiphos::core;
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
struct Fault : sword::SWFilter {
  std::string needle;
  explicit Fault(std::string value) : needle(std::move(value)) {}
  char processText(sword::SWBuf &text, const sword::SWKey *, const sword::SWModule *) override {
    if (std::string(text.c_str()).find(needle) != std::string::npos) throw std::runtime_error("injected SDK render failure");
    return 0;
  }
};
struct Attached {
  sword::SWModule &module; sword::SWFilter &filter;
  Attached(sword::SWModule &m, sword::SWFilter &f) : module(m), filter(f) { module.addRenderFilter(&filter); }
  ~Attached() { module.removeRenderFilter(&filter); }
};
int main(int argc, char **argv) {
  try {
    check(argc == 2, "original-prose heading-note fixture required");
    sword::SWBuf held; unsigned cases = 0;
    {
      sword::SWMgr manager(argv[1], true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
      manager.setGlobalOption("Headings", "On"); manager.setGlobalOption("Footnotes", "On");
      for (const auto *name : {"Notes", "ThMLNotes"}) {
        auto &module = *manager.getModule(name); module.setKey("John4:1");
        for (bool processing : {false, true}) {
          module.setProcessEntryAttributes(processing);
          const std::string expected_entry = module.renderText().c_str();
          module.setProcessEntryAttributes(processing);
          const std::string expected_buffer = module.renderText("Original healthy buffer.").c_str();
          module.setProcessEntryAttributes(processing);
          const auto *key = module.getKey();
          check(std::string(detail::render_sdk_entry(module).c_str()) == expected_entry && module.isProcessEntryAttributes() == processing &&
                std::string(detail::render_sdk_buffer(module, "Original healthy buffer.").c_str()) == expected_buffer &&
                module.isProcessEntryAttributes() == processing && module.getKey() == key,
                "SDK boundary keeps exact stored/buffer output, key identity and both initial processing states");
          {
            Fault fault("Original"); Attached attached(module, fault);
            bool source_failed = false, buffer_failed = false;
            try { detail::render_sdk_entry(module); } catch (const std::runtime_error &) { source_failed = true; }
            check(module.isProcessEntryAttributes() == processing, "stored-render failure restores caller processing state");
            try { detail::render_sdk_buffer(module, "Original fault buffer."); } catch (const std::runtime_error &) { buffer_failed = true; }
            check(source_failed && buffer_failed && module.isProcessEntryAttributes() == processing && module.getKey() == key,
                  "buffer-render failure cannot leave SDK attribute processing disabled or change the selected key");
          }
          held = detail::render_sdk_buffer(module, "Original owned buffer."); ++cases;
        }
        module.setProcessEntryAttributes(true);
        for (int operation = 0; operation < 4; ++operation) {
          const std::string needle = operation == 2 ? "Original first heading" : operation == 3 ? "Original body note." : "Original body ";
          {
            Fault fault(needle); Attached attached(module, fault); bool failed = false;
            try {
              if (operation == 0) render_entry(module, "John4:1");
              if (operation == 1) entry_attribute(module, "Footnote", "1", "body", true);
              if (operation == 2) render_preverse_heading_fragments(module);
              if (operation == 3) render_study_note(module, "John4:1", "1");
            } catch (const std::runtime_error &) { failed = true; }
            check(failed && module.isProcessEntryAttributes(),
                  "public render/attribute/heading/note helpers restore SDK processing after a filter exception");
            if (operation == 3) check(read_study_note_metadata(module, "John4:1", "1").html.empty(),
                                      "metadata-only note access still avoids the failing body renderer");
          }
          check(!render_preverse_heading_fragments(module).empty() &&
                render_study_note(module, "John4:1", "1").html.find("Original body note.") != std::string::npos &&
                entry_attribute(module, "Footnote", "1", "n", true) == "body" && module.isProcessEntryAttributes(),
                "later heading/note/attribute reads recover completely instead of inheriting stale disabled processing");
          ++cases;
        }
      }
    }
    check(std::string(held.c_str()) == "Original owned buffer.", "SDK return buffer is owned after later failures and manager destruction");
    std::cout << "SDK render processing recovery and unchanged output passed: " << cases << " cases\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
