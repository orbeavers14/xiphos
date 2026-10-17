// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/session.hpp"
#include <stdexcept>
#include <type_traits>
#include <functional>
#include "xiphos/core/heading_note_presentation.hpp"
#include "xiphos/core/note_markers.hpp"
#include "xiphos/core/preferences.hpp"
#include "xiphos/core/dictionary.hpp"
#include "xiphos/core/word_blocks.hpp"
#include "xiphos/core/lexical_style.hpp"
namespace xiphos::core {
namespace {
void clean_content(Entry &entry, ModulePresentationOptions &options) {
  auto clean = [&](std::string &html) {
    auto result = cleanup_html_content(std::move(html), options.image_content, options.respect_font_faces);
    html = std::move(result.html);
    options.image_content = result.image_content;
    options.respect_font_faces = result.respect_font_faces;
    for (auto &diagnostic : result.diagnostics) entry.cleanup_diagnostics.push_back(std::move(diagnostic));
  };
  clean(entry.heading_html); // Legacy collects/cleans headers before body content.
  clean(entry.html);
}
LexicalPresentation present_document(const std::vector<Entry *> &entries,
                                    ModulePresentationOptions &options,
                                    const std::optional<EngineOptions> &engine, bool enabled,
                                    const std::function<void(Entry &)> &after_cleanup = {}) {
  LexicalPresentation presentation;
  if (enabled) presentation.css = lexical_block_style(false, false, false, options.double_space);
  presentation.requested = enabled && engine && (engine->strongs || engine->lemmas || engine->morphs);
  if (presentation.requested) {
    std::vector<std::pair<LexicalLayout, LexicalLayout>> rendered;
    bool complete = !entries.empty();
    for (const auto *entry : entries) {
      auto heading = layout_lexical_content(entry->heading_html, engine->strongs, engine->lemmas, engine->morphs, options.double_space);
      auto body = layout_lexical_content(entry->html, engine->strongs, engine->lemmas, engine->morphs, options.double_space);
      complete = complete && heading.applied && body.applied;
      rendered.emplace_back(std::move(heading), std::move(body));
    }
    // One pane is one CSS document. A raw fallback verse must never inherit
    // positioning rules intended for formatted neighbors or headings.
    presentation.applied = complete;
    if (complete) {
      presentation.css = lexical_block_style(engine->strongs, engine->lemmas, engine->morphs, options.double_space);
      for (std::size_t i = 0; i < entries.size(); ++i) {
        entries[i]->heading_html = std::move(rendered[i].first.html);
        entries[i]->html = std::move(rendered[i].second.html);
      }
    }
  }
  for (auto *entry : entries) {
    entry->lexical = presentation;
    clean_content(*entry, options); // Legacy order: format, then heading/body cleanup.
    if (after_cleanup) after_cleanup(*entry);
  }
  return presentation;
}
// Flatten SDK heading fragments and bodies into one document for the existing
// atomic lexical stage. Each piece is then cleaned/numbered in legacy order;
// the raw copied Reader sources remain unchanged in the published snapshot.
LexicalPresentation present_numbered_document(const std::vector<Entry *> &entries,
    ModulePresentationOptions &options, const std::optional<EngineOptions> &engine, bool lexical,
    const std::vector<std::vector<std::string>> *heading_html = nullptr) {
  struct Span { Entry *entry; std::size_t first, body; };
  std::vector<Entry> pieces;
  std::vector<Span> spans;
  for (std::size_t entry_index = 0; entry_index < entries.size(); ++entry_index) {
    auto *entry = entries[entry_index];
    if (heading_html && (heading_html->size() != entries.size() || (*heading_html)[entry_index].size() != entry->note_source.headings.size()))
      throw std::logic_error("Heading replacements do not match source fragments");
    if (!entry->note_source.captured) throw std::logic_error("Note numbering requires captured Reader sources");
    const auto first = pieces.size(); std::size_t offset = 0;
    std::size_t fragment_index = 0;
    for (const auto &fragment : entry->note_source.headings) {
      if (offset > entry->heading_html.size() || fragment.html.size() > entry->heading_html.size() - offset ||
          entry->heading_html.compare(offset, fragment.html.size(), fragment.html) != 0)
        throw std::logic_error("Note heading sources do not match the Reader snapshot");
      offset += fragment.html.size();
      pieces.push_back({{}, heading_html ? (*heading_html)[entry_index][fragment_index] : fragment.html});
      ++fragment_index;
      pieces.back().note_source.captured = true;
      pieces.back().note_source.reported_label = fragment.reported_note_label;
    }
    if (offset != entry->heading_html.size()) throw std::logic_error("Missing note heading sources");
    const auto body = pieces.size();
    pieces.push_back({entry->reference, entry->html});
    pieces.back().note_source.captured = true;
    pieces.back().note_source.reported_label = entry->note_source.reported_label;
    spans.push_back({entry, first, body});
  }
  std::vector<Entry *> document;
  for (auto &piece : pieces) document.push_back(&piece);
  NoteNumberingState counters;
  const auto presentation = present_document(document, options, engine, lexical, [&](Entry &piece) {
    auto numbered = number_note_markers(std::move(piece.html), options.note_numbers,
                                        !piece.note_source.reported_label.empty(), counters);
    piece.html = std::move(numbered.html); counters = numbered.counters;
  });
  for (const auto &span : spans) {
    span.entry->heading_html.clear();
    for (auto index = span.first; index <= span.body; ++index) {
      auto &piece = pieces[index];
      if (index == span.body) span.entry->html = std::move(piece.html);
      else span.entry->heading_html += piece.html;
      for (auto &issue : piece.cleanup_diagnostics) span.entry->cleanup_diagnostics.push_back(std::move(issue));
    }
    span.entry->lexical = presentation;
  }
  return presentation;
}
}
std::vector<std::vector<std::string>> StudySession::heading_note_html(const std::string &module,
    const std::vector<Entry *> &entries, const ModulePresentationOptions &options,
    const std::optional<EngineOptions> &engine) {
  if (!heading_note_links_enabled_ || !options.note_numbering_available || !engine) return {};
  std::vector<std::vector<std::string>> result;
  for (const auto *entry : entries) {
    result.emplace_back();
    for (std::size_t index = 0; index < entry->note_source.headings.size(); ++index) {
      const auto &raw = entry->note_source.headings[index];
      auto html = raw.html;
      if (html.find("action=showNote") != std::string::npos) {
        try {
          auto capture = *engine;
          // Retain the legacy visible marker even if Headings extracted it before
          // Footnotes Off ran on the body. Candidates must match its exact HTML.
          capture.footnotes = true;
          const auto source = reader_.heading_notes(module, entry->reference, index, capture, options.note_numbers, false);
          if (compatible_heading_note_presentation(raw, source, index)) html = source.html;
        } catch (const std::invalid_argument &) { /* Unsupported candidate retains original heading. */ }
          catch (const std::out_of_range &) { /* Missing fragment retains original heading. */ }
      }
      result.back().push_back(std::move(html));
    }
  }
  return result;
}
Entry StudySession::present_content(const std::string &module, Entry entry) {
  auto options = module_presentation_options(module);
  const auto engine = module_engine_options(module);
  const auto headings = heading_note_html(module, {&entry}, options, engine);
  if (options.note_numbering_available) present_numbered_document({&entry}, options, engine, lexical_layout_enabled_, headings.empty() ? nullptr : &headings);
  else present_document({&entry}, options, engine, lexical_layout_enabled_);
  presentation_options_[module] = options;
  return entry;
}
Chapter StudySession::present_content(const std::string &module, Chapter chapter, const DisplayOptions &display, bool commentary) {
  auto options = module_presentation_options(module);
  std::vector<Entry *> entries;
  for (auto &entry : chapter.verses) entries.push_back(&entry);
  const auto engine = module_engine_options(module);
  const auto headings = heading_note_html(module, entries, options, engine);
  chapter.lexical = options.note_numbering_available ? present_numbered_document(entries, options, engine, lexical_layout_enabled_, headings.empty() ? nullptr : &headings)
      : present_document(entries, options, engine, lexical_layout_enabled_);
  chapter.html = commentary ? display_commentary_chapter(chapter, chapter.verse_ordinals, display) : display_chapter(chapter, display);
  presentation_options_[module] = options;
  return chapter;
}

void StudySession::set_lexicon_preferences(const LexiconPreferences &preferences) {
  (void)borrow_lexicon_preferences(preferences);
  auto replacement = std::make_optional(preferences);
  lexicons_.swap(replacement);
}
LexicalEntry StudySession::visit_lexical(const std::string &module, const std::string &reference,
                                      const std::string &uri, int origin) {
  return lookup_lexical(module, reference, uri, origin, true);
}
LexicalEntry StudySession::read_lexical(const std::string &module, const std::string &reference,
                                     const std::string &uri, int origin) {
  return lookup_lexical(module, reference, uri, origin, false);
}
LexicalEntry StudySession::lookup_lexical(const std::string &module, const std::string &reference,
                                        const std::string &uri, int origin, bool click) {
  if (!lexicons_) throw std::invalid_argument("Configure lexicons before lexical lookup");
  auto command = lexical_link(uri, module);
  // Legacy morphology uses the caller's committed source context; a query
  // passage is retained as metadata rather than replacing that context.
  const auto &context = reference;
  const auto selected = reader_.select_lexicon(command, context, origin, *lexicons_);
  // Main-pane clicks uppercase and record the requested key; information
  // dialogs and hover previews keep the raw key and have no dictionary visit.
  const bool main_click = click && origin == XIPHOS_LEXICON_MAIN;
  const auto key = main_click ? dictionary_click_key(command.value) : command.value;
  auto entry = read_dictionary(selected, key);
  if (main_click) dictionary_history_.visit({selected, key});
  return {std::move(command), context, std::move(entry)};
}
DictionaryEntry StudySession::visit_dictionary(const std::string &module, const std::string &key) {
  auto entry = read_dictionary(module, key);
  dictionary_history_.visit({module, key});
  return entry;
}
DictionaryEntry StudySession::navigate_dictionary_history(int direction) {
  const auto target = dictionary_history_.destination(direction);
  auto entry = read_dictionary(target.module, target.key);
  dictionary_history_.move(direction);
  return entry;
}
DictionaryEntry StudySession::navigate_dictionary(const std::string &module, const std::string &key, int direction) {
  const auto options = module_engine_options(module);
  return options ? reader_.navigate_dictionary(module, key, direction, *options)
                 : reader_.navigate_dictionary(module, key, direction);
}
DictionaryEntry StudySession::read_dictionary(const std::string &module, const std::string &key) {
  const auto options = module_engine_options(module);
  return options ? reader_.read_dictionary(module, key, *options) : reader_.read_dictionary(module, key);
}
BookEntry StudySession::read_book(const std::string &module, std::uint64_t offset) {
  const auto options = module_engine_options(module);
  return options ? reader_.read_book(module, offset, *options) : reader_.read_book(module, offset);
}
BookDocument StudySession::read_book_document(const std::string &module, std::uint64_t offset) {
  const auto options = module_engine_options(module);
  return options ? reader_.read_book_document(module, offset, *options) : reader_.read_book_document(module, offset);
}
BookEntry StudySession::select_book(const BookSelectionTarget &target) {
  const auto resolved = reader_.resolve_book_target(book_reading_target(target));
  auto result = read_book(resolved.module, resolved.node.offset);
  if (result.tree.module != resolved.module || result.tree.node.offset != resolved.node.offset ||
      result.tree.node.path != resolved.node.path)
    throw std::runtime_error("Book source changed before selection could be published");
  // Prepare every owned copy first. Failure to resolve/render/copy leaves the
  // previously published tab selection intact; SDK cursor position is ephemeral.
  std::optional<BookSelectionState> next{BookSelectionState{target, result.tree, std::nullopt}};
  static_assert(std::is_nothrow_move_assignable_v<decltype(book_selection_)>,
                "Book selection publication must not throw after staging owned copies");
  book_selection_ = std::move(next);
  return result;
}
BookDocument StudySession::select_book_document(const BookSelectionTarget &target) {
  const auto resolved = reader_.resolve_book_target(book_reading_target(target));
  auto result = read_book_document(resolved.module, resolved.node.offset);
  if (result.tree.module != resolved.module || result.tree.node.offset != resolved.node.offset ||
      result.tree.node.path != resolved.node.path)
    throw std::runtime_error("Book source changed before document publication");
  auto options = module_presentation_options(resolved.module);
  Entry visible;
  visible.html = result.html;
  // Legacy general-book display formats the combined document before cleanup.
  // Reuse the same atomic lexical/cleanup stage as other native reading panes.
  present_document({&visible}, options, module_engine_options(resolved.module), lexical_layout_enabled_);
  result.presentation = BookDocumentPresentation{std::move(visible.html), std::move(visible.lexical),
      std::move(visible.cleanup_diagnostics), options.image_content, options.respect_font_faces, options.double_space};
  std::optional<BookSelectionState> next{BookSelectionState{target, result.tree, result}};
  static_assert(std::is_nothrow_move_assignable_v<decltype(book_selection_)>,
                "Book document publication must not throw after staging owned copies");
  static_assert(std::is_nothrow_copy_assignable_v<ModulePresentationOptions>,
                "Presentation discovery assignment must not fail after map insertion");
  // Map insertion failure leaves prior discovery/selection intact. Once staged
  // state and this scalar option value exist, publication cannot throw.
  presentation_options_.insert_or_assign(resolved.module, options);
  book_selection_ = std::move(next);
  return result;
}
std::optional<BookDocument> StudySession::refresh_book_document() {
  if (!book_selection_ || !book_selection_->document) return std::nullopt;
  const auto target = book_selection_->requested;
  return select_book_document(target);
}
std::optional<BookDocument> StudySession::navigate_book_document(BookNavigation direction) {
  const auto target = book_navigation_target(direction);
  return target ? std::make_optional(select_book_document(*target)) : std::nullopt;
}
std::optional<BookDocument> StudySession::restore_book_document_selection(const SavedBookSelection &saved, BookRestoreMode mode) {
  const auto target = saved_book_target(saved, mode);
  return target ? std::make_optional(select_book_document(*target)) : std::nullopt;
}
BookNote StudySession::book_document_note_link(const std::string &uri, bool render_body) {
  if (!book_selection_ || !book_selection_->document)
    throw std::invalid_argument("Select a book document before opening its notes");
  const auto target = xiphos::core::book_document_note_link(uri, *book_selection_->document);
  return book_note_link_at(target.command.module, target.offset, uri, render_body);
}
std::optional<BookSelectionTarget> StudySession::book_navigation_target(BookNavigation direction) {
  switch (direction) {
    case BookNavigation::parent: case BookNavigation::first_child:
    case BookNavigation::previous_sibling: case BookNavigation::next_sibling: break;
    default: throw std::invalid_argument("Unknown book navigation direction");
  }
  if (!book_selection_) return std::nullopt;
  const auto &source = book_selection_->tree;
  const auto current = reader_.book_tree(source.module, source.node.offset);
  if (current.node.path != source.node.path)
    throw std::runtime_error("Committed book source changed before navigation");
  std::optional<BookTreeNode> target;
  switch (direction) {
    case BookNavigation::parent: target = current.parent; break;
    case BookNavigation::first_child: target = current.first_child; break;
    case BookNavigation::previous_sibling: target = current.previous_sibling; break;
    case BookNavigation::next_sibling: target = current.next_sibling; break;
    default: throw std::invalid_argument("Unknown book navigation direction");
  }
  if (!target) return std::nullopt;
  return BookSelectionTarget{current.module, target->offset};
}
std::optional<BookEntry> StudySession::navigate_book(BookNavigation direction) {
  const auto target = book_navigation_target(direction);
  return target ? std::make_optional(select_book(*target)) : std::nullopt;
}
std::optional<BookEntry> StudySession::restore_book_selection(const SavedBookSelection &saved, BookRestoreMode mode) {
  const auto target = saved_book_target(saved, mode);
  if (!target) return std::nullopt;
  return select_book(*target);
}
BookNote StudySession::book_note(const std::string &module, std::uint64_t offset, const std::string &identifier, bool render_body) {
  const auto options = module_engine_options(module);
  return options ? reader_.book_note(module, offset, identifier, *options, render_body)
                 : reader_.book_note(module, offset, identifier, render_body);
}
BookNote StudySession::book_note_link_at(const std::string &module, std::uint64_t offset, const std::string &uri, bool render_body) {
  const auto source = reader_.book_tree(module, offset);
  const auto target = xiphos::core::book_note_link(uri, module, source.node.path);
  auto result = book_note(module, offset, target.identifier, render_body);
  if (result.tree.node.path != target.reference || result.note.module != target.module ||
      ((target.type == "x") != (result.note.type == "crossReference")))
    throw std::invalid_argument("Book-note command type/source does not match the entry");
  return result;
}
ReferenceList StudySession::read_reference_list(const std::string &module, const std::string &expression,
                                              const std::string &context) {
  const auto options = module_engine_options(module);
  return options ? reader_.read_reference_list(module, expression, context, *options)
                 : reader_.read_reference_list(module, expression, context);
}
ParsedReferenceList StudySession::reference_targets(const std::string &module, const std::string &expression,
                                                    const std::string &context) {
  // Parsing depends on module versification/context, not rendering options.
  return reader_.reference_targets(module, expression, context);
}
StudyNote StudySession::study_note(const std::string &module, const std::string &reference,
                                  const std::string &identifier) {
  const auto options = module_engine_options(module);
  return options ? reader_.study_note(module, reference, identifier, *options)
                 : reader_.study_note(module, reference, identifier);
}
StudyNote StudySession::study_note_metadata(const std::string &module, const std::string &reference,
                                            const std::string &identifier) {
  const auto options = module_engine_options(module);
  return options ? reader_.study_note_metadata(module, reference, identifier, *options)
                 : reader_.study_note_metadata(module, reference, identifier);
}
Entry StudySession::read_commentary_entry(const std::string &module, const std::string &reference) {
  const auto options = module_engine_options(module);
  const auto presentation = module_presentation_options(module);
  auto entry = presentation.note_numbering_available
      ? reader_.read_commentary_entry(module, reference, presentation.headings, *options, presentation.note_numbers)
      : options ? reader_.read_commentary_entry(module, reference, presentation.headings, *options)
                : reader_.read_commentary_entry(module, reference, presentation.headings);
  return present_content(module, std::move(entry));
}
Entry StudySession::read_entry(const std::string &module, const std::string &reference) {
  const auto options = module_engine_options(module);
  const auto presentation = module_presentation_options(module);
  auto entry = presentation.note_numbering_available ? reader_.read(module, reference, *options, presentation.note_numbers)
      : options ? reader_.read(module, reference, *options) : reader_.read(module, reference);
  return present_content(module, std::move(entry));
}
ModulePresentationOptions StudySession::module_presentation_options(const std::string &id) const {
  require_module(id);
  const auto found = presentation_options_.find(id);
  auto result = [&]() -> ModulePresentationOptions {
    if (found != presentation_options_.end()) return found->second;
    for (const auto &module : reader_.modules())
      if (module.id == id) return presentation_options_from_preferences({}, module.regular_commentary);
    throw std::out_of_range("Module is not installed");
  }();
  result.note_numbering_available = note_numbering_enabled_ && module_engine_options(id).has_value();
  return result;
}
void StudySession::set_note_numbers(const std::string &id, bool enabled) {
  auto options = module_presentation_options(id);
  if (!options.note_numbering_available) throw std::invalid_argument("Enable note numbering and configure module engine options first");
  options.note_numbers = enabled;
  presentation_options_[id] = options;
}
void StudySession::set_double_space(const std::string &id, bool enabled) {
  auto options = module_presentation_options(id);
  options.double_space = enabled;
  presentation_options_[id] = options;
}
void StudySession::set_image_content(const std::string &id, bool enabled) {
  auto options = module_presentation_options(id);
  options.image_content = enabled ? ContentPreference::on : ContentPreference::off;
  presentation_options_[id] = options;
}
void StudySession::set_respect_font_faces(const std::string &id, bool enabled) {
  auto options = module_presentation_options(id);
  options.respect_font_faces = enabled ? ContentPreference::on : ContentPreference::off;
  presentation_options_[id] = options;
}
void StudySession::set_headings(const std::string &id, bool enabled) {
  auto options = module_presentation_options(id);
  options.headings = enabled;
  presentation_options_[id] = options;
}
void StudySession::set_commentary_chapter_mode(const std::string &id, bool enabled) {
  require_module(id);
  for (const auto &module : reader_.modules()) {
    if (module.id != id) continue;
    if (!module.regular_commentary) throw std::invalid_argument("Chapter mode requires a regular commentary");
    auto options = module_presentation_options(id);
    options.commentary_by_chapter = enabled;
    presentation_options_[id] = options;
    return;
  }
}
Chapter StudySession::read_commentary_chapter(const std::string &module, const std::string &reference,
                                              const DisplayOptions &display) {
  const auto options = module_engine_options(module);
  const auto presentation = module_presentation_options(module);
  auto chapter = presentation.note_numbering_available
      ? reader_.read_commentary_chapter(module, reference, display, *options, presentation.headings, presentation.note_numbers)
      : options ? reader_.read_commentary_chapter(module, reference, display, *options, presentation.headings)
                : reader_.read_commentary_chapter(module, reference, display, presentation.headings);
  return present_content(module, std::move(chapter), display, true);
}
Chapter StudySession::read_chapter(const std::string &module, const std::string &reference,
                                  const DisplayOptions &display) {
  const auto options = module_engine_options(module);
  const auto presentation = module_presentation_options(module);
  auto chapter = presentation.note_numbering_available ? reader_.read_chapter(module, reference, display, *options, presentation.note_numbers)
      : options ? reader_.read_chapter(module, reference, display, *options) : reader_.read_chapter(module, reference, display);
  return present_content(module, std::move(chapter), display, false);
}
void StudySession::require_module(const std::string &module) const {
  for (const auto &installed : reader_.modules()) if (installed.id == module) return;
  throw std::out_of_range("Module is not installed");
}
void StudySession::save_module_preferences(const std::string &path) const {
  ModuleEnginePreferences settings;
  ModulePresentationPreferences presentation;
  for (const auto &module : reader_.modules()) {
    const auto options = module_engine_options(module.id);
    if (!options) throw std::invalid_argument("Enable module preferences before saving");
    settings.emplace(module.id, *options);
    presentation.emplace(module.id, module_presentation_options(module.id));
  }
  xiphos::core::save_module_preferences(path, settings, presentation);
}
void StudySession::import_module_preferences(const std::string &path) {
  const auto loaded = read_module_option_values(path);
  ModuleEnginePreferences replacement;
  ModulePresentationPreferences presentation;
  for (const auto &module : reader_.modules()) {
    const auto found = loaded.find(module.id);
    const auto values = found == loaded.end() ? EngineOptionPreferences{} : found->second;
    replacement.emplace(module.id, engine_options_from_preferences(values));
    presentation.emplace(module.id, presentation_options_from_preferences(values, module.regular_commentary));
  }
  // Commit only after all file reads, decoding and allocations succeed.
  module_options_.swap(replacement);
  presentation_options_.swap(presentation);
  options_.reset();
  module_preferences_ = true;
}
void StudySession::select_module_variant(const std::string &module, int choice) {
  const auto options = module_engine_options(module);
  if (!options) throw std::invalid_argument("Enable module preferences before selecting a variant");
  bool declared = false;
  for (const auto &installed : reader_.modules())
    if (installed.id == module) declared = installed.options.declared.variants_primary;
  if (!declared) throw std::invalid_argument("Module does not declare a textual variant filter");
  set_module_engine_options(module, select_variant_reading(*options, choice));
}
void StudySession::set_module_engine_options(const std::string &module, const EngineOptions &options) {
  require_module(module);
  module_options_[module] = options;
}
void StudySession::clear_module_engine_options(const std::string &module) {
  require_module(module);
  module_options_.erase(module);
}
std::optional<EngineOptions> StudySession::module_engine_options(const std::string &module) const {
  require_module(module);
  const auto found = module_options_.find(module);
  if (found != module_options_.end()) return found->second;
  if (options_) return options_;
  if (module_preferences_) return engine_options_from_preferences({});
  return std::nullopt;
}
StudyPassage StudySession::read_passage(const std::string &module, const std::string &commentary,
                                        const std::string &reference) {
  if (reference.empty() || reference.find('\0') != std::string::npos)
    throw std::invalid_argument("Study passage requires a nonempty reference without NUL");
  auto require_type = [&](const std::string &id, const char *type) {
    require_module(id);
    for (const auto &installed : reader_.modules())
      if (installed.id == id && installed.type != type)
        throw std::invalid_argument("Module has the wrong study-pane type");
  };
  require_type(module, "Biblical Texts");
  if (!commentary.empty()) require_type(commentary, "Commentaries");
  StudyPassage result{module, commentary, read_entry(module, reference), std::nullopt};
  if (!commentary.empty()) result.commentary = read_commentary_entry(commentary, result.text.reference);
  return result;
}
StudyPassage StudySession::visit_passage(const std::string &module, const std::string &commentary,
                                         const std::string &reference) {
  auto result = read_passage(module, commentary, reference);
  // Render both panes before committing history. Preserve the characterized
  // reference-only deduplication, including its first recorded module pair.
  history_.visit({result.text.reference, module, commentary});
  return result;
}
StudyPassage StudySession::visit_linked_passage(const std::string &bible, const std::string &commentary,
                                                const PassageLink &target) {
  require_module(target.module);
  for (const auto &module : reader_.modules()) {
    if (module.id != target.module) continue;
    if (module.type == "Biblical Texts") return visit_passage(target.module, commentary, target.reference);
    if (module.type == "Commentaries") return visit_passage(bible, target.module, target.reference);
  }
  throw std::invalid_argument("Link target is not a Bible or commentary");
}
StudyPassage StudySession::navigate_passage(int direction) {
  const auto &location = history_.destination(direction);
  // Old single-entry clients can have standalone commentary locations. They
  // have no paired commentary; retain that history compatibility in the core.
  const bool bible = [&] {
    for (const auto &module : reader_.modules())
      if (module.id == location.module) return module.type == "Biblical Texts";
    return false;
  }();
  auto result = !bible && location.commentary.empty()
      ? StudyPassage{location.module, "", read_entry(location.module, location.reference), std::nullopt}
      : read_passage(location.module, location.commentary, location.reference);
  history_.move(direction);
  return result;
}
StudyPassage StudySession::navigate_passage_chapter(const std::string &module, const std::string &commentary,
                                                    const std::string &reference, int direction) {
  return visit_passage(module, commentary, reader_.adjacent_chapter(module, reference, direction));
}
Entry StudySession::visit(const std::string &module, const std::string &reference) {
  auto entry = read_entry(module, reference);
  history_.visit({entry.reference, module, ""});
  return entry;
}
Entry StudySession::navigate_chapter(const std::string &module, const std::string &reference,
                                     int direction) {
  return visit(module, reader_.adjacent_chapter(module, reference, direction));
}
Entry StudySession::navigate(int direction) {
  const auto &location = history_.destination(direction);
  auto entry = read_entry(location.module, location.reference);
  history_.move(direction);
  return entry;
}
}
