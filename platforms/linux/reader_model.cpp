// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <memory>
#include <stdexcept>
#include <thread>
namespace xiphos::linux_preview {
namespace {
std::string copy(const char *value) { return value ? value : ""; }
template<class Getter> std::optional<LexicalPresentation> copy_layout(Getter getter) {
  xc_lexical_presentation value{};
  if (getter(&value) != XC_OK || !value.css) return std::nullopt;
  return LexicalPresentation{value.css, value.requested != 0, value.applied != 0};
}
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
ReaderModel::ReaderModel(xc_worker &worker) : worker_(&worker), owns_worker_(false) {
  xc_request *request = nullptr; auto status = xc_worker_modules(worker_, &request);
  accept(status, request, Operation::catalog);
}
ReaderModel::~ReaderModel() {
  // Fallback for non-UI callers. GTK explicitly transfers ownership to GTask.
  if (auto *worker = detach()) std::thread([worker] { xc_worker_close(worker); }).join();
}
xc_worker *ReaderModel::detach() {
  closed_ = true; ready_ = false; back_ = forward_ = false;
  xc_request_release(hover_request_); hover_request_ = nullptr;
  xc_request_release(request_); request_ = nullptr;
  auto *worker = worker_; worker_ = nullptr;
  if (!owns_worker_) {
    if (worker && tab_) {
      xc_request *request = nullptr; xc_worker_close_tab(worker, tab_, &request); xc_request_release(request);
    }
    return nullptr;
  }
  return worker;
}
bool ReaderModel::accept(xc_status status, xc_request *request, Operation operation) {
  if (status != XC_OK || !request) {
    xc_request_release(request); error_ = message(status); ++revision_; return false;
  }
  xc_request_release(hover_request_); hover_request_ = nullptr;
  request_ = request; operation_ = operation; error_.clear(); notice_.clear(); ++revision_; return true;
}
bool ReaderModel::visit(const std::string &bible, const std::string &commentary, const std::string &reference) {
  if (!ready() || busy()) return false;
  xc_request *request = nullptr;
  const auto *selected = module(bible);
  if (selected && selected->type == "Commentaries") {
    pending_module_ = bible;
    auto status = xc_worker_visit(worker_, tab_, bible.c_str(), reference.c_str(), &request);
    return accept(status, request, Operation::entry);
  }
  auto status = xc_worker_visit_passage(worker_, tab_, bible.c_str(), commentary.c_str(), reference.c_str(), &request);
  return accept(status, request, Operation::passage);
}
bool ReaderModel::choose_commentary(const std::string &module) {
  const auto *main = this->module(passage_.bible);
  if (!ready() || busy() || !main || main->type != "Biblical Texts") return false;
  xc_request *request = nullptr;
  auto status = xc_worker_read_passage(worker_, tab_, passage_.bible.c_str(), module.c_str(), passage_.reference.c_str(), &request);
  return accept(status, request, Operation::passage);
}
bool ReaderModel::navigate(int direction) {
  if (!ready() || busy()) return false;
  xc_request *request = nullptr;
  auto status = xc_worker_navigate_passage(worker_, tab_, direction, &request);
  return accept(status, request, Operation::passage);
}
bool ReaderModel::set_lexicons(const Lexicons &names) {
  if (!ready() || busy()) return false;
  for (const auto &name : names) if (name && name->find('\0') != std::string::npos) return false;
  auto value = [&](size_t i) { return names[i] ? names[i]->c_str() : nullptr; };
  const xc_lexicon_names preferences{value(0), value(1), value(2), value(3), value(4)};
  xc_request *request = nullptr; auto status = xc_worker_set_lexicon_preferences(worker_, tab_, &preferences, &request);
  return accept(status, request, Operation::lexicon_changed);
}
bool ReaderModel::lexicon_profile(const std::string &path, bool save) {
  if (!ready() || busy()) return false;
  xc_request *request = nullptr;
  auto status = save ? xc_worker_save_lexicon_profile(worker_, tab_, path.c_str(), &request)
    : xc_worker_import_lexicon_profile(worker_, tab_, path.c_str(), &request);
  return accept(status, request, save ? Operation::saved : Operation::lexicons);
}
bool ReaderModel::set_cross_reference_list(bool enabled) {
  if (!ready() || busy()) return false;
  xc_request *request = nullptr; auto status = xc_worker_set_cross_reference_preview_list(worker_, tab_, enabled, &request);
  return accept(status, request, Operation::reference_preference_changed);
}
bool ReaderModel::hover(const std::string &uri) {
  if (!ready() || busy() || passage_.bible.empty() || uri.empty() || uri.size() > 32768) return false;
  xc_request_release(hover_request_); hover_request_ = nullptr;
  return xc_worker_hover_study_link_at(worker_, tab_, passage_.bible.c_str(), passage_.reference.c_str(), uri.c_str(), &hover_request_) == XC_OK;
}
void ReaderModel::poll_hover() {
  if (!hover_request_ || closed_) return;
  const auto status = xc_request_poll(hover_request_); if (status == XC_PENDING) return;
  std::unique_ptr<xc_request, decltype(&xc_request_release)> completed(hover_request_, xc_request_release);
  hover_request_ = nullptr;
  if (status == XC_OK && !xc_request_no_presentation(completed.get())) { take_preview(completed.get(), true); ++revision_; }
}
bool ReaderModel::show_references() {
  if (!ready() || busy() || note_.expression.empty()) return false;
  xc_request *request = nullptr;
  auto status = xc_worker_read_reference_list(worker_, tab_, note_.module.c_str(), note_.expression.c_str(), note_.reference.c_str(), &request);
  return accept(status, request, Operation::references);
}
bool ReaderModel::show_note() {
  if (!ready() || busy()) return false;
  showing_references_ = false; ++revision_; return true;
}
bool ReaderModel::close_preview() {
  if (!ready() || busy()) return false;
  xc_request_release(hover_request_); hover_request_ = nullptr;
  note_ = {}; references_ = {}; showing_references_ = false; ++revision_; return true;
}
bool ReaderModel::take_preview(xc_request *request, bool hover) {
  std::unique_ptr<xc_dictionary_entry, decltype(&xc_dictionary_entry_release)> dictionary(xc_request_take_dictionary_entry(request), xc_dictionary_entry_release);
  if (dictionary) {
    if (hover) information_ = {copy(xc_dictionary_entry_module(dictionary.get())), copy(xc_dictionary_entry_key(dictionary.get())), copy(xc_dictionary_entry_html(dictionary.get())), ""};
    else {
      dictionary_ = {copy(xc_dictionary_entry_module(dictionary.get())), copy(xc_dictionary_entry_key(dictionary.get())), copy(xc_dictionary_entry_html(dictionary.get())), "", ""};
      xc_request *next = nullptr;
      const auto status = xc_worker_dictionary_choices(worker_, tab_, dictionary_.module.c_str(), dictionary_.key.c_str(), &next);
      accept(status, next, Operation::dictionary_choices);
    }
    return true;
  }
  std::unique_ptr<xc_note, decltype(&xc_note_release)> note(xc_request_take_note(request), xc_note_release);
  if (note) {
    Preview value{copy(xc_note_module(note.get())), copy(xc_note_reference(note.get())), copy(xc_note_html(note.get())), copy(xc_note_reference_list(note.get()))};
    if (hover) information_ = std::move(value); else { note_ = std::move(value); references_ = {}; showing_references_ = false; }
    return true;
  }
  std::unique_ptr<xc_reference_list, decltype(&xc_reference_list_release)> references(xc_request_take_reference_list(request), xc_reference_list_release);
  if (references) {
    references_ = {copy(xc_reference_list_module(references.get())), copy(xc_reference_list_context(references.get())), copy(xc_reference_list_html(references.get())), copy(xc_reference_list_expression(references.get())), xc_reference_list_count(references.get()), xc_reference_list_limited(references.get()) != 0, xc_reference_list_skipped(references.get())};
    if (hover || operation_ != Operation::references) note_ = {};
    showing_references_ = true; return true;
  }
  std::unique_ptr<xc_lexical_entry, decltype(&xc_lexical_entry_release)> lexical(xc_request_take_lexical_entry(request), xc_lexical_entry_release);
  if (lexical) {
    if (hover) information_ = {copy(xc_lexical_entry_module(lexical.get())), copy(xc_lexical_entry_source_reference(lexical.get())), copy(xc_lexical_entry_dictionary_html(lexical.get())), ""};
    else {
      dictionary_ = {copy(xc_lexical_entry_dictionary_module(lexical.get())), copy(xc_lexical_entry_dictionary_key(lexical.get())), copy(xc_lexical_entry_dictionary_html(lexical.get())), copy(xc_lexical_entry_module(lexical.get())), copy(xc_lexical_entry_source_reference(lexical.get()))};
      xc_request *next = nullptr;
      auto status = xc_worker_dictionary_choices(worker_, tab_, dictionary_.module.c_str(), dictionary_.key.c_str(), &next);
      accept(status, next, Operation::dictionary_choices);
    }
    return true;
  }
  return false;
}
const Module *ReaderModel::module(const std::string &id) const {
  for (const auto &value : modules_) if (value.id == id) return &value;
  return nullptr;
}
bool ReaderModel::set_filter(unsigned flag, bool enabled) {
  const auto *selected = module(passage_.bible);
  if (!ready() || busy() || !engine_configured_ || !selected || !flag || (flag & (flag - 1)) || !(selected->declared_flags & flag)) return false;
  xc_request *request = nullptr;
  const auto flags = enabled ? engine_flags_ | flag : engine_flags_ & ~flag;
  auto status = xc_worker_set_module_engine_options(worker_, tab_, passage_.bible.c_str(), flags, &request);
  return accept(status, request, Operation::refresh);
}
bool ReaderModel::select_variant(int choice) {
  if (!ready() || busy() || passage_.bible.empty()) return false;
  xc_request *request = nullptr;
  auto status = xc_worker_select_module_variant(worker_, tab_, passage_.bible.c_str(), choice, &request);
  return accept(status, request, Operation::refresh);
}
bool ReaderModel::set_note_numbers(bool commentary, bool enabled) {
  const auto &module = commentary ? passage_.commentary : passage_.bible;
  if (!ready() || busy() || module.empty() || presentation(commentary).note_numbers < 0) return false;
  xc_request *request = nullptr;
  auto status = xc_worker_set_note_numbers(worker_, tab_, module.c_str(), enabled, &request);
  return accept(status, request, Operation::refresh);
}
bool ReaderModel::set_double_space(bool commentary, bool enabled) {
  const auto &module = commentary ? passage_.commentary : passage_.bible;
  if (!ready() || busy() || module.empty()) return false;
  xc_request *request = nullptr;
  auto status = xc_worker_set_double_space(worker_, tab_, module.c_str(), enabled, &request);
  return accept(status, request, Operation::refresh);
}
bool ReaderModel::set_content(bool commentary, bool font_faces, bool enabled) {
  const auto &module = commentary ? passage_.commentary : passage_.bible;
  const auto &options = presentation(commentary);
  if (!ready() || busy() || module.empty() || (font_faces ? options.font_faces : options.images) < 0) return false;
  xc_request *request = nullptr;
  auto status = font_faces ? xc_worker_set_respect_font_faces(worker_, tab_, module.c_str(), enabled, &request)
    : xc_worker_set_image_content(worker_, tab_, module.c_str(), enabled, &request);
  return accept(status, request, Operation::refresh);
}
bool ReaderModel::set_commentary_headings(bool enabled) {
  if (!ready() || busy() || passage_.commentary.empty()) return false;
  xc_request *request = nullptr;
  auto status = xc_worker_set_headings(worker_, tab_, passage_.commentary.c_str(), enabled, &request);
  return accept(status, request, Operation::refresh);
}
bool ReaderModel::reading_profile(const std::string &path, bool save) {
  if (!ready() || busy()) return false;
  xc_request *request = nullptr;
  auto status = save ? xc_worker_save_module_preferences(worker_, tab_, path.c_str(), &request)
    : xc_worker_import_module_preferences(worker_, tab_, path.c_str(), &request);
  return accept(status, request, save ? Operation::saved : Operation::refresh);
}
bool ReaderModel::lookup_dictionary(const std::string &module, const std::string &key, bool record) {
  if (!ready() || busy()) return false;
  xc_request *request = nullptr;
  dictionary_source_module_ = passage_.bible; dictionary_source_reference_ = passage_.reference;
  auto status = record ? xc_worker_visit_dictionary(worker_, tab_, module.c_str(), key.c_str(), &request)
    : xc_worker_read_dictionary(worker_, tab_, module.c_str(), key.c_str(), &request);
  return accept(status, request, Operation::dictionary);
}
bool ReaderModel::step_dictionary(int direction) {
  if (!ready() || busy() || dictionary_.module.empty()) return false;
  xc_request *request = nullptr;
  dictionary_source_module_ = dictionary_.source_module; dictionary_source_reference_ = dictionary_.source_reference;
  auto status = xc_worker_navigate_dictionary(worker_, tab_, dictionary_.module.c_str(), dictionary_.key.c_str(), direction, &request);
  return accept(status, request, Operation::dictionary);
}
bool ReaderModel::navigate_dictionary_history(int direction) {
  if (!ready() || busy()) return false;
  dictionary_source_module_ = passage_.bible; dictionary_source_reference_ = passage_.reference;
  xc_request *request = nullptr; auto status = xc_worker_navigate_dictionary_history(worker_, tab_, direction, &request);
  return accept(status, request, Operation::dictionary);
}
bool ReaderModel::clear_dictionary_history() {
  if (!ready() || busy()) return false;
  xc_request *request = nullptr; auto status = xc_worker_clear_dictionary_history(worker_, tab_, &request);
  return accept(status, request, Operation::dictionary_clear);
}
bool ReaderModel::close_dictionary() {
  if (!ready() || busy()) return false;
  dictionary_ = {}; dictionary_keys_.clear(); ++revision_; return true;
}
bool ReaderModel::navigate_chapter(int direction) {
  if (!ready() || busy() || passage_.bible.empty()) return false;
  xc_request *request = nullptr;
  const auto *selected = module(passage_.bible);
  if (selected && selected->type == "Commentaries") {
    pending_module_ = passage_.bible;
    auto status = xc_worker_navigate_chapter(worker_, tab_, passage_.bible.c_str(), passage_.reference.c_str(), direction, &request);
    return accept(status, request, Operation::entry);
  }
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
  const auto *selected = module(passage_.bible);
  if (selected && selected->type == "Commentaries") {
    pending_module_ = passage_.bible;
    auto status = xc_worker_read_tab_entry(worker_, tab_, passage_.bible.c_str(), passage_.reference.c_str(), &request);
    return accept(status, request, Operation::entry);
  }
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
bool ReaderModel::follow(const std::string &source, const std::string &uri, const std::string &context) {
  if (!ready() || busy() || passage_.bible.empty()) return false;
  xc_request *request = nullptr;
  dictionary_source_module_ = source; dictionary_source_reference_ = context.empty() ? passage_.reference : context;
  auto status = xc_worker_follow_pane_link_at(worker_, tab_, passage_.bible.c_str(), source.c_str(), (context.empty() ? passage_.reference : context).c_str(), uri.c_str(), &request);
  return accept(status, request, Operation::link);
}
void ReaderModel::history() {
  xc_request *request = nullptr;
  auto status = xc_worker_history(worker_, tab_, &request);
  accept(status, request, Operation::history);
}
void ReaderModel::tick() {
  poll_hover();
  if (!request_ || closed_) return;
  const auto status = xc_request_poll(request_);
  if (status == XC_PENDING) return;
  std::unique_ptr<xc_request, decltype(&xc_request_release)> completed(request_, xc_request_release);
  request_ = nullptr; ++revision_;
  if (status != XC_OK) { error_ = message(status); return; }
  xc_request *next = nullptr;
  if (take_extended(completed.get())) return;
  switch (operation_) {
  default: error_ = "Unknown native operation."; break;
  case Operation::catalog: {
    std::unique_ptr<xc_catalog, decltype(&xc_catalog_release)> catalog(xc_request_take_catalog(completed.get()), xc_catalog_release);
    if (!catalog) { error_ = "Missing module catalog."; return; }
    for (size_t i = 0; i < xc_catalog_count(catalog.get()); ++i)
      modules_.push_back({copy(xc_catalog_id(catalog.get(), i)), copy(xc_catalog_description(catalog.get(), i)), copy(xc_catalog_type(catalog.get(), i)), xc_catalog_is_dictionary(catalog.get(), i) != 0, xc_catalog_declared_engine_flags(catalog.get(), i), xc_catalog_is_regular_commentary(catalog.get(), i) != 0, xc_catalog_has_headings(catalog.get(), i) != 0, xc_catalog_lexicon_roles(catalog.get(), i), copy(xc_catalog_language(catalog.get(), i)), xc_catalog_has_book_tree(catalog.get(), i) != 0, xc_catalog_has_verse_search(catalog.get(), i) != 0});
    auto submitted = xc_worker_create_tab(worker_, &next); accept(submitted, next, Operation::tab); break;
  }
  case Operation::tab: {
    tab_ = xc_request_tab(completed.get());
    if (!tab_) { error_ = "Could not create study tab."; return; }
    auto submitted = xc_worker_set_lexical_layout_enabled(worker_, tab_, 1, &next);
    accept(submitted, next, Operation::lexical_layout); break;
  }
  case Operation::lexical_layout: {
    auto submitted = xc_worker_set_note_numbering_enabled(worker_, tab_, 1, &next);
    accept(submitted, next, Operation::note_numbering); break;
  }
  case Operation::note_numbering: {
    auto submitted = xc_worker_set_heading_note_links_enabled(worker_, tab_, 1, &next);
    accept(submitted, next, Operation::heading_note_links); break;
  }
  case Operation::heading_note_links: {
    auto submitted = xc_worker_enable_module_preferences(worker_, tab_, &next);
    accept(submitted, next, Operation::preferences); break;
  }
  case Operation::preferences: {
    auto submitted = xc_worker_enable_lexicon_preferences(worker_, tab_, &next);
    accept(submitted, next, Operation::enable_lexicons); break;
  }
  case Operation::enable_lexicons:
  case Operation::lexicon_changed: {
    auto submitted = xc_worker_lexicon_preferences(worker_, tab_, &next);
    accept(submitted, next, Operation::lexicons); break;
  }
  case Operation::lexicons: {
    std::unique_ptr<xc_lexicons, decltype(&xc_lexicons_release)> preferences(xc_request_take_lexicons(completed.get()), xc_lexicons_release);
    if (!preferences) { error_ = "Missing lexicon preferences."; break; }
    const char *names[] = {xc_lexicons_greek(preferences.get()), xc_lexicons_hebrew(preferences.get()), xc_lexicons_greek_morph_nt(preferences.get()), xc_lexicons_greek_morph_ot(preferences.get()), xc_lexicons_hebrew_morph(preferences.get())};
    for (size_t i = 0; i < 5; ++i) lexicons_[i] = names[i] ? std::optional<std::string>(names[i]) : std::nullopt;
    auto submitted = xc_worker_cross_reference_preview_list(worker_, tab_, &next);
    accept(submitted, next, Operation::reference_preference); break;
  }
  case Operation::reference_preference_changed: {
    auto submitted = xc_worker_cross_reference_preview_list(worker_, tab_, &next);
    accept(submitted, next, Operation::reference_preference); break;
  }
  case Operation::reference_preference: {
    int enabled = 1; xc_request_cross_reference_preview_list(completed.get(), &enabled);
    cross_reference_list_ = enabled != 0; ready_ = true; break;
  }
  case Operation::references: take_preview(completed.get(), false); break;
  case Operation::entry: {
    std::unique_ptr<xc_entry, decltype(&xc_entry_release)> result(xc_request_take_entry(completed.get()), xc_entry_release);
    if (!result) { error_ = "Missing entry snapshot."; break; }
    const auto layout = copy_layout([&](auto *out) { return xc_entry_lexical_layout(result.get(), out); });
    if (!layout) { error_ = "Missing layout snapshot."; break; }
    passage_ = {pending_module_, "", copy(xc_entry_reference(result.get())), copy(xc_entry_html(result.get())), "", *layout, {}};
    bible_chapter_ = {}; commentary_chapter_ = {}; note_ = {}; references_ = {}; showing_references_ = false;
    history(); break;
  }
  case Operation::passage: {
    std::unique_ptr<xc_passage, decltype(&xc_passage_release)> result(xc_request_take_passage(completed.get()), xc_passage_release);
    if (!result) { error_ = "Missing passage snapshot."; return; }
    Passage value{copy(xc_passage_module(result.get())), copy(xc_passage_commentary_module(result.get())), copy(xc_passage_reference(result.get())), copy(xc_passage_html(result.get())), ""};
    if (const auto *anchor = xc_passage_anchor(result.get())) {
      value.anchor = anchor; value.anchor_module = copy(xc_passage_anchor_module(result.get()));
      value.anchor_revision = ++anchor_revision_;
    }
    const auto main_layout = copy_layout([&](auto *out) { return xc_passage_lexical_layout(result.get(), out); });
    if (!main_layout) { error_ = "Missing layout snapshot."; break; }
    value.bible_lexical = *main_layout;
    if (xc_passage_has_commentary(result.get())) {
      value.commentary_html = copy(xc_passage_commentary_heading_html(result.get())) + copy(xc_passage_commentary_html(result.get()));
      const auto commentary_layout = copy_layout([&](auto *out) { return xc_passage_commentary_lexical_layout(result.get(), out); });
      if (!commentary_layout) { error_ = "Missing commentary layout snapshot."; break; }
      value.commentary_lexical = *commentary_layout;
    }
    passage_ = std::move(value); note_ = {}; references_ = {}; showing_references_ = false; bible_chapter_ = {}; commentary_chapter_ = {}; history(); break;
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
  case Operation::dictionary: {
    std::unique_ptr<xc_dictionary_entry, decltype(&xc_dictionary_entry_release)> result(xc_request_take_dictionary_entry(completed.get()), xc_dictionary_entry_release);
    if (!result) { error_ = "Missing dictionary snapshot."; break; }
    dictionary_ = {copy(xc_dictionary_entry_module(result.get())), copy(xc_dictionary_entry_key(result.get())), copy(xc_dictionary_entry_html(result.get())), dictionary_source_module_, dictionary_source_reference_};
    auto submitted = xc_worker_dictionary_choices(worker_, tab_, dictionary_.module.c_str(), dictionary_.key.c_str(), &next);
    accept(submitted, next, Operation::dictionary_choices); break;
  }
  case Operation::dictionary_choices: {
    std::unique_ptr<xc_dictionary_choices, decltype(&xc_dictionary_choices_release)> result(xc_request_take_dictionary_choices(completed.get()), xc_dictionary_choices_release);
    if (!result) { error_ = "Missing dictionary choices."; break; }
    dictionary_keys_.clear();
    for (size_t i = 0; i < xc_dictionary_choices_count(result.get()); ++i) dictionary_keys_.push_back(copy(xc_dictionary_choices_at(result.get(), i)));
    auto submitted = xc_worker_dictionary_history(worker_, tab_, &next);
    accept(submitted, next, Operation::dictionary_history); break;
  }
  case Operation::dictionary_clear: {
    auto submitted = xc_worker_dictionary_history(worker_, tab_, &next);
    accept(submitted, next, Operation::dictionary_history); break;
  }
  case Operation::dictionary_history: {
    std::unique_ptr<xc_dictionary_history, decltype(&xc_dictionary_history_release)> result(xc_request_take_dictionary_history(completed.get()), xc_dictionary_history_release);
    if (!result) { error_ = "Missing dictionary history."; break; }
    dictionary_back_ = xc_dictionary_history_can_back(result.get()); dictionary_forward_ = xc_dictionary_history_can_forward(result.get());
    dictionary_history_count_ = xc_dictionary_history_count(result.get()); break;
  }
  case Operation::saved: notice_ = "Preferences saved."; break;
  case Operation::refresh: if (!refresh()) notice_ = "Preferences updated."; break;
  case Operation::clear: history(); break;
  case Operation::presentation:
    show_commentary_chapter_ = xc_request_commentary_chapter_mode(completed.get()) == 1;
    load_chapters(); break;
  case Operation::bible_chapter:
  case Operation::commentary_chapter: {
    std::unique_ptr<xc_chapter, decltype(&xc_chapter_release)> chapter(xc_request_take_chapter(completed.get()), xc_chapter_release);
    if (!chapter) { error_ = "Missing chapter snapshot."; break; }
    Chapter value{copy(xc_chapter_html(chapter.get())), copy(xc_chapter_selected_anchor(chapter.get())), copy(xc_chapter_title(chapter.get()))};
    const auto layout = copy_layout([&](auto *out) { return xc_chapter_lexical_layout(chapter.get(), out); });
    if (!layout) { error_ = "Missing chapter layout snapshot."; break; }
    value.lexical = *layout;
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
    auto submitted = xc_worker_module_engine_options(worker_, tab_, passage_.bible.c_str(), &next);
    accept(submitted, next, Operation::engine_options); break;
  }
  case Operation::engine_options: {
    int configured = 0;
    if (xc_request_engine_options(completed.get(), &engine_flags_, &configured) != XC_OK) { error_ = "Missing engine preferences."; break; }
    engine_configured_ = configured != 0;
    auto submitted = xc_worker_module_presentation_options(worker_, tab_, passage_.bible.c_str(), &next);
    accept(submitted, next, Operation::bible_options); break;
  }
  case Operation::bible_options:
  case Operation::commentary_options: {
    const int index = operation_ == Operation::bible_options ? 0 : 1;
    const auto spacing = xc_request_double_space(completed.get());
    if (spacing < 0) { error_ = "Missing spacing preference."; break; }
    presentations_[index].double_space = spacing != 0;
    int numbers = 0, available = 0;
    if (xc_request_note_numbers(completed.get(), &numbers, &available) != XC_OK) { error_ = "Missing note-number preference."; break; }
    presentations_[index].note_numbers = available ? numbers : -1;
    presentations_[index].chapter = xc_request_commentary_chapter_mode(completed.get()) == 1;
    presentations_[index].headings = xc_request_headings(completed.get()) == 1;
    if (xc_request_content_options(completed.get(), &presentations_[index].images, &presentations_[index].font_faces) != XC_OK) { error_ = "Missing presentation preferences."; break; }
    if (index == 0 && !passage_.commentary.empty()) {
      auto submitted = xc_worker_module_presentation_options(worker_, tab_, passage_.commentary.c_str(), &next);
      accept(submitted, next, Operation::commentary_options);
    }
    break;
  }
  case Operation::book_link:
  case Operation::link: {
    std::unique_ptr<xc_book_uri_target, decltype(&xc_book_uri_target_release)> book(xc_request_take_book_uri_target(completed.get()), xc_book_uri_target_release);
    if (book) {
      const auto *tree = xc_book_uri_target_tree(book.get());
      const auto module = copy(xc_book_tree_module(tree)), key = copy(xc_book_uri_target_key(book.get()));
      const auto *anchor = xc_book_uri_target_anchor(book.get());
      pending_book_anchor_ = anchor ? std::make_optional(std::string(anchor)) : std::nullopt;
      const xc_saved_book_selection target{module.c_str(), key.c_str(), nullptr};
      const auto submitted = xc_worker_restore_book_document_selection(worker_, tab_, &target, XC_BOOK_RESTORE_DISPLAY_KEY, &next);
      accept(submitted, next, Operation::book_uri_document); break;
    }
    std::unique_ptr<xc_link, decltype(&xc_link_release)> link(xc_request_take_link(completed.get()), xc_link_release);
    if (!link) { if (take_image(completed.get())) return; if (!take_preview(completed.get(), false)) error_ = "Unsupported study link result."; return; }
    const auto module = copy(xc_link_module(link.get())), reference = copy(xc_link_reference(link.get()));
    const auto *main = this->module(passage_.bible);
    if (main && main->type != "Biblical Texts") { visit(module, "", reference); break; }
    auto submitted = xc_worker_visit_linked_passage_with_anchor(worker_, tab_, passage_.bible.c_str(), passage_.commentary.c_str(), module.c_str(), reference.c_str(), xc_link_anchor(link.get()), &next);
    accept(submitted, next, Operation::passage); break;
  }
  }
}
}
