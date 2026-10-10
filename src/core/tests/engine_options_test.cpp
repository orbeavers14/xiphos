// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/engine_options.hpp"
#include <swmgr.h>
#include <string>
#include <vector>
#include <utility>
#include <stdexcept>
#include <iostream>
struct GLOBAL_OPS {
  int strongs = 0;
  int morphs = 0;
  int footnotes = 0;
  int greekaccents = 0;
  int lemmas = 0;
  int scripturerefs = 0;
  int hebrewpoints = 0;
  int hebrewcant = 0;
  int italic_headings = 0;
  int words_in_red = 0;
  int xlit = 0;
  int enumerated = 0;
  int glosses = 0;
  int morphseg = 0;
  int transliteration = 0;
  int variants_primary = 0;
  int variants_secondary = 0;
  int variants_all = 0;
  int headings = 0; // Deliberately ignored by the engine, as in legacy Xiphos.
};
struct RecordingManager : sword::SWMgr {
  std::vector<std::pair<std::string, std::string>> calls;
  RecordingManager() : sword::SWMgr(nullptr, false, nullptr, false, false) {}
  void setGlobalOption(const char *name, const char *value) override {
    calls.emplace_back(name, value);
  }
};
struct Backend { RecordingManager manager; sword::SWMgr *get_mgr() { return &manager; } };
Backend storage;
Backend *backend = &storage;
void _set_global_option(const char *name, int value) {
  backend->get_mgr()->setGlobalOption(name, value ? "On" : "Off");
}
void _set_global_textual(const char *name, const char *value) {
  backend->get_mgr()->setGlobalOption(name, value);
}
#include "legacy_engine_options.inc"
#include "legacy_options_adapter.inc"
int main() {
  try {
    const std::vector<int GLOBAL_OPS::*> fields = {
&GLOBAL_OPS::strongs,&GLOBAL_OPS::morphs,&GLOBAL_OPS::footnotes,&GLOBAL_OPS::greekaccents,&GLOBAL_OPS::lemmas,&GLOBAL_OPS::scripturerefs,&GLOBAL_OPS::hebrewpoints,&GLOBAL_OPS::hebrewcant,&GLOBAL_OPS::italic_headings,&GLOBAL_OPS::words_in_red,&GLOBAL_OPS::xlit,&GLOBAL_OPS::enumerated,&GLOBAL_OPS::glosses,&GLOBAL_OPS::morphseg,&GLOBAL_OPS::transliteration};
    // Every filter independently, all on/off, and deterministic mixed states;
    // each combines all 8 textual-variant flag states and both display headings.
    for (unsigned sample = 0; sample < 150; ++sample) {
      unsigned bits = sample == 0 ? 0 : sample == 1 ? 0x7fff :
                      sample < 17 ? 1u << (sample - 2) : sample * 1103515245u;
      for (unsigned variants = 0; variants < 8; ++variants)
        for (int headings = 0; headings < 2; ++headings) {
          GLOBAL_OPS options;
          options.headings = headings;
          for (size_t i = 0; i < fields.size(); ++i)
            options.*fields[i] = (bits & (1u << i)) ? (i % 2 ? -7 : 3) : 0;
          options.variants_primary = variants & 1;
          options.variants_secondary = variants & 2;
          options.variants_all = variants & 4;
          storage.manager.calls.clear();
          legacy_set_global_options(&options);
          const auto expected = storage.manager.calls;
          storage.manager.calls.clear();
          main_set_global_options(&options);
          if (storage.manager.calls != expected)
            throw std::runtime_error("Legacy option adapter/filter sequence diverged");
          const auto &actual = storage.manager.calls;
          if (actual.size() != (variants ? 17u : 16u) ||
              actual[8] != std::make_pair(std::string("Headings"), std::string("On")))
            throw std::runtime_error("Headings or no-variant preservation changed");
        }
    }
    std::cout << "Legacy/shared engine option characterization passed (2400 states)\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
