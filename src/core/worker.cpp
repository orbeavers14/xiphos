// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/profile.hpp"
#include "xiphos/core/note_presentation.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <algorithm>
namespace xiphos::core {
struct StudyWorker::Impl {
  struct State {
    Reader reader;
    std::map<StudyTab, std::unique_ptr<StudySession>> tabs;
    StudyTab next_tab = 1;
    explicit State(const std::string &root) : reader(root) {}
    StudySession &tab(StudyTab id) {
      const auto found = tabs.find(id);
      if (found == tabs.end()) throw std::invalid_argument("Study tab is closed or unknown");
      return *found->second;
    }
    StudyNote note_target(StudyTab id, const StudyNoteLink &link, bool render_body = true) {
      auto &session = tab(id);
      auto note = render_body ? session.study_note(link.module, link.reference, link.identifier)
                              : session.study_note_metadata(link.module, link.reference, link.identifier);
      if ((link.type == "x") != (note.type == "crossReference"))
        throw std::invalid_argument("Study-note link type does not match the entry");
      return note;
    }
    StudyNote note_link(StudyTab id, const std::string &current_module, const std::string &uri) {
      return note_target(id, xiphos::core::study_note_link(uri, current_module));
    }
    ContextualStudyLink study_link_at(StudyTab tab, const std::string &current_module,
                                     const std::string &current_reference, const std::string &uri, bool record_lexical,
                                     const std::string &navigation_module = "") {
      auto &session = this->tab(tab);
      const auto target = navigation_module.empty() ? xiphos::core::contextual_study_link(uri, current_module)
          : xiphos::core::contextual_study_link(uri, current_module, navigation_module);
      if (const auto *passage = std::get_if<PassageLink>(&target))
        return reader.resolve_link(passage->module, passage_uri(passage->reference));
      if (const auto *list = std::get_if<ReferenceListLink>(&target))
        return session.read_reference_list(list->module, list->expression, current_reference);
      if (std::holds_alternative<LexicalLink>(target))
        return record_lexical ? session.visit_lexical(current_module, current_reference, uri, XIPHOS_LEXICON_MAIN)
                              : session.read_lexical(current_module, current_reference, uri, XIPHOS_LEXICON_MAIN);
      return note_target(tab, std::get<StudyNoteLink>(target));
    }
  };
  using Job = std::function<void(State *, std::exception_ptr)>;
  std::mutex mutex;
  std::condition_variable ready;
  std::deque<Job> jobs;
  bool stopping = false;
  std::thread thread;
  explicit Impl(std::string root) : thread([this, root = std::move(root)] { run(root); }) {}
  ~Impl() {
    { std::lock_guard<std::mutex> lock(mutex); stopping = true; }
    ready.notify_one();
    thread.join();
  }
  void run(const std::string &root) {
    std::unique_ptr<State> state;
    std::exception_ptr initialization_error;
    try { state = std::make_unique<State>(root); }
    catch (...) { initialization_error = std::current_exception(); }
    for (;;) {
      Job job;
      {
        std::unique_lock<std::mutex> lock(mutex);
        ready.wait(lock, [this] { return stopping || !jobs.empty(); });
        if (jobs.empty()) break; // stopping and no accepted work left
        job = std::move(jobs.front()); jobs.pop_front();
      }
      job(state.get(), initialization_error);
    }
    // State, then sessions/reader, are destroyed here on the engine thread.
  }
  template<class F> auto submit(F operation) {
    using Result = std::invoke_result_t<F, State &>;
    auto task = std::make_shared<std::packaged_task<Result(State *, std::exception_ptr)>>(
      [operation = std::move(operation)](State *state, std::exception_ptr error) mutable -> Result {
        if (error) std::rethrow_exception(error);
        return operation(*state);
      });
    auto future = task->get_future();
    {
      std::lock_guard<std::mutex> lock(mutex);
      if (stopping) throw std::logic_error("Engine worker is shutting down");
      jobs.emplace_back([task](State *state, std::exception_ptr error) { (*task)(state, error); });
    }
    ready.notify_one();
    return future;
  }
};
StudyWorker::StudyWorker(std::string root) : impl_(std::make_unique<Impl>(std::move(root))) {}
StudyWorker::~StudyWorker() = default;
std::future<std::vector<Module>> StudyWorker::modules() {
  return impl_->submit([](Impl::State &s) { return s.reader.modules(); });
}
std::future<StudyTab> StudyWorker::create_tab() {
  return impl_->submit([](Impl::State &s) {
    if (s.next_tab == std::numeric_limits<StudyTab>::max()) throw std::overflow_error("Study tab IDs exhausted");
    auto session = std::make_unique<StudySession>(s.reader);
    const auto id = s.next_tab;
    s.tabs.emplace(id, std::move(session)); ++s.next_tab;
    return id;
  });
}
std::future<void> StudyWorker::close_tab(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) { s.tab(tab); s.tabs.erase(tab); });
}
std::future<void> StudyWorker::set_lexicon_preferences(StudyTab tab, LexiconPreferences preferences) {
  return impl_->submit([tab, preferences = std::move(preferences)](Impl::State &s) {
    s.tab(tab).set_lexicon_preferences(preferences);
  });
}
std::future<std::optional<LexiconPreferences>> StudyWorker::lexicon_preferences(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) { return s.tab(tab).lexicon_preferences(); });
}
std::future<void> StudyWorker::set_cross_reference_preview_list(StudyTab tab, bool enabled) {
  return impl_->submit([tab, enabled](Impl::State &s) { s.tab(tab).set_cross_reference_preview_list(enabled); });
}
std::future<bool> StudyWorker::cross_reference_preview_list(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) { return s.tab(tab).cross_reference_preview_list(); });
}
std::future<StudyHoverLink> StudyWorker::hover_study_link_at(StudyTab tab, std::string current_module,
                                                         std::string current_reference, std::string uri) {
  return impl_->submit([tab, current_module = std::move(current_module), current_reference = std::move(current_reference),
                        uri = std::move(uri)](Impl::State &s) -> StudyHoverLink {
    auto &session = s.tab(tab);
    if (current_reference.empty() || uri.empty() || current_module.find('\0') != std::string::npos ||
        current_reference.find('\0') != std::string::npos || uri.find('\0') != std::string::npos)
      throw std::invalid_argument("Hover requires copied source context and a valid URI");
    const auto target = xiphos::core::contextual_hover_link(uri, current_module, current_reference);
    if (std::holds_alternative<PassageLink>(target) || std::holds_alternative<ReferenceListLink>(target))
      return std::monostate{};
    if (std::holds_alternative<LexicalLink>(target))
      return session.read_lexical(current_module, current_reference, uri, XIPHOS_LEXICON_MAIN);
    auto link = std::get<StudyNoteLink>(target);
    if (!std::any_of(s.reader.modules().begin(), s.reader.modules().end(),
                    [&](const Module &module) { return module.id == link.module; }))
      link.module = current_module;
    const auto destination = xiphos_core_note_presentation(link.type.c_str(), 0, session.cross_reference_preview_list());
    if (destination == XIPHOS_NOTE_NO_PRESENTATION) return std::monostate{};
    auto note = s.note_target(tab, link, destination == XIPHOS_NOTE_HOVER_BODY);
    if (destination == XIPHOS_NOTE_HOVER_BODY) return note;
    if (destination == XIPHOS_NOTE_HOVER_REFERENCES)
      return session.read_reference_list(note.module, note.reference_list, current_reference);
    note.html = display_reference_links(session.reference_targets(note.module, note.reference_list, current_reference));
    return note;
  });
}
std::future<std::optional<LexiconPreferences>> StudyWorker::import_lexicon_profile(StudyTab tab, std::string path) {
  return impl_->submit([tab, path = std::move(path)](Impl::State &s) {
    auto &session = s.tab(tab);
    const auto profile = read_study_profile(path);
    auto copied = std::make_optional(profile.lexicons);
    session.set_lexicon_preferences(profile.lexicons);
    return copied;
  });
}
std::future<void> StudyWorker::save_lexicon_profile(StudyTab tab, std::string path) {
  return impl_->submit([tab, path = std::move(path)](Impl::State &s) {
    const auto &preferences = s.tab(tab).lexicon_preferences();
    if (!preferences) throw std::invalid_argument("Configure lexicons before saving a profile");
    xiphos::core::save_lexicon_profile(path, *preferences);
  });
}
std::future<LexicalEntry> StudyWorker::visit_lexical(StudyTab tab, std::string module, std::string reference, std::string uri, int origin) {
  return impl_->submit([tab, module = std::move(module), reference = std::move(reference), uri = std::move(uri), origin](Impl::State &s) {
    return s.tab(tab).visit_lexical(module, reference, uri, origin);
  });
}
std::future<LexicalEntry> StudyWorker::read_lexical(StudyTab tab, std::string module, std::string reference,
                                               std::string uri, int origin) {
  return impl_->submit([tab, module = std::move(module), reference = std::move(reference), uri = std::move(uri), origin](Impl::State &s) {
    return s.tab(tab).read_lexical(module, reference, uri, origin);
  });
}
std::future<DictionaryChoices> StudyWorker::dictionary_choices(StudyTab tab, std::string module, std::string key) {
  return impl_->submit([tab, module = std::move(module), key = std::move(key)](Impl::State &s) {
    s.tab(tab);
    return s.reader.dictionary_choices(module, key);
  });
}
std::future<DictionaryEntry> StudyWorker::visit_dictionary(StudyTab tab, std::string module, std::string key) {
  return impl_->submit([tab, module = std::move(module), key = std::move(key)](Impl::State &s) {
    return s.tab(tab).visit_dictionary(module, key);
  });
}
std::future<DictionaryEntry> StudyWorker::navigate_dictionary_history(StudyTab tab, int direction) {
  return impl_->submit([tab, direction](Impl::State &s) { return s.tab(tab).navigate_dictionary_history(direction); });
}
std::future<DictionaryHistorySnapshot> StudyWorker::dictionary_history(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) {
    const auto &history = s.tab(tab).dictionary_history();
    return DictionaryHistorySnapshot{history.entries(), history.cursor(), history.can_back(), history.can_forward()};
  });
}
std::future<void> StudyWorker::clear_dictionary_history(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) { s.tab(tab).clear_dictionary_history(); });
}
std::future<DictionaryEntry> StudyWorker::navigate_dictionary(StudyTab tab, std::string module, std::string key, int direction) {
  return impl_->submit([tab, module = std::move(module), key = std::move(key), direction](Impl::State &s) {
    return s.tab(tab).navigate_dictionary(module, key, direction);
  });
}
std::future<DictionaryEntry> StudyWorker::read_dictionary(StudyTab tab, std::string module, std::string key) {
  return impl_->submit([tab, module = std::move(module), key = std::move(key)](Impl::State &s) {
    return s.tab(tab).read_dictionary(module, key);
  });
}
std::future<ReferenceList> StudyWorker::read_reference_list(StudyTab tab, std::string module, std::string expression, std::string context) {
  return impl_->submit([tab, module = std::move(module), expression = std::move(expression), context = std::move(context)](Impl::State &s) {
    return s.tab(tab).read_reference_list(module, expression, context);
  });
}
std::future<ContextualStudyLink> StudyWorker::follow_pane_link_at(StudyTab tab, std::string navigation_module,
                                                                std::string source_module, std::string current_reference,
                                                                std::string uri) {
  return impl_->submit([tab, navigation_module = std::move(navigation_module), source_module = std::move(source_module),
                        current_reference = std::move(current_reference), uri = std::move(uri)](Impl::State &s) {
    return s.study_link_at(tab, source_module, current_reference, uri, true, navigation_module);
  });
}
std::future<ContextualStudyLink> StudyWorker::follow_study_link_at(StudyTab tab, std::string current_module, std::string current_reference, std::string uri) {
  return impl_->submit([tab, current_module = std::move(current_module), current_reference = std::move(current_reference),
                        uri = std::move(uri)](Impl::State &s) -> ContextualStudyLink {
    return s.study_link_at(tab, current_module, current_reference, uri, true);
  });
}
std::future<ContextualStudyLink> StudyWorker::resolve_study_link_at(StudyTab tab, std::string current_module, std::string current_reference, std::string uri) {
  return impl_->submit([tab, current_module = std::move(current_module), current_reference = std::move(current_reference),
                        uri = std::move(uri)](Impl::State &s) -> ContextualStudyLink {
    return s.study_link_at(tab, current_module, current_reference, uri, false);
  });
}
std::future<ResolvedStudyLink> StudyWorker::resolve_study_link(StudyTab tab, std::string current_module, std::string uri) {
  return impl_->submit([tab, current_module = std::move(current_module), uri = std::move(uri)](Impl::State &s) -> ResolvedStudyLink {
    s.tab(tab); // Both kinds require an open study context.
    const auto target = xiphos::core::study_link(uri, current_module);
    if (const auto *passage = std::get_if<PassageLink>(&target))
      return s.reader.resolve_link(passage->module, passage_uri(passage->reference));
    return s.note_target(tab, std::get<StudyNoteLink>(target));
  });
}
std::future<StudyNote> StudyWorker::study_note_link(StudyTab tab, std::string current_module, std::string uri) {
  return impl_->submit([tab, current_module = std::move(current_module), uri = std::move(uri)](Impl::State &s) {
    return s.note_link(tab, current_module, uri);
  });
}
std::future<StudyNote> StudyWorker::study_note(StudyTab tab, std::string module, std::string reference, std::string identifier) {
  return impl_->submit([tab, module = std::move(module), reference = std::move(reference),
                        identifier = std::move(identifier)](Impl::State &s) {
    return s.tab(tab).study_note(module, reference, identifier);
  });
}
std::future<Entry> StudyWorker::read_entry(StudyTab tab, std::string module, std::string reference) {
  return impl_->submit([tab, module = std::move(module), reference = std::move(reference)](Impl::State &s) {
    return s.tab(tab).read_entry(module, reference);
  });
}
std::future<StudyPassage> StudyWorker::read_passage(StudyTab tab, std::string module, std::string commentary,
                                                  std::string reference) {
  return impl_->submit([tab, module = std::move(module), commentary = std::move(commentary),
                        reference = std::move(reference)](Impl::State &s) {
    return s.tab(tab).read_passage(module, commentary, reference);
  });
}
std::future<StudyPassage> StudyWorker::visit_passage(StudyTab tab, std::string module, std::string commentary,
                                                   std::string reference) {
  return impl_->submit([tab, module = std::move(module), commentary = std::move(commentary),
                        reference = std::move(reference)](Impl::State &s) {
    return s.tab(tab).visit_passage(module, commentary, reference);
  });
}
std::future<StudyPassage> StudyWorker::visit_linked_passage(StudyTab tab, std::string bible, std::string commentary,
                                                          PassageLink target) {
  return impl_->submit([tab, bible = std::move(bible), commentary = std::move(commentary), target = std::move(target)](Impl::State &s) {
    return s.tab(tab).visit_linked_passage(bible, commentary, target);
  });
}
std::future<StudyPassage> StudyWorker::navigate_passage(StudyTab tab, int direction) {
  return impl_->submit([tab, direction](Impl::State &s) { return s.tab(tab).navigate_passage(direction); });
}
std::future<StudyPassage> StudyWorker::navigate_passage_chapter(StudyTab tab, std::string module,
                                                            std::string commentary, std::string reference,
                                                            int direction) {
  return impl_->submit([tab, module = std::move(module), commentary = std::move(commentary),
                        reference = std::move(reference), direction](Impl::State &s) {
    return s.tab(tab).navigate_passage_chapter(module, commentary, reference, direction);
  });
}
std::future<Entry> StudyWorker::visit(StudyTab tab, std::string module, std::string reference) {
  return impl_->submit([tab, module = std::move(module), reference = std::move(reference)](Impl::State &s) {
    return s.tab(tab).visit(module, reference);
  });
}
std::future<Entry> StudyWorker::navigate(StudyTab tab, int direction) {
  return impl_->submit([tab, direction](Impl::State &s) { return s.tab(tab).navigate(direction); });
}
std::future<Entry> StudyWorker::navigate_chapter(StudyTab tab, std::string module, std::string reference, int direction) {
  return impl_->submit([tab, module = std::move(module), reference = std::move(reference), direction](Impl::State &s) {
    return s.tab(tab).navigate_chapter(module, reference, direction);
  });
}
std::future<void> StudyWorker::set_engine_options(StudyTab tab, EngineOptions options) {
  return impl_->submit([tab, options](Impl::State &s) { s.tab(tab).set_engine_options(options); });
}
std::future<void> StudyWorker::clear_engine_options(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) { s.tab(tab).clear_engine_options(); });
}
std::future<void> StudyWorker::save_module_preferences(StudyTab tab, std::string path) {
  return impl_->submit([tab, path = std::move(path)](Impl::State &s) { s.tab(tab).save_module_preferences(path); });
}
std::future<void> StudyWorker::import_module_preferences(StudyTab tab, std::string path) {
  return impl_->submit([tab, path = std::move(path)](Impl::State &s) { s.tab(tab).import_module_preferences(path); });
}
std::future<void> StudyWorker::enable_module_preferences(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) { s.tab(tab).enable_module_preferences(); });
}
std::future<void> StudyWorker::select_module_variant(StudyTab tab, std::string module, int choice) {
  return impl_->submit([tab, module = std::move(module), choice](Impl::State &s) {
    s.tab(tab).select_module_variant(module, choice);
  });
}
std::future<void> StudyWorker::set_module_engine_options(StudyTab tab, std::string module, EngineOptions options) {
  return impl_->submit([tab, module = std::move(module), options](Impl::State &s) {
    s.tab(tab).set_module_engine_options(module, options);
  });
}
std::future<void> StudyWorker::clear_module_engine_options(StudyTab tab, std::string module) {
  return impl_->submit([tab, module = std::move(module)](Impl::State &s) { s.tab(tab).clear_module_engine_options(module); });
}
std::future<std::optional<EngineOptions>> StudyWorker::module_engine_options(StudyTab tab, std::string module) {
  return impl_->submit([tab, module = std::move(module)](Impl::State &s) { return s.tab(tab).module_engine_options(module); });
}
std::future<ModulePresentationOptions> StudyWorker::module_presentation_options(StudyTab tab, std::string module) {
  return impl_->submit([tab, module = std::move(module)](Impl::State &s) { return s.tab(tab).module_presentation_options(module); });
}
std::future<void> StudyWorker::set_image_content(StudyTab tab, std::string module, bool enabled) {
  return impl_->submit([tab, module = std::move(module), enabled](Impl::State &s) { s.tab(tab).set_image_content(module, enabled); });
}
std::future<void> StudyWorker::set_respect_font_faces(StudyTab tab, std::string module, bool enabled) {
  return impl_->submit([tab, module = std::move(module), enabled](Impl::State &s) { s.tab(tab).set_respect_font_faces(module, enabled); });
}
std::future<void> StudyWorker::set_headings(StudyTab tab, std::string module, bool enabled) {
  return impl_->submit([tab, module = std::move(module), enabled](Impl::State &s) { s.tab(tab).set_headings(module, enabled); });
}
std::future<void> StudyWorker::set_commentary_chapter_mode(StudyTab tab, std::string module, bool enabled) {
  return impl_->submit([tab, module = std::move(module), enabled](Impl::State &s) { s.tab(tab).set_commentary_chapter_mode(module, enabled); });
}
std::future<Chapter> StudyWorker::read_commentary_chapter(StudyTab tab, std::string module, std::string reference, DisplayOptions options) {
  return impl_->submit([tab, module = std::move(module), reference = std::move(reference), options](Impl::State &s) {
    return s.tab(tab).read_commentary_chapter(module, reference, options);
  });
}
std::future<Chapter> StudyWorker::read_chapter(StudyTab tab, std::string module, std::string reference, DisplayOptions options) {
  return impl_->submit([tab, module = std::move(module), reference = std::move(reference), options](Impl::State &s) {
    return s.tab(tab).read_chapter(module, reference, options);
  });
}
std::future<HistorySnapshot> StudyWorker::history(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) {
    const auto &h = s.tab(tab).history();
    return HistorySnapshot{h.entries(), h.cursor(), h.can_back(), h.can_forward()};
  });
}
std::future<void> StudyWorker::clear_history(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) { s.tab(tab).clear_history(); });
}
std::future<Chapter> StudyWorker::read_chapter(std::string module, std::string reference, DisplayOptions options) {
  return impl_->submit([module = std::move(module), reference = std::move(reference), options](Impl::State &s) {
    return s.reader.read_chapter(module, reference, options);
  });
}
std::future<ReferenceChoices> StudyWorker::reference_choices(std::string module, std::string reference) {
  return impl_->submit([module = std::move(module), reference = std::move(reference)](Impl::State &s) {
    return s.reader.reference_choices(module, reference);
  });
}
std::future<PassageLink> StudyWorker::resolve_link(std::string module, std::string uri) {
  return impl_->submit([module = std::move(module), uri = std::move(uri)](Impl::State &s) {
    return s.reader.resolve_link(module, uri);
  });
}
}
