// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "session.hpp"
#include <cstdint>
#include <future>
#include <memory>
namespace xiphos::core {
using StudyTab = std::uint64_t;
struct HistorySnapshot {
  std::vector<Location> entries;
  std::size_t cursor = 0;
  bool can_back = false, can_forward = false;
};
// Native hosts share one worker per process. Reader/session construction,
// operations and destruction all run on its single engine thread. This does
// not make unrelated SWORD instances or legacy global access thread-safe.
// Requests copy arguments/results and execute FIFO. Initialization/operation
// errors arrive through futures; failed jobs do not stop later jobs.
// Destruction drains accepted work and joins; do not destroy concurrently with
// submission, or wait on futures/destruction from a UI thread. No cancellation
// or interruption of a blocked engine call is promised by this initial API.
class StudyWorker {
public:
  explicit StudyWorker(std::string root);
  ~StudyWorker();
  StudyWorker(const StudyWorker &) = delete;
  StudyWorker &operator=(const StudyWorker &) = delete;
  std::future<std::vector<Module>> modules();
  std::future<StudyTab> create_tab();
  std::future<void> close_tab(StudyTab tab);
  std::future<StudyNote> study_note_link(StudyTab tab, std::string current_module, std::string uri);
  std::future<StudyNote> study_note(StudyTab tab, std::string module, std::string reference, std::string identifier);
  std::future<Entry> read_entry(StudyTab tab, std::string module, std::string reference);
  std::future<Entry> visit(StudyTab tab, std::string module, std::string reference);
  std::future<Entry> navigate(StudyTab tab, int direction);
  std::future<Entry> navigate_chapter(StudyTab tab, std::string module, std::string reference, int direction);
  std::future<void> set_engine_options(StudyTab tab, EngineOptions options);
  std::future<void> clear_engine_options(StudyTab tab);
  std::future<void> enable_module_preferences(StudyTab tab);
  std::future<void> import_module_preferences(StudyTab tab, std::string path);
  std::future<void> save_module_preferences(StudyTab tab, std::string path);
  std::future<void> set_module_engine_options(StudyTab tab, std::string module, EngineOptions options);
  std::future<void> clear_module_engine_options(StudyTab tab, std::string module);
  std::future<std::optional<EngineOptions>> module_engine_options(StudyTab tab, std::string module);
  std::future<Chapter> read_chapter(StudyTab tab, std::string module, std::string reference, DisplayOptions options = {});
  std::future<HistorySnapshot> history(StudyTab tab);
  std::future<void> clear_history(StudyTab tab);
  std::future<Chapter> read_chapter(std::string module, std::string reference, DisplayOptions options = {});
  std::future<ReferenceChoices> reference_choices(std::string module, std::string reference);
  std::future<PassageLink> resolve_link(std::string module, std::string uri);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
