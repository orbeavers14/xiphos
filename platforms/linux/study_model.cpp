// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include <memory>
namespace xiphos::linux_preview {
namespace {
std::string copy(const char *p) { return p ? p : ""; }
bool has_nul(const std::string &s) { return s.find('\0') != std::string::npos; }
BookNode node(const xc_book_node &n) { return {n.offset, copy(n.path), copy(n.title), n.present != 0, n.has_children != 0}; }
}
bool ReaderModel::choose_book(const std::string &module) {
  if (!ready() || busy() || has_nul(module)) return false;
  xc_request *r = nullptr;
  auto s = xc_worker_select_book_document(worker_, tab_, module.c_str(), 4, &r);
  return accept(s, r, Operation::book_document);
}
bool ReaderModel::read_book_key(const std::string &key) {
  if (!ready() || busy() || book_.module.empty() || has_nul(key)) return false;
  // Shared restore owns numeric versus display-path interpretation.
  xc_saved_book_selection saved{book_.module.c_str(), key.c_str(), nullptr};
  xc_request *r = nullptr;
  auto s = xc_worker_restore_book_document_selection(worker_, tab_, &saved, XC_BOOK_RESTORE_DISPLAY_KEY, &r);
  return accept(s, r, Operation::book_document);
}
bool ReaderModel::read_book_node(uint64_t offset) {
  if (!ready() || busy() || book_.module.empty()) return false;
  xc_request *r = nullptr; auto s = xc_worker_select_book_document(worker_, tab_, book_.module.c_str(), offset, &r);
  return accept(s, r, Operation::book_document);
}
bool ReaderModel::navigate_book(int direction) {
  if (!ready() || busy() || book_.module.empty()) return false;
  xc_request *r = nullptr; auto s = xc_worker_navigate_book_document(worker_, tab_, direction, &r);
  return accept(s, r, Operation::book_document);
}
bool ReaderModel::browse_book(uint64_t parent, bool more) {
  if (!ready() || busy() || book_.module.empty() || (more && !book_contents_.after)) return false;
  append_book_children_ = more;
  pending_book_parent_ = more ? book_contents_.parent.offset : parent;
  xc_request *r = nullptr; auto s = xc_worker_book_tree(worker_, book_.module.c_str(), pending_book_parent_, &r);
  return accept(s, r, Operation::book_key);
}
bool ReaderModel::restore_book(const std::string &path) {
  if (!ready() || busy() || has_nul(path)) return false;
  xc_request *r = nullptr; auto s = xc_worker_restore_book_document_profile(worker_, tab_, path.c_str(), XC_BOOK_RESTORE_DISPLAY_KEY, &r);
  return accept(s, r, Operation::book_document);
}
bool ReaderModel::book_option(int option, bool enabled) {
  if (!ready() || busy() || book_.module.empty()) return false;
  xc_request *r = nullptr; xc_status s = XC_INVALID_ARGUMENT;
  switch (option) {
  case 0: s = xc_worker_set_image_content(worker_, tab_, book_.module.c_str(), enabled, &r); break;
  case 1: s = xc_worker_set_respect_font_faces(worker_, tab_, book_.module.c_str(), enabled, &r); break;
  case 2: s = xc_worker_set_double_space(worker_, tab_, book_.module.c_str(), enabled, &r); break;
  case 3: s = xc_worker_set_note_numbers(worker_, tab_, book_.module.c_str(), enabled, &r); break;
  }
  return accept(s, r, Operation::book_option);
}
bool ReaderModel::refresh_book() {
  if (!ready() || busy() || book_.module.empty()) return false;
  xc_request *r = nullptr; auto s = xc_worker_refresh_book_document(worker_, tab_, &r);
  return accept(s, r, Operation::book_document);
}
bool ReaderModel::close_book() {
  if (!ready() || busy()) return false;
  book_ = {}; book_contents_ = {}; ++revision_; return true;
}
bool ReaderModel::follow_book(const std::string &uri) {
  if (!ready() || busy() || book_.module.empty() || has_nul(uri)) return false;
  xc_request *r = nullptr; auto s = xc_worker_follow_book_link(worker_, tab_, uri.c_str(), &r);
  return accept(s, r, Operation::book_link);
}
bool ReaderModel::take_image(xc_request *r) {
  std::unique_ptr<xc_verse_image, decltype(&xc_verse_image_release)> verse(xc_request_take_verse_image(r), xc_verse_image_release);
  std::unique_ptr<xc_book_image_data, decltype(&xc_book_image_data_release)> book(xc_request_take_book_image_data(r), xc_book_image_data_release);
  Image value; const unsigned char *bytes = nullptr; size_t size = 0;
  if (verse) {
    value.module = copy(xc_verse_image_module(verse.get())); value.source = copy(xc_verse_image_source(verse.get()));
    bytes = xc_verse_image_bytes(verse.get()); size = xc_verse_image_size(verse.get());
  } else if (book) {
    const auto *source = xc_book_image_data_source(book.get());
    value.module = copy(xc_book_image_module(source)); value.source = copy(xc_book_image_filename(source));
    bytes = xc_book_image_data_bytes(book.get()); size = xc_book_image_data_size(book.get());
  } else return false;
  if (bytes && size) value.bytes.assign(bytes, bytes + size);
  value.revision = image_.revision + 1; image_ = std::move(value); return true;
}
bool ReaderModel::search(const SearchInput &input) {
  if (!ready() || busy() || input.modules.empty() || input.modules.size() > 32) return false;
  if (has_nul(input.expression) || has_nul(input.range_module) || has_nul(input.range)) return false;
  for (const auto &name : input.modules) if (has_nul(name)) return false;
  xc_search_options options; xc_search_options_default(&options);
  options.mode = input.mode; options.case_sensitive = input.case_sensitive;
  xc_search_query query; xc_search_query_default(&query);
  query.expression = input.expression.c_str(); query.scope = input.scope; query.options = &options;
  if (input.scope == XC_SEARCH_SCOPE_BOUNDS) { query.range_module = input.range_module.c_str(); query.range_expression = input.range.c_str(); }
  if (input.annotations) query.text_options = &*input.annotations;
  std::vector<const char *> names; for (const auto &m : input.modules) names.push_back(m.c_str());
  xc_request *r = nullptr; auto s = xc_worker_search_modules(worker_, tab_, names.data(), names.size(), &query, nullptr, &r);
  return accept(s, r, Operation::search_batch);
}
bool ReaderModel::cancel_search() { return request_ && xc_request_cancel_search(request_) == XC_OK; }
xc_search_progress ReaderModel::search_progress() const {
  xc_search_progress p{};
  if (request_) xc_request_search_progress(request_, &p);
  return p;
}
bool ReaderModel::reset_search_annotations() {
  if (!ready() || busy()) return false;
  xc_request *r = nullptr; auto s = xc_worker_clear_search_text_options(worker_, tab_, &r);
  return accept(s, r, Operation::search_reset);
}
bool ReaderModel::clear_search_scope() {
  if (!ready() || busy()) return false;
  xc_request *r = nullptr; auto s = xc_worker_clear_search_scope(worker_, tab_, &r);
  return accept(s, r, Operation::search_scope);
}
bool ReaderModel::preview_search_hit(size_t i) {
  if (!ready() || busy() || i >= search_results_.hits.size()) return false;
  const auto &hit = search_results_.hits[i];
  pending_module_ = hit.module;
  xc_request *r = nullptr; auto s = xc_worker_read_tab_entry(worker_, tab_, hit.module.c_str(), hit.reference.c_str(), &r);
  return accept(s, r, Operation::search_preview);
}
bool ReaderModel::activate_search_hit(size_t i) {
  if (!ready() || busy() || i >= search_results_.hits.size()) return false;
  const auto hit = search_results_.hits[i]; xc_request *r = nullptr;
  const auto *main = module(passage_.bible);
  if (!main || main->type != "Biblical Texts") return visit(hit.module, "", hit.reference);
  auto s = xc_worker_visit_linked_passage(worker_, tab_, passage_.bible.c_str(), passage_.commentary.c_str(), hit.module.c_str(), hit.reference.c_str(), &r);
  return accept(s, r, Operation::passage);
}
bool ReaderModel::export_search(bool html, bool all, const std::vector<size_t> &selection,
                                bool scripture, bool context, bool images, bool partial) {
  if (!ready() || busy() || !search_results_.generation.id) return false;
  std::vector<xc_search_visible_row> rows;
  for (auto i : selection) {
    if (i >= search_results_.hits.size()) return false;
    const auto &hit = search_results_.hits[i]; rows.push_back({hit.group, hit.row});
  }
  if (!all && rows.empty()) return false;
  xc_request *r = nullptr; xc_status s;
  if (html) {
    xc_search_html_export_request input; xc_search_html_export_request_default(&input);
    input.generation = search_results_.generation.id; input.selection = all ? 1 : 0;
    input.rows = all ? nullptr : rows.data(); input.row_count = all ? 0 : rows.size();
    input.include_scripture = scripture; input.allow_partial_source = partial;
    xc_search_html_image_options policy; xc_search_html_image_options_default(&policy);
    policy.policy = images ? XC_HTML_IMAGES_EMBEDDED_STATIC_PNG : XC_HTML_IMAGES_REJECT;
    s = xc_worker_export_search_rich_html_with_context(worker_, tab_, &input, &policy, 8 * 1024 * 1024, context, &r);
  } else if (all) {
    xc_search_all_text_export_request input; xc_search_all_text_export_request_default(&input);
    input.generation = search_results_.generation.id; input.include_scripture = scripture; input.allow_partial_source = partial;
    s = xc_worker_export_search_all_text_with_context(worker_, tab_, &input, context, &r);
  } else {
    xc_search_text_export_request input; xc_search_text_export_request_default(&input);
    input.generation = search_results_.generation.id; input.rows = rows.data(); input.row_count = rows.size(); input.include_scripture = scripture;
    s = xc_worker_export_search_visible_text_with_context(worker_, tab_, &input, context, &r);
  }
  return accept(s, r, Operation::search_export);
}
bool ReaderModel::import_search_lists(const std::string &path) {
  if (!ready() || busy() || has_nul(path)) return false;
  xc_request *r = nullptr; auto s = xc_worker_read_search_lists(worker_, tab_, path.c_str(), &r);
  return accept(s, r, Operation::search_lists);
}
bool ReaderModel::parse_search_modules(const std::string &text) {
  if (!ready() || busy() || has_nul(text)) return false;
  xc_request *r = nullptr; auto s = xc_worker_parse_search_module_list(worker_, tab_, text.c_str(), &r);
  return accept(s, r, Operation::search_names);
}
bool ReaderModel::take_extended(xc_request *r) {
  switch (operation_) {
  case Operation::book_document: {
    std::unique_ptr<xc_book_document, decltype(&xc_book_document_release)> d(xc_request_take_book_document(r), xc_book_document_release);
    if (!d) { if (xc_request_book_selection_present(r) != 0) error_ = "Missing book document."; return true; }
    BookDocument value; const auto *tree = xc_book_document_tree(d.get()); value.module = copy(xc_book_tree_module(tree));
    for (int i = 0; i < 5; ++i) { xc_book_node n{}; xc_book_tree_get_node(tree, i, &n); value.nodes[i] = node(n); }
    value.html = copy(xc_book_document_inline_html(d.get()));
    if (!xc_book_document_inline_html(d.get())) value.html = copy(xc_book_document_presentation_html(d.get()));
    xc_lexical_presentation layout{}; xc_book_document_lexical_layout(d.get(), &layout);
    value.css = copy(layout.css) + copy(xc_book_document_stylesheet_inline_css(d.get()));
    xc_book_document_display_options options{};
    if (xc_book_document_get_display_options(d.get(), &options) == XC_OK) { value.images = options.images; value.fonts = options.font_faces; value.spacing = options.double_space; }
    value.numbers = xc_book_document_note_numbers(d.get());
    if (xc_book_document_inline_issue_count(d.get())) value.issue = "Some book images could not be loaded.";
    if (xc_book_document_stylesheet_issue(d.get())) value.issue += " The module stylesheet could not be loaded.";
    const bool changed_module = book_.module != value.module; book_ = std::move(value);
    if (changed_module) { book_contents_ = {}; browse_book(); }
    return true;
  }
  case Operation::book_key: {
    std::unique_ptr<xc_book_tree, decltype(&xc_book_tree_release)> tree(xc_request_take_book_tree(r), xc_book_tree_release);
    if (!tree) { error_ = "Missing book browser source."; return true; }
    xc_book_node n{}; xc_book_tree_get_node(tree.get(), XC_BOOK_PARENT, &n); pending_book_up_ = node(n);
    xc_request *next = nullptr;
    auto s = xc_worker_book_children(worker_, book_.module.c_str(), pending_book_parent_, append_book_children_,
      append_book_children_ ? *book_contents_.after : 0, 64, &next);
    accept(s, next, Operation::book_children); return true;
  }
  case Operation::book_children: {
    std::unique_ptr<xc_book_children, decltype(&xc_book_children_release)> p(xc_request_take_book_children(r), xc_book_children_release);
    if (!p) { error_ = "Missing book contents."; return true; }
    BookContents value; value.module = copy(xc_book_children_module(p.get())); xc_book_node n{};
    xc_book_children_parent(p.get(), &n); value.parent = node(n); value.up = pending_book_up_;
    if (append_book_children_ && value.module == book_contents_.module && value.parent.offset == book_contents_.parent.offset) value.nodes = book_contents_.nodes;
    for (size_t i = 0; i < xc_book_children_count(p.get()); ++i) { xc_book_children_get_node(p.get(), i, &n); value.nodes.push_back(node(n)); }
    uint64_t after = 0; if (xc_book_children_next_after(p.get(), &after)) value.after = after;
    book_contents_ = std::move(value); return true;
  }
  case Operation::book_option: refresh_book(); return true;
  case Operation::book_link: {
    if (take_image(r)) return true;
    std::unique_ptr<xc_book_note, decltype(&xc_book_note_release)> n(xc_request_take_book_note(r), xc_book_note_release);
    if (!n) { error_ = "Unsupported book link result."; return true; }
    const auto *content = xc_book_note_content(n.get());
    note_ = {copy(xc_note_module(content)), copy(xc_note_reference(content)), copy(xc_note_html(content)), ""};
    references_ = {}; showing_references_ = false; return true;
  }
  case Operation::search_batch: {
    std::unique_ptr<xc_search_batch, decltype(&xc_search_batch_release)> b(xc_request_take_search_batch(r), xc_search_batch_release);
    if (!b) { error_ = "Missing search results."; return true; }
    SearchResults value; value.revision = search_results_.revision + 1;
    xc_search_batch_get_generation(b.get(), &value.generation); xc_search_batch_get_scope(b.get(), &value.scope);
    xc_search_batch_info info{}; xc_search_batch_get_info(b.get(), &info); value.cancelled = info.cancelled; value.limited = info.limited;
    for (size_t g = 0; g < info.group_count; ++g) {
      xc_search_batch_group_info group{}; xc_search_batch_get_group(b.get(), g, &group);
      SearchGroup copied; copied.module = copy(xc_search_batch_requested_module(b.get(), group.request_index)); copied.status = group.status;
      const auto *result = xc_search_batch_group_result(b.get(), g);
      if (result) {
        xc_search_get_info(result, &copied.info);
        for (size_t row = 0; row < copied.info.result_count; ++row) value.hits.push_back({copy(xc_search_source_module(result)), copy(xc_search_reference(result, row)), g, row});
      }
      value.groups.push_back(std::move(copied));
    }
    search_results_ = std::move(value); search_preview_ = {}; return true;
  }
  case Operation::search_preview: {
    std::unique_ptr<xc_entry, decltype(&xc_entry_release)> e(xc_request_take_entry(r), xc_entry_release);
    if (e) search_preview_ = {pending_module_, copy(xc_entry_reference(e.get())), copy(xc_entry_html(e.get())), ""};
    else error_ = "Missing search preview.";
    return true;
  }
  case Operation::search_export: {
    SearchExport value; value.revision = search_export_.revision + 1;
    std::unique_ptr<xc_search_html_export, decltype(&xc_search_html_export_release)> h(xc_request_take_search_html_export(r), xc_search_html_export_release);
    std::unique_ptr<xc_search_text_export, decltype(&xc_search_text_export_release)> t(xc_request_take_search_text_export(r), xc_search_text_export_release);
    if (h) {
      xc_search_html_export_info info{}; xc_search_html_export_get_info(h.get(), &info); value.disposition = info.disposition;
      if (info.disposition == XC_SEARCH_EXPORT_OK) { value.html = copy(xc_search_html_export_html(h.get())); value.text = copy(xc_search_html_export_plain_text(h.get())); }
    } else if (t) {
      xc_search_text_export_info info{}; xc_search_text_export_get_info(t.get(), &info); value.disposition = info.disposition;
      if (info.disposition == XC_SEARCH_EXPORT_OK) value.text = copy(xc_search_text_export_text(t.get()));
    }
    if (value.disposition != XC_SEARCH_EXPORT_OK) error_ = "Export refused (status " + std::to_string(value.disposition) + "). No partial document was produced.";
    search_export_ = std::move(value); return true;
  }
  case Operation::search_reset: notice_ = "Search annotation choices reset."; return true;
  case Operation::search_scope: xc_request_search_scope(r, &search_results_.scope); return true;
  case Operation::search_lists: {
    std::unique_ptr<xc_search_lists, decltype(&xc_search_lists_release)> l(xc_request_take_search_lists(r), xc_search_lists_release);
    if (!l) { error_ = "Missing saved search lists."; return true; }
    std::vector<SavedSearchList> values[2];
    for (int k = 0; k < 2; ++k) for (size_t i = 0; i < xc_search_lists_count(l.get(), k); ++i) {
      size_t effective = i; xc_search_lists_effective_index(l.get(), k, i, &effective);
      const auto *label = xc_search_lists_label(l.get(), k, i), *value = xc_search_lists_value(l.get(), k, i);
      values[k].push_back({label ? std::make_optional(std::string(label)) : std::nullopt, value ? std::make_optional(std::string(value)) : std::nullopt, effective});
    }
    for (int k = 0; k < 2; ++k) saved_lists_[k] = std::move(values[k]);
    return true;
  }
  case Operation::search_names: {
    std::unique_ptr<xc_search_module_names, decltype(&xc_search_module_names_release)> n(xc_request_take_search_module_names(r), xc_search_module_names_release);
    if (!n) { error_ = "Missing search module names."; return true; }
    parsed_search_modules_.clear(); for (size_t i = 0; i < xc_search_module_names_count(n.get()); ++i) parsed_search_modules_.push_back(copy(xc_search_module_names_at(n.get(), i)));
    ++parsed_search_revision_; return true;
  }
  default: return false;
  }
}
}
