// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <memory>
#include <stdexcept>
#include <thread>
namespace xiphos::linux_preview {
namespace {
std::string copy(const char *value) { return value ? value : ""; }
std::string message(xc_status status) {
  switch (status) {
  case XC_MODULE_NOT_FOUND: return "The selected module is unavailable.";
  case XC_INVALID_ARGUMENT: return "The requested passage or action is unavailable.";
  default: return "The study engine could not complete this action.";
  }
}
}
ReaderModel::ReaderModel(const char *root) {
  auto status = xc_worker_open(root, &worker_);
  if (status != XC_OK) throw std::runtime_error(message(status));
  xc_request *request = nullptr;
  status = xc_worker_modules(worker_, &request);
  accept(status, request, Operation::catalog);
}
ReaderModel::~ReaderModel() {
  // Fallback for non-UI callers. GTK explicitly transfers ownership to GTask.
  if (auto *worker = detach()) std::thread([worker] { xc_worker_close(worker); }).join();
}
xc_worker *ReaderModel::detach() {
  closed_ = true; ready_ = false; back_ = forward_ = false;
  xc_request_release(request_); request_ = nullptr;
  auto *worker = worker_; worker_ = nullptr;
  return worker;
}
bool ReaderModel::accept(xc_status status, xc_request *request, Operation operation) {
  if (status != XC_OK || !request) {
    xc_request_release(request); error_ = message(status); ++revision_; return false;
  }
  request_ = request; operation_ = operation; error_.clear(); ++revision_; return true;
}
bool ReaderModel::visit(const std::string &bible, const std::string &commentary, const std::string &reference) {
  if (!ready() || busy()) return false;
  xc_request *request = nullptr;
  auto status = xc_worker_visit_passage(worker_, tab_, bible.c_str(), commentary.c_str(), reference.c_str(), &request);
  return accept(status, request, Operation::passage);
}
bool ReaderModel::navigate(int direction) {
  if (!ready() || busy()) return false;
  xc_request *request = nullptr;
  auto status = xc_worker_navigate_passage(worker_, tab_, direction, &request);
  return accept(status, request, Operation::passage);
}
bool ReaderModel::navigate_chapter(int direction) {
  if (!ready() || busy() || passage_.bible.empty()) return false;
  xc_request *request = nullptr;
  auto status = xc_worker_navigate_passage_chapter(worker_, tab_, passage_.bible.c_str(), passage_.commentary.c_str(), passage_.reference.c_str(), direction, &request);
  return accept(status, request, Operation::passage);
}
bool ReaderModel::clear_history() {
  if (!ready() || busy()) return false;
  xc_request *request = nullptr; auto status = xc_worker_clear_history(worker_, tab_, &request);
  return accept(status, request, Operation::clear);
}
bool ReaderModel::refresh() {
  if (!ready() || busy() || passage_.bible.empty()) return false;
  xc_request *request = nullptr;
  auto status = xc_worker_read_passage(worker_, tab_, passage_.bible.c_str(), passage_.commentary.c_str(), passage_.reference.c_str(), &request);
  return accept(status, request, Operation::passage);
}
bool ReaderModel::set_display(bool chapter, unsigned flags) {
  if (!ready() || busy() || (flags & ~(XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED))) return false;
  show_chapter_ = chapter; display_flags_ = flags;
  if (passage_.bible.empty()) { ++revision_; return true; }
  return refresh();
}
bool ReaderModel::set_commentary_chapter(bool enabled) {
  if (!ready() || busy() || passage_.commentary.empty()) return false;
  xc_request *request = nullptr;
  auto status = xc_worker_set_commentary_chapter_mode(worker_, tab_, passage_.commentary.c_str(), enabled, &request);
  return accept(status, request, Operation::refresh);
}
void ReaderModel::load_choices() {
  xc_request *request = nullptr;
  auto status = xc_worker_reference_choices(worker_, passage_.bible.c_str(), passage_.reference.c_str(), &request);
  accept(status, request, Operation::choices);
}
void ReaderModel::load_chapters(bool commentary) {
  xc_request *request = nullptr;
  if (!commentary && show_chapter_) {
    auto status = xc_worker_read_tab_chapter(worker_, tab_, passage_.bible.c_str(), passage_.reference.c_str(), display_flags_, &request);
    accept(status, request, Operation::bible_chapter);
  } else if (!passage_.commentary.empty() && show_commentary_chapter_) {
    auto status = xc_worker_read_tab_commentary_chapter(worker_, tab_, passage_.commentary.c_str(), passage_.reference.c_str(), display_flags_, &request);
    accept(status, request, Operation::commentary_chapter);
  } else load_choices();
}
bool ReaderModel::follow(const std::string &source, const std::string &uri) {
  if (!ready() || busy() || passage_.bible.empty()) return false;
  xc_request *request = nullptr;
  auto status = xc_worker_follow_pane_link_at(worker_, tab_, passage_.bible.c_str(), source.c_str(), passage_.reference.c_str(), uri.c_str(), &request);
  return accept(status, request, Operation::link);
}
void ReaderModel::history() {
  xc_request *request = nullptr;
  auto status = xc_worker_history(worker_, tab_, &request);
  accept(status, request, Operation::history);
}
void ReaderModel::tick() {
  if (!request_ || closed_) return;
  const auto status = xc_request_poll(request_);
  if (status == XC_PENDING) return;
  std::unique_ptr<xc_request, decltype(&xc_request_release)> completed(request_, xc_request_release);
  request_ = nullptr; ++revision_;
  if (status != XC_OK) { error_ = message(status); return; }
  xc_request *next = nullptr;
  switch (operation_) {
  case Operation::catalog: {
    std::unique_ptr<xc_catalog, decltype(&xc_catalog_release)> catalog(xc_request_take_catalog(completed.get()), xc_catalog_release);
    if (!catalog) { error_ = "Missing module catalog."; return; }
    for (size_t i = 0; i < xc_catalog_count(catalog.get()); ++i)
      modules_.push_back({copy(xc_catalog_id(catalog.get(), i)), copy(xc_catalog_description(catalog.get(), i)), copy(xc_catalog_type(catalog.get(), i))});
    auto submitted = xc_worker_create_tab(worker_, &next); accept(submitted, next, Operation::tab); break;
  }
  case Operation::tab: {
    tab_ = xc_request_tab(completed.get());
    if (!tab_) { error_ = "Could not create study tab."; return; }
    auto submitted = xc_worker_enable_module_preferences(worker_, tab_, &next);
    accept(submitted, next, Operation::preferences); break;
  }
  case Operation::preferences: ready_ = true; break;
  case Operation::passage: {
    std::unique_ptr<xc_passage, decltype(&xc_passage_release)> result(xc_request_take_passage(completed.get()), xc_passage_release);
    if (!result) { error_ = "Missing passage snapshot."; return; }
    Passage value{copy(xc_passage_module(result.get())), copy(xc_passage_commentary_module(result.get())), copy(xc_passage_reference(result.get())), copy(xc_passage_html(result.get())), ""};
    if (xc_passage_has_commentary(result.get()))
      value.commentary_html = copy(xc_passage_commentary_heading_html(result.get())) + copy(xc_passage_commentary_html(result.get()));
    passage_ = std::move(value); bible_chapter_ = {}; commentary_chapter_ = {}; history(); break;
  }
  case Operation::history: {
    std::unique_ptr<xc_history, decltype(&xc_history_release)> result(xc_request_take_history(completed.get()), xc_history_release);
    if (!result) { error_ = "Missing history snapshot."; return; }
    back_ = xc_history_can_back(result.get()); forward_ = xc_history_can_forward(result.get());
    if (passage_.bible.empty()) break;
    if (!passage_.commentary.empty()) {
      auto submitted = xc_worker_module_presentation_options(worker_, tab_, passage_.commentary.c_str(), &next);
      accept(submitted, next, Operation::presentation);
    } else { show_commentary_chapter_ = false; load_chapters(); }
    break;
  }
  case Operation::refresh: refresh(); break;
  case Operation::clear: history(); break;
  case Operation::presentation:
    show_commentary_chapter_ = xc_request_commentary_chapter_mode(completed.get()) == 1;
    load_chapters(); break;
  case Operation::bible_chapter:
  case Operation::commentary_chapter: {
    std::unique_ptr<xc_chapter, decltype(&xc_chapter_release)> chapter(xc_request_take_chapter(completed.get()), xc_chapter_release);
    if (!chapter) { error_ = "Missing chapter snapshot."; break; }
    Chapter value{copy(xc_chapter_html(chapter.get())), copy(xc_chapter_selected_anchor(chapter.get())), copy(xc_chapter_title(chapter.get()))};
    if (operation_ == Operation::bible_chapter) { bible_chapter_ = std::move(value); load_chapters(true); }
    else { commentary_chapter_ = std::move(value); load_choices(); }
    break;
  }
  case Operation::choices: {
    std::unique_ptr<xc_choices, decltype(&xc_choices_release)> choices(xc_request_take_choices(completed.get()), xc_choices_release);
    if (!choices) { error_ = "Missing reference choices."; break; }
    for (int k = 0; k < 3; ++k) {
      choices_[k].clear(); const auto kind = static_cast<xc_choice_kind>(k);
      for (size_t i = 0; i < xc_choices_count(choices.get(), kind); ++i)
        choices_[k].push_back({copy(xc_choices_label(choices.get(), kind, i)), copy(xc_choices_reference(choices.get(), kind, i))});
      choice_labels_[k] = copy(xc_choices_current_label(choices.get(), kind));
    }
    break;
  }
  case Operation::link: {
    std::unique_ptr<xc_link, decltype(&xc_link_release)> link(xc_request_take_link(completed.get()), xc_link_release);
    if (!link) { error_ = "This preview currently supports passage links only."; return; }
    const auto module = copy(xc_link_module(link.get())), reference = copy(xc_link_reference(link.get()));
    auto submitted = xc_worker_visit_linked_passage(worker_, tab_, passage_.bible.c_str(), passage_.commentary.c_str(), module.c_str(), reference.c_str(), &next);
    accept(submitted, next, Operation::passage); break;
  }
  }
}
}
