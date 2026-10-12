// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/engine_manager.hpp"
#include <markupfiltmgr.h>
#include <utf8html.h>
#include <array>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
int main(int argc, char **argv) {
  try {
    check(argc == 2, "fixture path required");
    const std::string path = argv[1];
    std::array<std::string, 4> expected;
    std::string note_id, note_type, note_label, reference_id, reference_list;
    {
      EngineManager legacy(path.c_str(), true, new sword::MarkupFilterMgr(sword::FMT_XHTML), false, false);
      auto *module = legacy.getModule("OptionOther");
      check(module != nullptr, "load note fixture");
      for (unsigned bits = 0; bits < expected.size(); ++bits) {
        legacy.setGlobalOption("Strong's Numbers", bits & 1 ? "On" : "Off");
        legacy.setGlobalOption("Footnotes", bits & 2 ? "On" : "Off");
        module->setKey("John 3:19"); module->renderText();
        auto notes = module->getEntryAttributes()["Footnote"];
        check(notes.size() == 1, "one original study note");
        note_id = notes.begin()->first.c_str();
        auto values = notes.begin()->second;
        note_type = values["type"].c_str(); note_label = values["n"].c_str();
        // Freeze the existing backend's get_entry_attribute/render_this_text sequence.
        sword::SWBuf body(values["body"].c_str());
        sword::UTF8HTML conversion; conversion.processText(body);
        expected[bits] = module->renderText(body.c_str()).c_str();
      }
      check(expected[0] != expected[1] && expected[1].find("3056") != std::string::npos,
            "configured note body changes actual engine output");
      module->setKey("John 3:18"); module->renderText();
      const auto notes = module->getEntryAttributes()["Footnote"];
      check(notes.size() == 1, "one original cross reference");
      reference_id = notes.begin()->first.c_str();
      reference_list = notes.begin()->second.at("refList").c_str();
    }
    StudyNote owned;
    {
      Reader reader(path);
      for (unsigned bits = 0; bits < expected.size(); ++bits) {
        auto options = engine_options_from_preferences({});
        options.strongs = bits & 1; options.footnotes = bits & 2;
        reader.read_chapter("OptionOther", "John 3:18"); // Leaves attributes from a different verse.
        const auto note = reader.study_note("OptionOther", "John 3:19", note_id, options);
        check(note.html == expected[bits] && note.reference == "John 3:19" && note.identifier == note_id &&
              note.type == note_type && note.label == note_label, "note body/metadata matches legacy calls");
      }
      const auto reference = reader.study_note("OptionOther", "John 3:18", reference_id);
      check(reference.reference_list == reference_list && !reference_list.empty(), "cross-reference list copied");
      owned = reader.study_note("OptionOther", "John 3:19", note_id);
      const auto before = reader.read("OptionOther", "John 3:19").html;
      bool missing = false;
      try { reader.study_note("OptionOther", "John 3:19", "missing", {}); }
      catch (const std::invalid_argument &) { missing = true; }
      check(missing && reader.read("OptionOther", "John 3:19").html == before, "failed note restores engine options");
      missing = false;
      try { reader.study_note("OptionOther", "John 3:17", note_id); }
      catch (const std::invalid_argument &) { missing = true; }
      check(missing, "note lookup cannot reuse a previous verse's attributes");
      missing = false;
      try { reader.study_note("OptionOther", "John 3:19", ""); }
      catch (const std::invalid_argument &) { missing = true; }
      check(missing, "empty note identifier rejected");
      StudySession first(reader), second(reader);
      auto options = engine_options_from_preferences({}); options.strongs = true;
      first.set_module_engine_options("OptionOther", options);
      options.strongs = false; second.set_module_engine_options("OptionOther", options);
      first.visit("OptionOther", "John 3:19"); first.visit("OptionOther", "John 3:18");
      const auto count = first.history().entries().size(), cursor = first.history().cursor();
      check(first.study_note("OptionOther", "John 3:19", note_id).html == expected[3] &&
            second.study_note("OptionOther", "John 3:19", note_id).html == expected[2], "note settings isolated per tab");
      check(first.history().entries().size() == count && first.history().cursor() == cursor,
            "note preview does not move or add history");
      check(first.study_note("OptionTest", "John 3:16", "1").html.find("Original note.") != std::string::npos,
            "module argument selects the correct note");
    }
    check(owned.html.find("Original other note") != std::string::npos, "copied note outlives reader");
    std::future<StudyNote> accepted;
    {
      StudyWorker worker(path);
      const auto tab = worker.create_tab().get();
      worker.enable_module_preferences(tab).get();
      auto options = engine_options_from_preferences({}); options.strongs = true;
      worker.set_module_engine_options(tab, "OptionOther", options).get();
      std::string id = note_id;
      accepted = worker.study_note(tab, "OptionOther", "John 3:19", id); id = "missing";
      auto failure = worker.study_note(tab, "OptionOther", "John 3:17", note_id);
      bool rejected = false;
      try { (void)failure.get(); } catch (const std::invalid_argument &) { rejected = true; }
      check(rejected && worker.history(tab).get().entries.empty(), "worker note error preserves empty history and queue continues");
      worker.close_tab(tab).get(); rejected = false;
      try { (void)worker.study_note(tab, "OptionOther", "John 3:19", note_id).get(); }
      catch (const std::invalid_argument &) { rejected = true; }
      check(rejected, "closed tab rejects note request");
    }
    check(accepted.get().html == expected[3], "queued note copies arguments and outlives worker");
    std::cout << "Shared study-note legacy rendering, context, lifetime and isolation checks passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
