// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/c_api.h"
#include <chrono>
#include <thread>
#include "xiphos/core/engine_manager.hpp"
#include <markupfiltmgr.h>
#include <utf8html.h>
#include <array>
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
xc_status wait(xc_request *request) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  xc_status status;
  while ((status = xc_request_poll(request)) == XC_PENDING) {
    check(std::chrono::steady_clock::now() < deadline, "C note request deadline");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return status;
}
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
        check(note.module == "OptionOther" && note.html == expected[bits] && note.reference == "John 3:19" && note.identifier == note_id &&
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
    std::string generated_note_uri;
    {
      StudyWorker worker(path);
      const auto tab = worker.create_tab().get();
      worker.enable_module_preferences(tab).get();
      auto options = engine_options_from_preferences({}); options.strongs = true;
      worker.set_module_engine_options(tab, "OptionOther", options).get();
      const auto rendered = worker.read_entry(tab, "OptionOther", "John 3:19").get().html;
      const auto href = rendered.find("href=\"");
      check(href != std::string::npos, "engine generates note URI");
      const auto end = rendered.find('"', href + 6);
      generated_note_uri = rendered.substr(href + 6, end - href - 6);
      check(worker.study_note_link(tab, "OptionTest", generated_note_uri).get().html == expected[3],
            "actual engine note URI resolves explicit module/verse/body");
      const auto resolvedNote = worker.resolve_study_link(tab, "OptionTest", generated_note_uri).get();
      check(std::holds_alternative<StudyNote>(resolvedNote) && std::get<StudyNote>(resolvedNote).html == expected[3], "shared dispatch identifies note result");
      const auto resolvedPassage = worker.resolve_study_link(tab, "OptionOther", "sword:///John+3%3A18").get();
      check(std::holds_alternative<PassageLink>(resolvedPassage) && std::get<PassageLink>(resolvedPassage).module == "OptionOther" &&
            std::get<PassageLink>(resolvedPassage).reference == "John 3:18", "shared dispatch identifies passage without visiting");
      bool wrong_type = false;
      auto wrong_uri = generated_note_uri;
      wrong_uri.replace(wrong_uri.find("type=n"), 6, "type=x");
      try { (void)worker.study_note_link(tab, "OptionTest", wrong_uri).get(); }
      catch (const std::invalid_argument &) { wrong_type = true; }
      check(wrong_type, "note link type validated against entry metadata");
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
    {
      xc_worker *worker = nullptr; xc_request *request = nullptr;
      check(xc_worker_open(path.c_str(), &worker) == XC_OK, "C note worker");
      check(xc_worker_create_tab(worker, &request) == XC_OK && wait(request) == XC_OK, "C note tab");
      check(xc_request_take_note(request) == nullptr, "wrong request kind has no note");
      const auto tab = xc_request_tab(request); xc_request_release(request);
      check(xc_worker_set_module_engine_options(worker, tab, "OptionOther", XC_ENGINE_STRONGS | XC_ENGINE_FOOTNOTES, &request) == XC_OK &&
            wait(request) == XC_OK, "C note options"); xc_request_release(request);
      std::string id = note_id;
      check(xc_worker_study_note(worker, tab, "OptionOther", "John 3:19", id.c_str(), &request) == XC_OK, "C note request");
      id = "missing";
      check(xc_request_take_note(request) == nullptr, "note not ready until observed");
      check(wait(request) == XC_OK && xc_request_take_entry(request) == nullptr, "C note ready and type separated");
      auto *note = xc_request_take_note(request);
      check(note && xc_request_take_note(request) == nullptr, "note result transfers only once");
      xc_request_release(request);
      check(std::string(xc_note_module(note)) == "OptionOther" && std::string(xc_note_html(note)) == expected[3] && std::string(xc_note_reference(note)) == "John 3:19" &&
            std::string(xc_note_identifier(note)) == note_id && std::string(xc_note_type(note)) == note_type &&
            std::string(xc_note_label(note)) == note_label && std::string(xc_note_reference_list(note)).empty(), "C note copied metadata/body");
      check(xc_worker_resolve_study_link(worker, tab, "OptionTest", generated_note_uri.c_str(), &request) == XC_OK && wait(request) == XC_OK,
            "C shared note dispatch");
      auto *resolved_note = xc_request_take_note(request);
      check(resolved_note && !xc_request_take_link(request) && std::string(xc_note_html(resolved_note)) == expected[3], "C dispatch note result kind");
      xc_note_release(resolved_note); xc_request_release(request);
      check(xc_worker_resolve_study_link(worker, tab, "OptionOther", "sword:///John+3%3A18", &request) == XC_OK && wait(request) == XC_OK,
            "C shared passage dispatch");
      auto *resolved_link = xc_request_take_link(request);
      check(resolved_link && !xc_request_take_note(request) && std::string(xc_link_module(resolved_link)) == "OptionOther", "C dispatch passage result kind");
      xc_link_release(resolved_link); xc_request_release(request);
      check(xc_worker_study_note_link(worker, tab, "OptionTest", generated_note_uri.c_str(), &request) == XC_OK && wait(request) == XC_OK,
            "C generated note URI resolves");
      auto *linked = xc_request_take_note(request); xc_request_release(request);
      check(linked && std::string(xc_note_html(linked)) == expected[3], "C note link result"); xc_note_release(linked);
      check(xc_worker_study_note_link(worker, tab, "OptionTest", "https://example.invalid/passagestudy.jsp", &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT,
            "C external note URI rejected"); xc_request_release(request);
      check(xc_worker_study_note_link(worker, tab, nullptr, nullptr, &request) == XC_INVALID_ARGUMENT && request == nullptr,
            "C null note URI rejected before queue");
      check(xc_worker_study_note(worker, tab, "OptionOther", "John 3:18", reference_id.c_str(), &request) == XC_OK && wait(request) == XC_OK,
            "C cross-reference note");
      auto *reference = xc_request_take_note(request); xc_request_release(request);
      check(reference && std::string(xc_note_reference_list(reference)) == reference_list, "C reference-list metadata");
      xc_note_release(reference);
      check(xc_worker_study_note(worker, tab, "OptionOther", "John 3:17", note_id.c_str(), &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT &&
            xc_request_take_note(request) == nullptr, "C missing note failure"); xc_request_release(request);
      check(xc_worker_study_note(worker, tab, "Missing", "John 3:19", note_id.c_str(), &request) == XC_OK && wait(request) == XC_MODULE_NOT_FOUND,
            "C missing module status"); xc_request_release(request);
      check(xc_worker_study_note(worker, tab, "OptionOther", "John 3:19", nullptr, &request) == XC_INVALID_ARGUMENT && request == nullptr,
            "C null identifier rejected before queue");
      check(xc_worker_study_note(worker, tab, "OptionOther", "", "1", &request) == XC_INVALID_ARGUMENT && request == nullptr,
            "C empty reference rejected before queue");
      check(xc_worker_study_note(worker, tab, nullptr, "John 3:19", "1", &request) == XC_INVALID_ARGUMENT && request == nullptr,
            "C null module rejected before queue");
      check(xc_worker_study_note(worker, tab, "OptionOther", "John 3:19", "1", nullptr) == XC_INVALID_ARGUMENT, "C null output rejected");
      check(xc_worker_history(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "C queue continues after note error");
      auto *history = xc_request_take_history(request); xc_request_release(request);
      check(history && xc_history_count(history) == 0, "C note preview preserves history"); xc_history_release(history);
      check(xc_worker_close_tab(worker, tab, &request) == XC_OK && wait(request) == XC_OK, "C note tab close"); xc_request_release(request);
      check(xc_worker_study_note(worker, tab, "OptionOther", "John 3:19", "1", &request) == XC_OK && wait(request) == XC_INVALID_ARGUMENT,
            "C closed note tab rejected"); xc_request_release(request);
      xc_worker_close(worker);
      check(std::string(xc_note_html(note)) == expected[3], "C note outlives worker/request"); xc_note_release(note);
      xc_note_release(nullptr);
      check(!xc_note_module(nullptr) && !xc_note_reference(nullptr) && !xc_note_identifier(nullptr) && !xc_note_type(nullptr) && !xc_note_label(nullptr) &&
            !xc_note_html(nullptr) && !xc_note_reference_list(nullptr) && !xc_request_take_note(nullptr), "C null result access");
    }
    std::cout << "Shared study-note legacy rendering, context, lifetime and isolation checks passed\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
