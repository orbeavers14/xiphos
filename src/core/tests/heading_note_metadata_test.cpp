// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/headings.hpp"
#include "xiphos/core/reader.hpp"
#include <algorithm>
#include "xiphos/core/entry_attribute.hpp"
#include "xiphos/core/note_rendering.hpp"
#include "xiphos/core/note_markers.hpp"
#include <rawcom.h>
#include <rawtext.h>
#include <versekey.h>
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
    { std::ofstream config(root / "mods.d/text.conf", std::ios::app); config << "GlobalOptionFilter=ThMLFootnotes\n"; }
    {
      sword::RawText writer((root / "text").string().c_str());
      writer.setKey("John4:1"); writer.setEntry("Original first Bible <note n=\"first\">Original first note.</note>");
      writer.setKey("John4:2"); writer.setEntry("Original second Bible <note n=\"second\">Original second note.</note>");
      writer.setKey("John4:3"); writer.setEntry("Original unlabeled Bible <note>Original note.</note>");
      sword::VerseKey source_key("John4:1"); writer.setKey("John4:4"); writer.linkEntry(&source_key);
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
    Entry held_entry; Chapter held_sparse, held_dense;
    {
      Reader reader(root.string()); EngineOptions engine; engine.footnotes = true;
      const auto bible_baseline = reader.read("Text", "John4:1", engine);
      const auto baseline = reader.read_commentary_entry("ThMLNotes", "John4:1", true, engine);
      check(!baseline.note_source.captured && baseline.note_source.headings.empty(), "existing Reader overload retains unavailable metadata state");
      held_entry = reader.read_commentary_entry("ThMLNotes", "John4:1", true, engine, true);
      check(held_entry.note_source.captured && held_entry.note_source.reported_label == "body" &&
            held_entry.note_source.headings.size() == 2 && held_entry.note_source.headings[0].reported_note_label == "body" &&
            held_entry.html.find("*nbody") != std::string::npos && held_entry.heading_html.find("*na") != std::string::npos,
            "opt-in Reader copies body and heading metadata while scoped SDK labels are enabled");
      const auto disabled = reader.read_commentary_entry("ThMLNotes", "John4:1", true, engine, false);
      check(disabled.note_source.captured && disabled.note_source.reported_label == "body" && disabled.html == baseline.html &&
            disabled.heading_html == baseline.heading_html,
            "explicit label Off remains captured and preserves the baseline rendering");
      const auto after = reader.read_commentary_entry("ThMLNotes", "John4:1", true, engine);
      check(after.html == baseline.html && after.heading_html == baseline.heading_html && !after.note_source.captured,
            "opt-in Reader restores renderer state for existing consumers");
      const auto body_only = reader.read("ThMLNotes", "John4:1", engine, true);
      check(body_only.note_source.captured && body_only.note_source.reported_label == "body" && body_only.note_source.headings.empty(),
            "opt-in entry read captures the body without inventing headings");
      held_sparse = reader.read_commentary_chapter("ThMLNotes", "John4:2", DisplayOptions{}, engine, true, true);
      const auto first = std::find(held_sparse.verse_ordinals.begin(), held_sparse.verse_ordinals.end(), 1);
      check(first != held_sparse.verse_ordinals.end(), "sparse chapter retains SDK verse positions");
      const auto offset = static_cast<std::size_t>(first - held_sparse.verse_ordinals.begin());
      check(held_sparse.verses[offset].note_source.captured && held_sparse.verses[offset].note_source.reported_label == "body" &&
            held_sparse.verses[offset].note_source.headings.size() == 2 && held_sparse.selected_anchor == "commentary-verse-2",
            "sparse Reader snapshots copy each source before later SDK entries replace attributes");
      const auto no_headings = reader.read_commentary_chapter("ThMLNotes", "John4:2", DisplayOptions{}, engine, false, true);
      check(no_headings.verses[offset].note_source.captured && no_headings.verses[offset].note_source.headings.empty() &&
            no_headings.verses[offset].heading_html.empty(), "heading-disabled note capture preserves eligibility");
      held_dense = reader.read_chapter("Text", "John4:2", DisplayOptions{}, engine, true);
      check(!held_dense.verses.empty() && std::all_of(held_dense.verses.begin(), held_dense.verses.end(), [](const Entry &entry) { return entry.note_source.captured; }),
            "dense Reader snapshots distinguish captured empty metadata from unavailable state");
      check(held_dense.verses.size() >= 4 && held_dense.verses[0].note_source.reported_label == "first" &&
            held_dense.verses[1].note_source.reported_label == "second" && held_dense.verses[2].note_source.reported_label.empty() &&
            held_dense.verses[3].note_source.reported_label == "first" && held_dense.verse_ordinals[3] == 4 &&
            held_dense.verses[3].reference.find("4:4") != std::string::npos && held_dense.selected_anchor == "verse-2",
            "dense metadata stays with each distinct/empty/linked SDK verse instead of the last rendered entry");
      check(reader.read("Text", "John4:1", engine).html == bible_baseline.html,
            "shared Bible/commentary render filter restores the existing Bible consumer");
      reader.read_commentary_entry("Notes", "John4:1", true, engine, true);
      check(reader.read_commentary_entry("ThMLNotes", "John4:1", true, engine).html == baseline.html,
            "separate markup renderer scopes preserve prior module behavior");
      bool wrong_type = false;
      try { reader.read_commentary_entry("Text", "John4:1", true, engine, true); } catch (const std::invalid_argument &) { wrong_type = true; }
      check(wrong_type && reader.read_commentary_entry("ThMLNotes", "John4:1", true, engine).html == baseline.html,
            "failed opt-in request retains subsequent Reader state");
    }
    check(held_entry.note_source.captured && held_entry.note_source.headings[0].reported_note_label == "body" &&
          !held_sparse.verses.empty() && held_sparse.verses[0].note_source.captured && held_dense.verses[0].note_source.captured,
          "owned Reader metadata outlives renderer state and manager destruction");
    std::cout << "Owned heading fragments match frozen metadata, heading/body numbering, source restoration and lifetime\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
