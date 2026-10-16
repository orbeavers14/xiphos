// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include "xiphos/core/preferences.hpp"
#include <rawtext.h>
#include <rawcom.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <glib.h>
using namespace xiphos::core;
struct GLOBAL_OPS { int image_content = 0, respect_font_faces = 0; bool xrefnotenumbers = true; };
struct FrozenBackend {
  std::string label;
  char *get_entry_attribute(const char *, const char *, const char *, bool) {
    return label.empty() ? nullptr : g_strdup(label.c_str());
  }
} probe;
FrozenBackend *backend = &probe;
int footnote = 0, xref = 0;
int main_save_module_options(const char *, const char *, int) { return 1; }
#define XI_message(args) ((void)0)
namespace frozen {
#include "legacy_html_cleanup.inc"
#include "legacy_content_cleanup.inc"
}
#undef XI_message
#undef CLEAR_FONT_NAME
#undef CLEAR_FONT_NAME_LENGTH
void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
// Independent complete legacy postprocessor, including its global counters and
// source-label suppression. Compare one SDK fragment at a time in source order.
std::string expected_piece(const std::string &html, const std::string &label, bool enabled) {
  probe.label = label; GLOBAL_OPS options; options.xrefnotenumbers = enabled;
  auto *text = frozen::CleanupContent(g_string_new(html.c_str()), &options, "Fixture", false);
  std::string result(text->str); g_string_free(text, true); return result;
}
void compare(const Entry &raw, const Entry &actual, bool enabled) {
  std::string headings;
  for (const auto &fragment : raw.note_source.headings)
    headings += expected_piece(fragment.html, fragment.reported_note_label, enabled);
  check(actual.heading_html == headings && actual.html == expected_piece(raw.html, raw.note_source.reported_label, enabled),
        "session output matches complete frozen heading/body cleanup and numbering");
  check(actual.note_source.captured && actual.note_source.reported_label == raw.note_source.reported_label &&
        actual.note_source.headings.size() == raw.note_source.headings.size(), "published raw source metadata remains owned");
  for (std::size_t i = 0; i < raw.note_source.headings.size(); ++i)
    check(actual.note_source.headings[i].html == raw.note_source.headings[i].html &&
          actual.note_source.headings[i].reported_note_label == raw.note_source.headings[i].reported_note_label,
          "presentation never overwrites raw heading sources");
}
void clean(StudySession &session, const char *module) {
  session.set_image_content(module, false); session.set_respect_font_faces(module, false);
}
int main(int argc, char **argv) {
  try {
    check(argc == 3, "source metadata fixture and private session fixture required");
    const std::filesystem::path source(argv[1]), root(argv[2]);
    std::filesystem::create_directories(root);
    std::filesystem::copy(source, root, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(root / "mods.d/modules-conf.cache");
    { std::ofstream config(root / "mods.d/text.conf", std::ios::app); config << "GlobalOptionFilter=ThMLStrongs\n"; }
    {
      sword::RawText writer((root / "text").string().c_str());
      writer.setKey("John4:5"); writer.setEntry("Original fifth <note>Original A.</note> <note>Original B.</note>");
      writer.setKey("John5:1"); writer.setEntry("<font face=\"Original Font\">Original styled word</font> <sync type=\"Strongs\" value=\"G3056\"/><note>Original note.</note>");
      writer.setKey("John5:2"); writer.setEntry("Original safe word <sync type=\"Strongs\" value=\"G3056\"/><note>Original second note.</note>");
      writer.setKey("John6:1"); writer.setEntry("<img src=\"*n.png\"/><font face=\"*x Original Font\">Original</font><note>Original note.</note>");
    }
    {
      sword::RawCom writer((root / "data").string().c_str());
      writer.setKey("John4:3"); writer.setEntry("Original OSIS <note type=\"study\">Original study.</note><note type=\"crossReference\">John4:1</note><note type=\"study\">Original second.</note>");
      writer.setKey("John4:4"); writer.setEntry("Original next <note type=\"crossReference\">John4:2</note>");
    }
    Entry held;
    {
      Reader reader(root.string()); EngineOptions engine; engine.footnotes = true;
      StudySession session(reader), other(reader), unknown(reader);
      session.set_engine_options(engine); other.set_engine_options(engine);
      clean(session, "Text"); clean(session, "ThMLNotes"); clean(session, "Notes");
      const auto baseline = session.read_entry("Text", "John4:1");
      check(!baseline.note_source.captured && !session.module_presentation_options("Text").note_numbering_available,
            "existing configured consumers retain unavailable note presentation");
      unknown.set_note_numbering_enabled(true);
      bool rejected = false;
      try { unknown.set_note_numbers("Text", false); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected && !unknown.module_presentation_options("Text").note_numbering_available &&
            !unknown.read_entry("Text", "John4:1").note_source.captured && unknown.history().entries().empty(),
            "capability never guesses unknown engine state or changes history on rejection");
      session.set_note_numbering_enabled(true);
      check(session.module_presentation_options("Text").note_numbers && session.module_presentation_options("Text").note_numbering_available,
            "configured opt-in uses the legacy default On");
      for (bool enabled : {true, false, true}) {
        session.set_note_numbers("Text", enabled); session.set_note_numbers("ThMLNotes", enabled);
        const auto raw_dense = reader.read_chapter("Text", "John4:3", {}, engine, enabled);
        const auto dense = session.read_chapter("Text", "John4:3");
        footnote = xref = 0;
        for (std::size_t i = 0; i < raw_dense.verses.size(); ++i) compare(raw_dense.verses[i], dense.verses.at(i), enabled);
        check(dense.verse_ordinals == raw_dense.verse_ordinals && dense.selected_anchor == "verse-3" &&
              dense.html.find("id=\"verse-3\"") != std::string::npos, "dense numbering preserves verse identity and selection");
        if (enabled) check(dense.verses.at(2).html.find("*n1") != std::string::npos &&
                           dense.verses.at(4).html.find("*n2") != std::string::npos && dense.verses.at(4).html.find("*n3") != std::string::npos,
                           "chapter counters continue past empty and labelled/linked source verses");
        const auto raw_sparse = reader.read_commentary_chapter("ThMLNotes", "John4:2", {}, engine, true, enabled);
        const auto sparse = session.read_commentary_chapter("ThMLNotes", "John4:2");
        footnote = xref = 0;
        for (std::size_t i = 0; i < raw_sparse.verses.size(); ++i) compare(raw_sparse.verses[i], sparse.verses.at(i), enabled);
        check(sparse.verse_ordinals == raw_sparse.verse_ordinals && sparse.selected_anchor == "commentary-verse-2",
              "sparse fragment cleanup retains original commentary ordinals");
        const auto raw_entry = reader.read("Text", "John4:5", engine, enabled);
        held = session.read_entry("Text", "John4:5"); footnote = xref = 0; compare(raw_entry, held, enabled);
        if (enabled) check(held.html.find("*n1") != std::string::npos && held.html.find("*n2") != std::string::npos,
                           "new entry documents reset counters rather than inheriting the chapter");
        const auto pair = session.read_passage("Text", "ThMLNotes", "John4:2");
        footnote = xref = 0; compare(reader.read("Text", "John4:2", engine, enabled), pair.text, enabled);
        footnote = xref = 0; compare(reader.read_commentary_entry("ThMLNotes", "John4:2", true, engine, enabled), *pair.commentary, enabled);
      }
      const auto raw_cross = reader.read_commentary_chapter("Notes", "John4:3", {}, engine, true, true);
      const auto cross = session.read_commentary_chapter("Notes", "John4:3");
      footnote = xref = 0;
      for (std::size_t i = 0; i < raw_cross.verses.size(); ++i) compare(raw_cross.verses[i], cross.verses.at(i), true);
      check(footnote == 2 && xref == 2, "independent footnote/cross-reference counters continue across sparse entries");
      check(reader.read("Text", "John4:1", engine).html == baseline.html,
            "session SDK label scopes restore existing Reader rendering");
      engine.strongs = true; session.set_engine_options(engine); session.set_lexical_layout_enabled(true);
      const auto lexical_notes = session.read_entry("Text", "John4:5");
      check(lexical_notes.lexical.requested && lexical_notes.lexical.applied && lexical_notes.html.find("*n2") != std::string::npos &&
            lexical_notes.html.find("action=showNote") != std::string::npos,
            "opted-in session combines automatic note counters with safe lexical HTML/CSS");
      const auto combined_headings = session.read_commentary_entry("Notes", "John4:1");
      check(combined_headings.lexical.applied && combined_headings.note_source.headings.size() == 1 &&
            combined_headings.note_source.headings[0].html.find("Original second heading") != std::string::npos &&
            combined_headings.heading_html.find(">second</span>") != std::string::npos &&
            combined_headings.heading_html.find("*na") != std::string::npos && combined_headings.html.find("*nbody") != std::string::npos,
            "multiple OSIS headings in one SDK fragment retain supplied labels and matching body layout");
      const auto distinct_headings = session.read_commentary_entry("ThMLNotes", "John4:2");
      check(distinct_headings.lexical.applied && distinct_headings.note_source.headings.size() == 2 &&
            distinct_headings.heading_html.find("*n1") != std::string::npos && distinct_headings.heading_html.find("*n2") != std::string::npos,
            "distinct SDK heading fragments retain sequential numbering while lexical layout is active");
      session.set_lexical_layout_enabled(false);
      const auto raw_cleanup = reader.read("Text", "John6:1", engine, true);
      const auto cleanup = session.read_entry("Text", "John6:1"); footnote = xref = 0; compare(raw_cleanup, cleanup, true);
      check(footnote == 2 && xref == 0 && cleanup.html.find("*n2") != std::string::npos,
            "cleanup removes image/font markers before counting; surviving SDK image-link URI retains legacy literal numbering");
      session.set_lexical_layout_enabled(true); engine.strongs = true; session.set_engine_options(engine);
      session.set_double_space("Text", true);
      const auto fallback = session.read_chapter("Text", "John5:2");
      check(fallback.lexical.requested && !fallback.lexical.applied && !fallback.lexical.css.empty() &&
            fallback.lexical.css.find("position: absolute") == std::string::npos &&
            fallback.verses.at(0).html.find("*n1") != std::string::npos && fallback.verses.at(1).html.find("*n2") != std::string::npos,
            "whole-document lexical rollback retains spacing CSS and ordered numbering");
      check(!other.read_entry("Text", "John4:1").note_source.captured && session.history().entries().empty(),
            "capability, reads and preference changes isolate tabs without adding visits");
      session.visit_passage("Text", "ThMLNotes", "John4:1"); session.visit_passage("Text", "ThMLNotes", "John4:2");
      session.set_note_numbers("Text", false);
      check(session.navigate_passage(-1).text.html.find("*nfirst") == std::string::npos && session.history().entries().size() == 2,
            "history rerenders current numbering policy without creating extra visits");
      const auto profile = (root / "profile.conf").string();
      { std::ofstream file(profile); file << "[Text]\nXrefNoteNumbers=On\nCustom=Keep\n[NotInstalled]\nXrefNoteNumbers=Keep\n"; }
      session.save_module_preferences(profile);
      auto values = read_module_option_values(profile);
      check(values.at("Text").at("XrefNoteNumbers") == "Off" && values.at("Text").at("Custom") == "Keep" &&
            values.at("NotInstalled").at("XrefNoteNumbers") == "Keep", "opt-in save updates only applicable installed note settings");
      StudySession restored(reader); restored.import_module_preferences(profile);
      check(!restored.module_presentation_options("Text").note_numbers && !restored.module_presentation_options("Text").note_numbering_available &&
            !restored.read_entry("Text", "John4:1").note_source.captured, "profile restores value without persisting migration capability");
      restored.set_note_numbering_enabled(true);
      check(restored.read_entry("Text", "John4:1").html.find("*nfirst") == std::string::npos, "imported Off applies after opt-in");
      bool failed = false;
      try { restored.import_module_preferences((root / "missing/profile.conf").string()); } catch (const std::exception &) { failed = true; }
      check(failed && !restored.module_presentation_options("Text").note_numbers, "failed import preserves applied note preferences");
      session.set_note_numbering_enabled(false); session.set_engine_options(EngineOptions{});
      check(!session.module_presentation_options("Text").note_numbering_available && !session.read_entry("Text", "John4:1").note_source.captured,
            "disabling capability removes availability and restores old Reader overloads");
      // Existing consumers must preserve a target file's unhandled note value.
      { std::ofstream file(profile); file << "[Text]\nXrefNoteNumbers=TargetKeep\n"; }
      session.save_module_preferences(profile);
      check(read_module_option_values(profile).at("Text").at("XrefNoteNumbers") == "TargetKeep",
            "non-opted-in save does not replace target note preferences");
    }
    check(held.note_source.captured && held.html.find("*n1") != std::string::npos,
          "numbered result and copied source survive Reader and session destruction");
    std::cout << "Session note snapshots match frozen cleanup, chapter/fragment order, isolation, fallback and selective persistence\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
