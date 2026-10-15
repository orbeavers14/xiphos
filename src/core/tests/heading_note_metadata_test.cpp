// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/headings.hpp"
#include "xiphos/core/entry_attribute.hpp"
#include "xiphos/core/note_rendering.hpp"
#include "xiphos/core/note_markers.hpp"
#include <rawcom.h>
#include <swmgr.h>
#include <swmodule.h>
#include <markupfiltmgr.h>
#include <filesystem>
#include <glib.h>
#include <utf8html.h>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
using namespace sword;
struct GLOBAL_OPS { bool strongs = false, lemmas = false, morphs = false; int image_content = 1, respect_font_faces = 1; bool xrefnotenumbers = true; };
namespace ModuleCache {
struct CacheVerse {
  std::string html;
  void SetHeader(const char *text) { html = text; }
  void AppendHeader(const char *text) { html += text; }
};
}
struct FrozenHeadingBackend { SWModule *display_mod; char *get_entry_attribute(const char *, const char *, const char *, bool = true); };
std::vector<HeadingFragment> observed;
FrozenHeadingBackend *observing = nullptr, *backend = nullptr;
int footnote = 0, xref = 0;
int main_save_module_options(const char *, const char *, int) { return 1; }
#define XI_message(args) ((void)0)
namespace frozen_cleanup {
#include "legacy_html_cleanup.inc"
#include "legacy_content_cleanup.inc"
}
#undef XI_message
#undef CLEAR_FONT_NAME
#undef CLEAR_FONT_NAME_LENGTH
const char *block_render(const char *) { throw std::runtime_error("Lexical formatting outside metadata capture seam"); }
GString *CleanupContent(GString *text, GLOBAL_OPS *options, const char *name, bool reset) {
  if (reset || !observing) throw std::runtime_error("Legacy heading cleanup probe contract changed");
  char *label = observing->get_entry_attribute("Footnote", "1", "n", false);
  observed.push_back({text->str, label ? label : ""}); std::free(label);
  return frozen_cleanup::CleanupContent(text, options, name, reset);
}
#include "legacy_heading_collection.inc"
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char **argv) {
  try {
    check(argc == 3, "shared heading and private note-fragment roots required");
    const std::filesystem::path source(argv[1]), root(argv[2]);
    std::filesystem::create_directories(root);
    std::filesystem::copy(source, root, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    { std::ofstream config(root / "mods.d/notes.conf", std::ios::app); config << "GlobalOptionFilter=OSISFootnotes\n"; }
    { std::ofstream config(root / "mods.d/thml.conf", std::ios::app); config << "GlobalOptionFilter=ThMLFootnotes\n"; }
    {
      sword::RawCom writer((root / "data").string().c_str());
      writer.setKey("John4:1"); writer.setEntry("<div subType=\"x-preverse\"><title>Original first heading <note n=\"a\" type=\"study\">Original heading note.</note></title><title>Original second heading.</title></div>Original body <note n=\"body\" type=\"study\">Original body note.</note>");
      writer.setKey("John4:2"); writer.setEntry("Original body without heading.");
    }
    {
      sword::RawCom writer((root / "thml").string().c_str());
      writer.setKey("John4:1"); writer.setEntry("<div class=\"sechead\">Original first heading <note n=\"a\">Original heading note.</note></div><div class=\"title\">Original second heading <note>Original unlabeled heading note.</note></div>Original body <note n=\"body\">Original body note.</note>");
      writer.setKey("John4:2"); writer.setEntry("<div class=\"sechead\">Original unlabeled heading <note>Original note.</note></div><div class=\"title\">Original UTF-8 heading <note n=\"β\">Original note.</note></div>Original second body.");
      writer.setKey("John4:3"); writer.setEntry("<div class=\"sechead\">Original heading without a note.</div>Original body <note n=\"β\">Original UTF-8 labelled body note.</note>");
      writer.setKey("John4:4"); writer.setEntry("Original body without headings.");
    }
    std::vector<HeadingFragment> retained;
    {
      sword::SWMgr manager(root.string().c_str(), true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
      manager.setGlobalOption("Headings", "On"); manager.setGlobalOption("Footnotes", "On");
      for (const auto *name : {"Notes", "ThMLNotes"}) {
        auto *module = manager.getModule(name); check(module, "load private heading-note module");
        for (bool enabled : {false, true}) for (const auto *reference : {"John4:1", "John4:2", "John4:3", "John4:4", "John4:1"}) {
          set_render_note_numbers(*module, enabled);
          module->setKey(reference); const std::string source_body = module->renderText().c_str();
          const auto source_label = entry_attribute(*module, "Footnote", "1", "n", false);
          const std::string key = module->getKeyText(); auto *identity = module->getKey();
          FrozenHeadingBackend probe{module}; observing = backend = &probe; observed.clear(); footnote = xref = 0;
          GLOBAL_OPS options; options.xrefnotenumbers = enabled; ModuleCache::CacheVerse legacy;
          frozen_cache_header(legacy, *module, &options, &probe); observing = nullptr;
          const auto expected = observed;
          GString *body = frozen_cleanup::CleanupContent(g_string_new(source_body.c_str()), &options, name, false);
          const std::string expected_body = body->str; g_string_free(body, true);
          const NoteNumberingState expected_counters(footnote, xref); backend = nullptr;
          const auto plain = render_preverse_headings(*module);
          const auto fragments = render_preverse_heading_fragments(*module);
          check(fragments.size() == plain.size() && fragments.size() == expected.size(), "rich and compatible heading APIs preserve SDK fragment boundaries");
          for (std::size_t i = 0; i < fragments.size(); ++i) {
            check(fragments[i].html == plain[i] && fragments[i].html == expected[i].html &&
                  fragments[i].reported_note_label == expected[i].reported_note_label,
                  "rich metadata copies the exact frozen legacy cleanup-point value without altering heading HTML");
          }
          NoteNumberingState counters; std::string numbered_headings;
          for (const auto &fragment : fragments) {
            auto numbered = number_note_markers(fragment.html, enabled, !fragment.reported_note_label.empty(), counters);
            numbered_headings += numbered.html; counters = numbered.counters;
          }
          const auto numbered_body = number_note_markers(source_body, enabled, !source_label.empty(), counters);
          check(numbered_headings == legacy.html && numbered_body.html == expected_body &&
                numbered_body.counters.footnotes == expected_counters.footnotes && numbered_body.counters.cross_references == expected_counters.cross_references,
                "owned metadata and counters reproduce frozen heading-then-body numbering without rereading SDK state");
          check(module->getKey() == identity && std::string(module->getKeyText()) == key &&
                entry_attribute(*module, "Footnote", "1", "n", false) == source_label,
                "normal collection restores source attributes and retains selected SDK key");
          if (std::string(name) == "ThMLNotes" && std::string(reference) == "John4:1") retained = fragments;
        }
      }
    }
    check(retained.size() == 2 && retained[0].html.find("Original first heading") != std::string::npos && retained[0].reported_note_label == "body" &&
          retained[1].reported_note_label == "body",
          "copied fragments remain owned after later renders and manager destruction");
    std::cout << "Owned heading fragments match frozen metadata, heading/body numbering, source restoration and lifetime\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
