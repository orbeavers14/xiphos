// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/worker.hpp"
#include "xiphos/core/profile.hpp"
#include "search_control.hpp"
#include "xiphos/core/note_presentation.h"
#include "xiphos/core/reference_command.hpp"
#include "xiphos/core/passage_uri.hpp"
#include "xiphos/core/dictionary.hpp"
#include "xiphos/core/study_link_admission.hpp"
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
namespace {
template<class T> std::future<T> rejected_study_link() {
  std::promise<T> result;
  auto future = result.get_future();
  result.set_exception(std::make_exception_ptr(std::invalid_argument("Study link exceeds byte bounds or contains NUL")));
  return future;
}
}
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
    bool installed_module(const char *id) const {
      return id && std::any_of(reader.modules().begin(), reader.modules().end(),
          [&](const Module &module) { return module.id == id; });
    }
    const Module *uri_module(const std::string &token) {
      const auto resolved = reader.resolve_module_name(token);
      if (!resolved.module) return nullptr;
      const auto found = std::find_if(reader.modules().begin(), reader.modules().end(),
          [&](const Module &module) { return module.id == *resolved.module; });
      return found == reader.modules().end() ? nullptr : &*found;
    }
    ContextualStudyLink study_link_at(StudyTab tab, const std::string &current_module,
                                     const std::string &current_reference, const std::string &uri, bool record_lexical,
                                     const std::string &navigation_module = "") {
      auto &session = this->tab(tab);
      if (current_module.find('\0') != std::string::npos || navigation_module.find('\0') != std::string::npos ||
          current_reference.find('\0') != std::string::npos)
        throw std::invalid_argument("Study link context contains NUL");
      if (uri.compare(0, 8, "sword://") == 0 || uri.compare(0, 8, "bible://") == 0) {
        auto keyed = module_uri_link(uri);
        const auto &main_module = navigation_module.empty() ? current_module : navigation_module;
        const auto *module = uri_module(keyed.module.empty() ? main_module : keyed.module);
        if (module && module->type == "Lexicons / Dictionaries") {
          // Devotional click selection has its own dated-pane policy. Do not
          // silently treat it as a normal dictionary history visit.
          if (!module->dictionary) throw std::invalid_argument("URI target requires devotional navigation");
          passage_uri_anchor(keyed.key.data());
          keyed.key.resize(std::strlen(keyed.key.c_str()));
          const auto key = record_lexical ? dictionary_click_key(keyed.key) : keyed.key;
          return record_lexical ? session.visit_dictionary(module->id, key) : session.read_dictionary(module->id, key);
        }
      }
      const auto target = navigation_module.empty() ? xiphos::core::contextual_study_link(uri, current_module)
          : xiphos::core::contextual_study_link(uri, current_module, navigation_module);
      if (std::holds_alternative<PassageLink>(target))
        return reader.resolve_link(navigation_module.empty() ? current_module : navigation_module, uri);
      if (const auto *list = std::get_if<ReferenceListLink>(&target)) {
        const bool is_passage_uri = uri.compare(0, 8, "sword://") == 0 || uri.compare(0, 8, "bible://") == 0;
        if (is_passage_uri) {
          const auto resolved = reader.resolve_module_name(list->module);
          if (!resolved.module) {
            const auto &main_module = navigation_module.empty() ? current_module : navigation_module;
            return reader.navigation_target(main_module, list->expression);
          }
          const bool sidebar = passage_uri_reference_list(resolved.module->c_str(), list->expression.c_str(),
              [&](const char *id) {
                return std::any_of(reader.modules().begin(), reader.modules().end(),
                    [&](const Module &module) { return module.id == id && module.type == "Biblical Texts"; });
              });
          if (!sidebar) return reader.navigation_target(*resolved.module, list->expression);
          return session.read_reference_list(*resolved.module, list->expression, current_reference);
        }
        ReferenceList result;
        const auto &main_module = navigation_module.empty() ? current_module : navigation_module;
        dispatch_reference_command(list->module.c_str(), main_module.c_str(), current_reference.c_str(),
            list->expression.c_str(), true, [&](const char *id) { return installed_module(id); },
            [&](const char *context, const char *id, const char *expression) {
              result = session.read_reference_list(id, expression, context);
            });
        return result;
      }
      if (std::holds_alternative<LexicalLink>(target))
        return record_lexical ? session.visit_lexical(current_module, current_reference, uri, XIPHOS_LEXICON_MAIN)
                              : session.read_lexical(current_module, current_reference, uri, XIPHOS_LEXICON_MAIN);
      if(const auto *image=std::get_if<VerseImageLink>(&target)){
        if(current_reference.empty()||current_reference.find('\0')!=std::string::npos)
          throw std::invalid_argument("Image command requires committed reading context");
        return reader.read_verse_image_link(image->module,image->uri);
      }
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
  template<class F> auto submit(F operation,std::shared_ptr<SearchControl> failure_control = {}) {
    using Result = std::invoke_result_t<F, State &>;
    auto task = std::make_shared<std::packaged_task<Result(State *, std::exception_ptr)>>(
      [operation = std::move(operation),failure_control=std::move(failure_control)](State *state, std::exception_ptr error) mutable -> Result {
        if (error) { detail::SearchControlAccess::fail_if_queued(failure_control); std::rethrow_exception(error); }
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
StudySearch StudyWorker::search_module_task(StudyTab tab,SearchQuery query,SearchLimits limits) {
  StudySearch task{std::make_shared<SearchControl>(),{}};
  task.result=impl_->submit([tab,query=std::move(query),limits,control=task.control](Impl::State &s){
    try{return s.tab(tab).search_module(query,limits,control);}
    catch(...){detail::SearchControlAccess::fail_if_queued(control);throw;}
  },task.control);return task;
}
std::future<SearchResult> StudyWorker::search_module(StudyTab tab,SearchQuery query,SearchLimits limits) {
  return search_module_task(tab,std::move(query),limits).result;
}
StudyBatchSearch StudyWorker::search_batch_task(StudyTab tab,std::vector<SearchQuery> queries,SearchBatchLimits limits) {
  StudyBatchSearch task{std::make_shared<SearchControl>(),{}};
  task.result=impl_->submit([tab,queries=std::move(queries),limits,control=task.control](Impl::State &s){
    try{return s.tab(tab).search_batch(queries,limits,control);}
    catch(...){detail::SearchControlAccess::fail_if_queued(control);throw;}
  },task.control);return task;
}
std::future<SearchBatchResult> StudyWorker::search_batch(StudyTab tab,std::vector<SearchQuery> queries,SearchBatchLimits limits) {
  return search_batch_task(tab,std::move(queries),limits).result;
}
std::future<SearchGenerationInfo> StudyWorker::search_generation(StudyTab tab){
  return impl_->submit([tab](Impl::State &s){return s.tab(tab).search_generation();});
}
std::future<SearchRetentionLimits> StudyWorker::set_search_retention_limits(StudyTab tab,SearchRetentionLimits limits){
  return impl_->submit([tab,limits](Impl::State &s){auto &session=s.tab(tab);session.set_search_retention_limits(limits);return session.search_retention_limits();});
}
std::future<SearchRetentionLimits> StudyWorker::search_retention_limits(StudyTab tab){
  return impl_->submit([tab](Impl::State &s){return s.tab(tab).search_retention_limits();});
}
std::future<SearchGenerationRead> StudyWorker::read_search_generation(StudyTab tab,std::uint64_t id,std::size_t group,SearchReferenceSource source,SearchLimits limits){
  return impl_->submit([tab,id,group,source,limits](Impl::State &s){return s.tab(tab).read_search_generation(id,group,source,limits);});
}
StudyWorker::SearchHtmlExportTask StudyWorker::export_search_html_task(StudyTab tab,SearchHtmlExportRequest request){
  SearchHtmlExportTask task{std::make_shared<SearchControl>(),{}};
  task.result=impl_->submit([tab,request=std::move(request),control=task.control](Impl::State &s){
    try{return s.tab(tab).export_search_html(request,control);}
    catch(...){detail::SearchControlAccess::fail_if_queued(control);throw;}
  },task.control);return task;
}
std::future<SearchHtmlExport> StudyWorker::export_search_html(StudyTab tab,SearchHtmlExportRequest request){
  return export_search_html_task(tab,std::move(request)).result;
}
StudyWorker::SearchExportTask StudyWorker::export_search_all_text_task(StudyTab tab,SearchAllTextExportRequest request){
  SearchExportTask task{std::make_shared<SearchControl>(),{}};
  task.result=impl_->submit([tab,request=std::move(request),control=task.control](Impl::State &s){
    try{return s.tab(tab).export_search_all_text(request,control);}
    catch(...){detail::SearchControlAccess::fail_if_queued(control);throw;}
  },task.control);return task;
}
std::future<SearchTextExport> StudyWorker::export_search_all_text(StudyTab tab,SearchAllTextExportRequest request){
  return export_search_all_text_task(tab,std::move(request)).result;
}
StudyWorker::SearchExportTask StudyWorker::export_search_visible_text_task(StudyTab tab,SearchTextExportRequest request){
  SearchExportTask task{std::make_shared<SearchControl>(),{}};
  task.result=impl_->submit([tab,request=std::move(request),control=task.control](Impl::State &s){
    try{return s.tab(tab).export_search_visible_text(request,control);}
    catch(...){detail::SearchControlAccess::fail_if_queued(control);throw;}
  },task.control);return task;
}
std::future<SearchTextExport> StudyWorker::export_search_visible_text(StudyTab tab,SearchTextExportRequest request){
  return export_search_visible_text_task(tab,std::move(request)).result;
}
std::future<SearchScopeInfo> StudyWorker::last_search_scope(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s){return s.tab(tab).last_search_scope();});
}
std::future<SearchScopeInfo> StudyWorker::clear_search_scope(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s){auto &session=s.tab(tab);session.clear_search_scope();return session.last_search_scope();});
}
std::future<SearchTextState> StudyWorker::search_text_options(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s){return s.tab(tab).search_text_options();});
}
std::future<SearchTextState> StudyWorker::clear_search_text_options(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s){auto &session=s.tab(tab);session.clear_search_text_options();return session.search_text_options();});
}
std::future<BookTree> StudyWorker::book_tree(std::string module, std::uint64_t offset) {
  return impl_->submit([module = std::move(module), offset](Impl::State &s) { return s.reader.book_tree(module, offset); });
}
std::future<BookTree> StudyWorker::book_tree_path(std::string module, std::string path) {
  return impl_->submit([module = std::move(module), path = std::move(path)](Impl::State &s) { return s.reader.book_tree_path(module, path); });
}
std::future<std::optional<BookTree>> StudyWorker::resolve_saved_book_selection(SavedBookSelection saved, BookRestoreMode mode) {
  return impl_->submit([saved = std::move(saved), mode](Impl::State &s) -> std::optional<BookTree> {
    const auto target = saved_book_target(saved, mode);
    if (!target) return std::nullopt;
    return s.reader.resolve_book_target(*target);
  });
}
std::future<BookEntry> StudyWorker::read_book(StudyTab tab, std::string module, std::uint64_t offset) {
  return impl_->submit([tab, module = std::move(module), offset](Impl::State &s) { return s.tab(tab).read_book(module, offset); });
}
std::future<BookDocument> StudyWorker::read_book_document(StudyTab tab, std::string module, std::uint64_t offset) {
  return impl_->submit([tab, module = std::move(module), offset](Impl::State &s) { return s.tab(tab).read_book_document(module, offset); });
}
std::future<BookDocument> StudyWorker::select_book_document(StudyTab tab, BookSelectionTarget target) {
  return impl_->submit([tab, target = std::move(target)](Impl::State &s) { return s.tab(tab).select_book_document(target); });
}
std::future<std::optional<BookDocument>> StudyWorker::refresh_book_document(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) { return s.tab(tab).refresh_book_document(); });
}
std::future<std::optional<BookDocument>> StudyWorker::navigate_book_document(StudyTab tab, BookNavigation direction) {
  return impl_->submit([tab, direction](Impl::State &s) { return s.tab(tab).navigate_book_document(direction); });
}
std::future<std::optional<BookDocument>> StudyWorker::restore_book_document_selection(StudyTab tab, SavedBookSelection saved, BookRestoreMode mode) {
  return impl_->submit([tab, saved = std::move(saved), mode](Impl::State &s) { return s.tab(tab).restore_book_document_selection(saved, mode); });
}
std::future<std::optional<BookDocument>> StudyWorker::restore_book_document_profile(StudyTab tab, std::string path, BookRestoreMode mode) {
  return impl_->submit([tab, path = std::move(path), mode](Impl::State &s) {
    auto &session = s.tab(tab);
    if (mode != BookRestoreMode::display_key && mode != BookRestoreMode::initial_window)
      throw std::invalid_argument("Unknown book restore mode");
    return session.restore_book_document_selection(read_saved_book_selection(path), mode);
  });
}
std::future<std::optional<BookDocument>> StudyWorker::book_document_selection(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) -> std::optional<BookDocument> {
    const auto &selected = s.tab(tab).book_selection();
    return selected ? selected->document : std::nullopt;
  });
}
std::future<BookNote> StudyWorker::book_document_note_link(StudyTab tab, std::string uri, bool render_body) {
  return impl_->submit([tab, uri = std::move(uri), render_body](Impl::State &s) { return s.tab(tab).book_document_note_link(uri, render_body); });
}
std::future<BookImageTarget> StudyWorker::book_image_link(StudyTab tab, std::string uri) {
  return impl_->submit([tab, uri = std::move(uri)](Impl::State &s) { return s.tab(tab).book_image_link(uri); });
}
std::future<BookImageResource> StudyWorker::read_book_image_link(StudyTab tab, std::string uri) {
  return impl_->submit([tab, uri = std::move(uri)](Impl::State &s) { return s.tab(tab).read_book_image_link(uri); });
}
std::future<BookFollowLink> StudyWorker::follow_book_link(StudyTab tab, std::string uri) {
  return impl_->submit([tab, uri = std::move(uri)](Impl::State &s) { return s.tab(tab).follow_book_link(uri); });
}
std::future<BookEntry> StudyWorker::select_book(StudyTab tab, BookSelectionTarget target) {
  return impl_->submit([tab, target = std::move(target)](Impl::State &s) { return s.tab(tab).select_book(target); });
}
std::future<std::optional<BookEntry>> StudyWorker::navigate_book(StudyTab tab, BookNavigation direction) {
  return impl_->submit([tab, direction](Impl::State &s) { return s.tab(tab).navigate_book(direction); });
}
std::future<std::optional<BookEntry>> StudyWorker::restore_book_selection(StudyTab tab, SavedBookSelection saved, BookRestoreMode mode) {
  return impl_->submit([tab, saved = std::move(saved), mode](Impl::State &s) {
    return s.tab(tab).restore_book_selection(saved, mode);
  });
}
std::future<std::optional<BookEntry>> StudyWorker::restore_book_profile(StudyTab tab, std::string path, BookRestoreMode mode) {
  return impl_->submit([tab, path = std::move(path), mode](Impl::State &s) {
    auto &session = s.tab(tab); // reject closed tabs before accessing a file
    if (mode != BookRestoreMode::display_key && mode != BookRestoreMode::initial_window)
      throw std::invalid_argument("Unknown book restore mode");
    const auto saved = read_saved_book_selection(path);
    return session.restore_book_selection(saved, mode);
  });
}
std::future<std::optional<BookTree>> StudyWorker::book_selection(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) -> std::optional<BookTree> {
    const auto &selected = s.tab(tab).book_selection();
    if (!selected) return std::nullopt;
    return selected->tree;
  });
}
std::future<BookNote> StudyWorker::book_note(StudyTab tab, std::string module, std::uint64_t offset,
                                          std::string identifier, bool render_body) {
  return impl_->submit([tab, module = std::move(module), offset, identifier = std::move(identifier), render_body](Impl::State &s) {
    return s.tab(tab).book_note(module, offset, identifier, render_body);
  });
}
std::future<BookNote> StudyWorker::book_note_link_at(StudyTab tab, std::string module, std::uint64_t offset,
                                                  std::string uri, bool render_body) {
  return impl_->submit([tab, module = std::move(module), offset, uri = std::move(uri), render_body](Impl::State &s) {
    return s.tab(tab).book_note_link_at(module, offset, uri, render_body);
  });
}
std::future<BookChildren> StudyWorker::book_children(std::string module, std::uint64_t parent,
                                                    std::optional<std::uint64_t> after, std::uint32_t limit) {
  return impl_->submit([module = std::move(module), parent, after, limit](Impl::State &s) {
    return s.reader.book_children(module, parent, after, limit);
  });
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
std::future<void> StudyWorker::set_note_numbering_enabled(StudyTab tab, bool enabled) {
  return impl_->submit([tab, enabled](Impl::State &s) { s.tab(tab).set_note_numbering_enabled(enabled); });
}
std::future<void> StudyWorker::set_heading_note_links_enabled(StudyTab tab, bool enabled) {
  return impl_->submit([tab, enabled](Impl::State &s) { s.tab(tab).set_heading_note_links_enabled(enabled); });
}
std::future<void> StudyWorker::set_note_numbers(StudyTab tab, std::string module, bool enabled) {
  return impl_->submit([tab, module = std::move(module), enabled](Impl::State &s) { s.tab(tab).set_note_numbers(module, enabled); });
}
std::future<void> StudyWorker::set_lexical_layout_enabled(StudyTab tab, bool enabled) {
  return impl_->submit([tab, enabled](Impl::State &s) { s.tab(tab).set_lexical_layout_enabled(enabled); });
}
std::future<bool> StudyWorker::cross_reference_preview_list(StudyTab tab) {
  return impl_->submit([tab](Impl::State &s) { return s.tab(tab).cross_reference_preview_list(); });
}
std::future<StudyHoverLink> StudyWorker::hover_study_link_at(StudyTab tab, std::string current_module,
                                                         std::string current_reference, std::string uri) {
  if (!detail::study_link_arguments(current_module, current_reference, uri)) return rejected_study_link<StudyHoverLink>();
  return impl_->submit([tab, current_module = std::move(current_module), current_reference = std::move(current_reference),
                        uri = std::move(uri)](Impl::State &s) -> StudyHoverLink {
    auto &session = s.tab(tab);
    if (current_reference.empty() || uri.empty() || current_module.find('\0') != std::string::npos ||
        current_reference.find('\0') != std::string::npos || uri.find('\0') != std::string::npos)
      throw std::invalid_argument("Hover requires copied source context and a valid URI");
    if (uri.compare(0, 8, "sword://") == 0 || uri.compare(0, 8, "bible://") == 0) {
      const auto keyed = module_uri_link(uri);
      // Retained hover only finds the module separator in the first 20 bytes.
      // Empty declared modules do not use the main-module click fallback.
      if (keyed.module.empty() || keyed.module.size() >= 20) return std::monostate{};
      const auto *module = s.uri_module(keyed.module);
      if (!module || module->type != "Lexicons / Dictionaries") return std::monostate{};
      return session.read_dictionary(module->id, keyed.key);
    }
    const auto target = xiphos::core::contextual_hover_link(uri, current_module, current_reference);
    if (std::holds_alternative<PassageLink>(target) || std::holds_alternative<ReferenceListLink>(target) || std::holds_alternative<VerseImageLink>(target))
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
std::future<ModuleNameResolution> StudyWorker::resolve_module_name(StudyTab tab, std::string requested) {
  return impl_->submit([tab, requested = std::move(requested)](Impl::State &s) {
    (void)s.tab(tab);
    return s.reader.resolve_module_name(requested);
  });
}
std::future<SavedSearchLists> StudyWorker::read_search_lists(StudyTab tab, std::string path) {
  return impl_->submit([tab, path = std::move(path)](Impl::State &s) {
    (void)s.tab(tab);
    return read_saved_search_lists(path);
  });
}
std::future<std::vector<std::string>> StudyWorker::parse_search_module_list(StudyTab tab, std::string text) {
  return impl_->submit([tab, text = std::move(text)](Impl::State &s) {
    (void)s.tab(tab);
    return saved_search_module_names(text);
  });
}
std::future<SavedSearchLists> StudyWorker::save_search_list(StudyTab tab, std::string path, SavedSearchListEdit edit) {
  return impl_->submit([tab, path = std::move(path), edit = std::move(edit)](Impl::State &s) {
    (void)s.tab(tab);
    return save_saved_search_list(path, edit);
  });
}
std::future<SavedSearchLists> StudyWorker::remove_search_list(StudyTab tab, std::string path, SavedSearchListKind kind, std::string label) {
  return impl_->submit([tab, path = std::move(path), kind, label = std::move(label)](Impl::State &s) {
    (void)s.tab(tab);
    return remove_saved_search_list(path, kind, label);
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
  if (!detail::study_link_arguments(source_module, current_reference, uri, navigation_module)) return rejected_study_link<ContextualStudyLink>();
  return impl_->submit([tab, navigation_module = std::move(navigation_module), source_module = std::move(source_module),
                        current_reference = std::move(current_reference), uri = std::move(uri)](Impl::State &s) {
    return s.study_link_at(tab, source_module, current_reference, uri, true, navigation_module);
  });
}
std::future<ContextualStudyLink> StudyWorker::follow_study_link_at(StudyTab tab, std::string current_module, std::string current_reference, std::string uri) {
  if (!detail::study_link_arguments(current_module, current_reference, uri)) return rejected_study_link<ContextualStudyLink>();
  return impl_->submit([tab, current_module = std::move(current_module), current_reference = std::move(current_reference),
                        uri = std::move(uri)](Impl::State &s) -> ContextualStudyLink {
    return s.study_link_at(tab, current_module, current_reference, uri, true);
  });
}
std::future<ContextualStudyLink> StudyWorker::resolve_study_link_at(StudyTab tab, std::string current_module, std::string current_reference, std::string uri) {
  if (!detail::study_link_arguments(current_module, current_reference, uri)) return rejected_study_link<ContextualStudyLink>();
  return impl_->submit([tab, current_module = std::move(current_module), current_reference = std::move(current_reference),
                        uri = std::move(uri)](Impl::State &s) -> ContextualStudyLink {
    return s.study_link_at(tab, current_module, current_reference, uri, false);
  });
}
std::future<ResolvedStudyLink> StudyWorker::resolve_study_link(StudyTab tab, std::string current_module, std::string uri) {
  if (!detail::study_link_arguments(current_module, {}, uri)) return rejected_study_link<ResolvedStudyLink>();
  return impl_->submit([tab, current_module = std::move(current_module), uri = std::move(uri)](Impl::State &s) -> ResolvedStudyLink {
    s.tab(tab); // Both kinds require an open study context.
    if (current_module.find('\0') != std::string::npos)
      throw std::invalid_argument("Study link context contains NUL");
    const auto target = xiphos::core::study_link(uri, current_module);
    if (const auto *passage = std::get_if<PassageLink>(&target)) {
      const bool is_passage_uri = uri.compare(0, 8, "sword://") == 0 || uri.compare(0, 8, "bible://") == 0;
      if (is_passage_uri)
        return s.reader.resolve_link(current_module, uri);
      PassageLink result;
      dispatch_reference_command(passage->module.c_str(), current_module.c_str(), nullptr,
          passage->reference.c_str(), true, [&](const char *id) { return s.installed_module(id); },
          [&](const char *, const char *id, const char *reference) {
            // Commands use exact IDs including the fallback; never resolve an
            // abbreviation merely because the older API returns PassageLink.
            if (!s.installed_module(id)) throw std::out_of_range("Command fallback module is not installed");
            result = s.reader.resolve_link(id, xiphos::core::passage_uri(reference));
          });
      return result;
    }
    return s.note_target(tab, std::get<StudyNoteLink>(target));
  });
}
std::future<StudyNote> StudyWorker::study_note_link(StudyTab tab, std::string current_module, std::string uri) {
  if (!detail::study_link_arguments(current_module, {}, uri)) return rejected_study_link<StudyNote>();
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
std::future<VerseImageResource> StudyWorker::read_verse_image_link(std::string module,std::string uri,std::size_t maximum_bytes){
  return impl_->submit([module=std::move(module),uri=std::move(uri),maximum_bytes](Impl::State &s){
    return s.reader.read_verse_image_link(module,uri,maximum_bytes);
  });
}
std::future<VerseImageResource> StudyWorker::read_verse_inline_image(std::string module,std::string source,std::size_t maximum_bytes){
  return impl_->submit([module=std::move(module),source=std::move(source),maximum_bytes](Impl::State &s){
    return s.reader.read_verse_inline_image(module,source,maximum_bytes);
  });
}
std::future<EntryHtmlSource> StudyWorker::capture_entry_html(StudyTab tab,std::string module,std::string reference,HtmlSourceLimits limits){
  return impl_->submit([tab,module=std::move(module),reference=std::move(reference),limits](Impl::State &s){
    return s.tab(tab).capture_entry_html(module,reference,limits);
  });
}
std::future<Entry> StudyWorker::read_entry_snapshot(StudyTab tab,std::string module,std::string reference){
  return impl_->submit([tab,module=std::move(module),reference=std::move(reference)](Impl::State &s){
    return s.tab(tab).read_entry_snapshot(module,reference);
  });
}
std::future<Entry> StudyWorker::read_commentary_entry_snapshot(StudyTab tab,std::string module,std::string reference){
  return impl_->submit([tab,module=std::move(module),reference=std::move(reference)](Impl::State &s){
    return s.tab(tab).read_commentary_entry_snapshot(module,reference);
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
std::future<void> StudyWorker::set_double_space(StudyTab tab, std::string module, bool enabled) {
  return impl_->submit([tab, module = std::move(module), enabled](Impl::State &s) { s.tab(tab).set_double_space(module, enabled); });
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
  if (!detail::study_link_arguments(module, {}, uri)) return rejected_study_link<PassageLink>();
  return impl_->submit([module = std::move(module), uri = std::move(uri)](Impl::State &s) {
    return s.reader.resolve_link(module, uri);
  });
}
}
