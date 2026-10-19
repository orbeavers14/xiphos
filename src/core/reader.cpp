#include "xiphos/core/dictionary.hpp"
#include "xiphos/core/commentary.hpp"
#include "xiphos/core/headings.hpp"
#include "xiphos/core/presentation.hpp"
#include "xiphos/core/entry_attribute.hpp"
#include "xiphos/core/note_rendering.hpp"
// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/reader.hpp"
#include "book_tree_source.hpp"
#include "book_resource_source.hpp"
#include "search_source.hpp"
#include "search_control.hpp"
#include "xiphos/core/reference_list.hpp"
#include <markupfiltmgr.h>
#include "xiphos/core/engine_manager.hpp"
#include <swmgr.h>
#include <swmodule.h>
#include <swoptfilter.h>
#include <swld.h>
#include <versekey.h>
#include <filesystem>
#include <mutex>
#include <stdexcept>

namespace xiphos::core {
namespace {
void copy_note_source(sword::SWModule &module, Entry &entry) {
  entry.note_source.captured = true;
  entry.note_source.reported_label = entry_attribute(module, "Footnote", "1", "n", false);
}
void copy_headings(sword::SWModule &module, Entry &entry, bool metadata) {
  if (metadata) {
    entry.note_source.headings = render_preverse_heading_fragments(module);
    for (const auto &fragment : entry.note_source.headings) entry.heading_html += fragment.html;
  } else {
    for (const auto &heading : render_preverse_headings(module)) entry.heading_html += heading;
  }
}
// Heading extraction must run even when no per-module engine profile is active.
// Preserve every registered Headings filter, rather than a global getter that
// may describe only one of several filters with the same option name.
class ScopedCommentaryHeadings {
  std::vector<std::pair<sword::SWOptionFilter *, std::string>> saved_;
public:
  explicit ScopedCommentaryHeadings(EngineManager &manager) {
    for (auto *filter : manager.option_filters())
      if (std::string(filter->getOptionName()) == "Headings") {
        const auto *value = filter->getOptionValue();
        if (!value) throw std::runtime_error("Heading filter has no restorable value");
        saved_.emplace_back(filter, value);
      }
    try { manager.setGlobalOption("Headings", "On"); }
    catch (...) { restore(); throw; }
  }
  ~ScopedCommentaryHeadings() { restore(); }
  void restore() { for (const auto &setting : saved_) setting.first->setOptionValue(setting.second.c_str()); }
  ScopedCommentaryHeadings(const ScopedCommentaryHeadings &) = delete;
  ScopedCommentaryHeadings &operator=(const ScopedCommentaryHeadings &) = delete;
};
}

struct Reader::Impl {
  std::unique_ptr<EngineManager> manager;
  NoteRenderingState note_rendering;
  std::vector<Module> modules;
  std::mutex mutex;
  sword::SWModule &verse_module(const std::string &id, const std::string &reference) {
    if (reference.empty()) throw std::invalid_argument("Reference is empty");
    auto *module = manager->getModule(id.c_str());
    if (!module) throw std::out_of_range("Module is not installed");
    if (!dynamic_cast<sword::VerseKey *>(module->getKey()))
      throw std::invalid_argument("This slice only supports verse-key modules");
    return *module;
  }
};
Reader::Reader(const std::string &root) : impl_(std::make_unique<Impl>()) {
  if (!root.empty() && !std::filesystem::is_directory(root))
    throw std::invalid_argument("Module root must be an existing directory");
  // SWMgr takes ownership of the filter manager, matching the legacy backend.
  impl_->manager = std::make_unique<EngineManager>(
    root.empty() ? nullptr : root.c_str(), true,
    new sword::MarkupFilterMgr(sword::FMT_XHTML), false, root.empty());
  bool transliteration_available = false;
  for (const auto &name : impl_->manager->getGlobalOptions())
    if (name == "Transliteration") transliteration_available = true;
  for (const auto &item : impl_->manager->Modules) {
    auto &m = *item.second;
    // This private manager owns all renderer writes. Establish the default Off
    // baseline verified by core_sdk_note_numbers; later scopes restore tracked
    // per-filter state, including aliases shared across modules.
    impl_->note_rendering.register_module(m, false);
    const char *language = m.getConfigEntry("Lang");
    const char *feature = m.getConfigEntry("Feature");
    const bool dictionary = std::string(m.getType()) == "Lexicons / Dictionaries" && dynamic_cast<sword::SWLD *>(&m) &&
      (!feature || std::string(feature) != "DailyDevotion");
    impl_->modules.push_back({m.getName(), m.getDescription(), m.getType(),
                             language ? language : "", module_options(m), transliteration_available, dictionary,
                             dictionary ? xiphos_core_lexicon_role(feature) : 0,
                             regular_commentary(m.getType(), m.getConfigEntry("ModDrv")), detail::indexed_book(m), detail::supports_verse_search(m)});
  }
}
Reader::~Reader() = default;
const std::vector<Module> &Reader::modules() const { return impl_->modules; }
BookTree Reader::book_tree(const std::string &id, std::uint64_t offset) {
  if (id.empty() || id.find('\0') != std::string::npos) throw std::invalid_argument("Invalid general-book module ID");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  return detail::capture_book_tree(*module, offset);
}
BookTree Reader::book_tree_path(const std::string &id, const std::string &path) {
  if (id.empty() || id.find('\0') != std::string::npos) throw std::invalid_argument("Invalid general-book module ID");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  return detail::capture_book_tree_path(*module, path);
}
BookTree Reader::resolve_book_target(const BookSelectionTarget &target) {
  if (target.module.empty() || target.module.find('\0') != std::string::npos)
    throw std::invalid_argument("Invalid general-book module ID");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = impl_->manager->getModule(target.module.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  if (const auto *offset = std::get_if<std::uint64_t>(&target.address))
    return detail::capture_book_tree(*module, *offset);
  return detail::capture_book_display_path(*module, std::get<std::string>(target.address));
}
BookEntry Reader::read_book(const std::string &id, std::uint64_t offset, const EngineOptions &options) {
  return book_impl(id, offset, &options);
}
BookChildren Reader::book_children(const std::string &id, std::uint64_t parent,
                                   std::optional<std::uint64_t> after, std::uint32_t limit) {
  if (id.empty() || id.find('\0') != std::string::npos) throw std::invalid_argument("Invalid general-book module ID");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  return detail::capture_book_children(*module, parent, after, limit);
}
BookEntry Reader::read_book(const std::string &id, std::uint64_t offset) {
  return book_impl(id, offset, nullptr);
}
SearchResult Reader::search_module(SearchQuery query,const SearchLimits &limits,std::shared_ptr<SearchControl> control) {
  return search_impl(std::move(query),limits,std::move(control),nullptr);
}
SearchResult Reader::search_impl(SearchQuery query,const SearchLimits &limits,std::shared_ptr<SearchControl> control,
                                 std::shared_ptr<const detail::SearchScope> *scope,const std::optional<SearchTextOptions> &previous_text) {
  detail::SearchControlScope progress(control);
  if(query.module.empty()||query.module.find('\0')!=std::string::npos)throw std::invalid_argument("Invalid search module");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module=impl_->manager->getModule(query.module.c_str());
  if(!module)throw std::out_of_range("Search module is not installed");
  std::shared_ptr<const detail::SearchScope> staged;
  auto result=detail::capture_module_search(*impl_->manager,*module,std::move(query),limits,control,
    scope?scope->get():nullptr,scope?&staged:nullptr,{},previous_text);
  if(staged){*scope=std::move(staged);result.scope_updated=true;}
  if(scope&&*scope)result.scope=(*scope)->info();
  progress.finish(result.cancelled);return result;
}
BookDocument Reader::read_book_document(const std::string &id, std::uint64_t offset,
                                        const EngineOptions &options, const BookDocumentLimits &limits) {
  return book_document_impl(id, offset, &options, limits);
}
BookDocument Reader::read_book_document(const std::string &id, std::uint64_t offset, const BookDocumentLimits &limits) {
  return book_document_impl(id, offset, nullptr, limits);
}
BookDocument Reader::read_book_document(const std::string &id, std::uint64_t offset,
    const EngineOptions &options, bool labels, const BookDocumentLimits &limits) {
  return book_document_impl(id, offset, &options, limits, &labels);
}
BookDocument Reader::book_document_impl(const std::string &id, std::uint64_t offset,
                                       const EngineOptions *options, const BookDocumentLimits &limits, const bool *labels) {
  if (id.empty() || id.find('\0') != std::string::npos) throw std::invalid_argument("Invalid general-book module ID");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  std::unique_ptr<ScopedEngineOptions> scope;
  if (options) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *options);
  std::unique_ptr<ScopedNoteNumbers> note_scope;
  if (labels) note_scope = std::make_unique<ScopedNoteNumbers>(impl_->note_rendering, *module, *labels);
  return detail::capture_book_document(*module, offset, limits, labels != nullptr);
}
BookResourceContext Reader::book_resource_context(const std::string &id) {
  if (id.empty() || id.find('\0') != std::string::npos) throw std::invalid_argument("Invalid general-book module ID");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  return detail::capture_book_resource_context(*module);
}
BookImageResource Reader::read_book_image(const std::string &id, const std::string &uri, std::size_t maximum_bytes) {
  if (id.empty() || id.find('\0') != std::string::npos) throw std::invalid_argument("Invalid general-book module ID");
  auto command = xiphos::core::book_image_link(uri);
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  return detail::capture_book_image_resource(*module, std::move(command), maximum_bytes);
}
BookImageResource Reader::read_book_inline_image(const std::string &id, const std::string &source, std::size_t maximum_bytes) {
  if (id.empty() || id.find('\0') != std::string::npos) throw std::invalid_argument("Invalid general-book module ID");
  auto command = xiphos::core::book_inline_image_link(source);
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  return detail::capture_book_image_resource(*module, std::move(command), maximum_bytes);
}
std::optional<BookStylesheetResource> Reader::read_book_stylesheet(const std::string &id, std::size_t maximum_bytes) {
  if (id.empty() || id.find('\0') != std::string::npos) throw std::invalid_argument("Invalid general-book module ID");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  return detail::capture_book_stylesheet_resource(*module, maximum_bytes);
}
BookNote Reader::book_note(const std::string &id, std::uint64_t offset, const std::string &identifier,
                           const EngineOptions &options, bool render_body) {
  return book_note_impl(id, offset, identifier, &options, render_body);
}
BookNote Reader::book_note(const std::string &id, std::uint64_t offset, const std::string &identifier, bool render_body) {
  return book_note_impl(id, offset, identifier, nullptr, render_body);
}
BookNote Reader::book_note_impl(const std::string &id, std::uint64_t offset, const std::string &identifier,
                                const EngineOptions *options, bool render_body) {
  if (id.empty() || id.find('\0') != std::string::npos) throw std::invalid_argument("Invalid general-book module ID");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  std::unique_ptr<ScopedEngineOptions> scope;
  if (options) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *options);
  return detail::capture_book_note(*module, offset, identifier, render_body);
}
BookEntry Reader::book_impl(const std::string &id, std::uint64_t offset, const EngineOptions *options) {
  if (id.empty() || id.find('\0') != std::string::npos) throw std::invalid_argument("Invalid general-book module ID");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  std::unique_ptr<ScopedEngineOptions> scope;
  if (options) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *options);
  return detail::capture_book_entry(*module, offset);
}
ReferenceChoices Reader::reference_choices(const std::string &id, const std::string &reference) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return xiphos::core::reference_choices(impl_->verse_module(id, reference), reference);
}
PassageLink Reader::resolve_link(const std::string &current_module, const std::string &url) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto target = passage_link(url, current_module);
  impl_->verse_module(target.module, target.reference);
  return target;
}
ReferenceList Reader::read_reference_list(const std::string &id, const std::string &expression,
                                         const std::string &context) {
  return reference_list_impl(id, expression, context, nullptr);
}
ReferenceList Reader::read_reference_list(const std::string &id, const std::string &expression,
                                         const std::string &context, const EngineOptions &options) {
  return reference_list_impl(id, expression, context, &options);
}
ParsedReferenceList Reader::reference_targets(const std::string &id, const std::string &expression,
                                               const std::string &context) {
  if (id.find('\0') != std::string::npos || expression.find('\0') != std::string::npos || context.find('\0') != std::string::npos)
    throw std::invalid_argument("Reference-list input contains NUL");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return parse_reference_list(impl_->verse_module(id, context), expression, context);
}
ReferenceList Reader::reference_list_impl(const std::string &id, const std::string &expression,
                                         const std::string &context, const EngineOptions *options) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto &module = impl_->verse_module(id, context);
  const auto parsed = parse_reference_list(module, expression, context);
  struct RestoreKey {
    sword::SWModule &module;
    std::string reference;
    ~RestoreKey() { module.getKey()->setText(reference.c_str()); }
  } restore{module, module.getKeyText()};
  std::unique_ptr<ScopedEngineOptions> scope;
  if (options) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *options);
  ReferenceList result;
  result.module = id; result.context = parsed.context; result.expression = expression;
  result.limited = parsed.limited; result.skipped = parsed.skipped;
  result.entries.reserve(parsed.references.size());
  for (const auto &reference : parsed.references) {
    auto html = render_entry(module, reference.c_str());
    if (module.getKey()->getError()) throw std::invalid_argument("SWORD could not render a reference-list entry");
    result.entries.push_back({module.getKeyText(), std::move(html)});
  }
  result.html = display_reference_list(result);
  return result;
}
HeadingNoteSource Reader::heading_notes(const std::string &id, const std::string &reference,
                                        std::size_t fragment, const EngineOptions &options,
                                        bool labels, bool render_bodies) {
  if (id.find('\0') != std::string::npos || reference.empty() || reference.find('\0') != std::string::npos)
    throw std::invalid_argument("Heading source requires a valid module and reference");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto &module = impl_->verse_module(id, reference);
  ScopedEngineOptions engine(*impl_->manager, options);
  ScopedCommentaryHeadings headings(*impl_->manager);
  ScopedNoteNumbers notes(impl_->note_rendering, module, labels);
  module.setKey(reference.c_str());
  return capture_scoped_preverse_heading_notes(module, fragment, render_bodies);
}
StudyNote Reader::study_note(const std::string &id, const std::string &reference,
                             const std::string &identifier) {
  return note_impl(id, reference, identifier, nullptr);
}
StudyNote Reader::study_note(const std::string &id, const std::string &reference,
                             const std::string &identifier, const EngineOptions &options) {
  return note_impl(id, reference, identifier, &options);
}
StudyNote Reader::note_impl(const std::string &id, const std::string &reference,
                            const std::string &identifier, const EngineOptions *options, bool render_body) {
  if (id.find('\0') != std::string::npos) throw std::invalid_argument("Note module name contains NUL");
  HeadingNoteTarget heading;
  const bool scoped_heading = parse_heading_note_identifier(identifier, heading);
  if (scoped_heading && !options) throw std::invalid_argument("Heading-note lookup requires configured engine options");
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto &module = impl_->verse_module(id, reference);
  if (scoped_heading) {
    auto capture = *options;
    // Display can hide markers; an already-issued heading link must still be
    // resolved from complete source metadata. Never save this temporary setting.
    capture.footnotes = true;
    ScopedEngineOptions engine(*impl_->manager, capture);
    ScopedCommentaryHeadings headings(*impl_->manager);
    if (reference.empty() || reference.find('\0') != std::string::npos)
      throw std::invalid_argument("Heading note requires a valid reference");
    module.setKey(reference.c_str());
    HeadingNoteSource source;
    try { source = capture_scoped_preverse_heading_notes(module, heading.fragment, render_body); }
    catch (const std::out_of_range &) { throw std::invalid_argument("Heading note is unavailable"); }
    const auto found = source.notes.find(identifier);
    if (found == source.notes.end()) throw std::invalid_argument("Heading note is unavailable");
    return found->second;
  }
  std::unique_ptr<ScopedEngineOptions> scope;
  if (options) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *options);
  return render_body ? render_study_note(module, reference, identifier)
                     : read_study_note_metadata(module, reference, identifier);
}
StudyNote Reader::study_note_metadata(const std::string &id, const std::string &reference,
                                       const std::string &identifier) {
  return note_impl(id, reference, identifier, nullptr, false);
}
StudyNote Reader::study_note_metadata(const std::string &id, const std::string &reference,
                                       const std::string &identifier, const EngineOptions &options) {
  return note_impl(id, reference, identifier, &options, false);
}
std::string Reader::select_lexicon(const LexicalLink &command, const std::string &source_reference,
                                  int origin, const LexiconPreferences &preferences) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (origin != XIPHOS_LEXICON_MAIN && origin != XIPHOS_LEXICON_DIALOG)
    throw std::invalid_argument("Invalid lexicon origin");
  if (command.kind != LexicalLinkKind::strongs && command.kind != LexicalLinkKind::morphology)
    throw std::invalid_argument("Invalid lexical kind");
  if (command.type.find('\0') != std::string::npos || command.module.find('\0') != std::string::npos ||
      source_reference.find('\0') != std::string::npos)
    throw std::invalid_argument("Lexical source context contains NUL");
  const auto borrowed = borrow_lexicon_preferences(preferences);
  struct Context { Impl *impl; const LexicalLink *command; const std::string *reference; };
  Context source{impl_.get(), &command, &source_reference};
  const XiphosLexiconContext context{&source,
    [](void *user, const char *id) -> int {
      return static_cast<Context *>(user)->impl->manager->getModule(id) != nullptr;
    },
    [](void *user) -> int {
      auto &source = *static_cast<Context *>(user);
      auto &module = source.impl->verse_module(source.command->module, *source.reference);
      sword::VerseKey key(*static_cast<sword::VerseKey *>(module.getKey()));
      key.setAutoNormalize(true);
      key.setText(source.reference->c_str());
      if (key.getError() || key.getChapter() < 1 || key.getVerse() < 1)
        throw std::invalid_argument("SWORD could not resolve lexical source context");
      return key.getTestament();
    }};
  const auto *selected = xiphos_core_select_lexicon(origin,
    command.kind == LexicalLinkKind::strongs ? XIPHOS_LEXICON_STRONGS : XIPHOS_LEXICON_MORPHOLOGY,
    command.type.c_str(), &borrowed, &context);
  if (!selected || !*selected) throw std::invalid_argument("No lexicon selected for this command");
  return selected;
}
DictionaryEntry Reader::read_dictionary(const std::string &module, const std::string &key) {
  return dictionary_impl(module, key, nullptr);
}
DictionaryEntry Reader::read_dictionary(const std::string &module, const std::string &key, const EngineOptions &options) {
  return dictionary_impl(module, key, &options);
}
DictionaryChoices Reader::dictionary_choices(const std::string &id, const std::string &key) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (id.empty() || key.empty() || id.find('\0') != std::string::npos || key.find('\0') != std::string::npos)
    throw std::invalid_argument("Dictionary module/key is empty or contains NUL");
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  if (!dynamic_cast<sword::SWLD *>(module)) throw std::invalid_argument("Module is not a dictionary");
  const std::string saved = module->getKeyText();
  struct Restore { sword::SWModule &module; const std::string &key; ~Restore() { module.setKey(key.c_str()); } } restore{*module, saved};
  module->setKey(key.c_str());
  const std::string resolved = module->getKeyText();
  return {id, resolved, dictionary_nearby_keys(*module)};
}
DictionaryEntry Reader::navigate_dictionary(const std::string &module, const std::string &key, int direction) {
  if (direction != -1 && direction != 1) throw std::invalid_argument("Invalid dictionary direction");
  return dictionary_impl(module, key, nullptr, direction);
}
DictionaryEntry Reader::navigate_dictionary(const std::string &module, const std::string &key, int direction, const EngineOptions &options) {
  if (direction != -1 && direction != 1) throw std::invalid_argument("Invalid dictionary direction");
  return dictionary_impl(module, key, &options, direction);
}
DictionaryEntry Reader::dictionary_impl(const std::string &id, const std::string &key, const EngineOptions *options, int direction) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (key.empty() || id.empty() || key.find('\0') != std::string::npos || id.find('\0') != std::string::npos)
    throw std::invalid_argument("Dictionary module/key is empty or contains NUL");
  auto *module = impl_->manager->getModule(id.c_str());
  if (!module) throw std::out_of_range("Module is not installed");
  if (!dynamic_cast<sword::SWLD *>(module)) throw std::invalid_argument("Module is not a dictionary");
  std::unique_ptr<ScopedEngineOptions> scope;
  if (options) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *options);
  std::string target = key;
  if (direction) {
    // Legacy main_dictionary_button_clicked: set current key, step, copy key,
    // then activate the entry. Preserve SDK boundary/matching behavior.
    module->setKey(key.c_str());
    target = dictionary_step_key(*module, direction);
  }
  DictionaryEntry result{id, "", render_entry(*module, target.c_str())};
  result.key = module->getKeyText();
  return result;
}
Entry Reader::read_commentary_entry(const std::string &id, const std::string &reference, bool headings) {
  return read_impl(id, reference, nullptr, headings, true);
}
Entry Reader::read_commentary_entry(const std::string &id, const std::string &reference, bool headings, const EngineOptions &options) {
  return read_impl(id, reference, &options, headings, true);
}
Entry Reader::read(const std::string &id, const std::string &reference) {
  return read_impl(id, reference, nullptr);
}
Entry Reader::read(const std::string &id, const std::string &reference, const EngineOptions &options) {
  return read_impl(id, reference, &options);
}
Entry Reader::read(const std::string &id, const std::string &reference, const EngineOptions &options, bool labels) {
  return read_impl(id, reference, &options, false, false, &labels);
}
Entry Reader::read_commentary_entry(const std::string &id, const std::string &reference, bool headings, const EngineOptions &options, bool labels) {
  return read_impl(id, reference, &options, headings, true, &labels);
}
Entry Reader::read_impl(const std::string &id, const std::string &reference, const EngineOptions *options,
                        bool headings, bool commentary_only, const bool *labels) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto *module = &impl_->verse_module(id, reference);
  if (commentary_only && std::string(module->getType()) != "Commentaries")
    throw std::invalid_argument("Commentary entry requires a commentary module");
  std::unique_ptr<ScopedEngineOptions> scope;
  if (options) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *options);
  std::unique_ptr<ScopedNoteNumbers> note_scope;
  if (labels) note_scope = std::make_unique<ScopedNoteNumbers>(impl_->note_rendering, *module, *labels);
  std::unique_ptr<ScopedCommentaryHeadings> heading_scope;
  if (commentary_only) heading_scope = std::make_unique<ScopedCommentaryHeadings>(*impl_->manager);
  // Resolve using the module's own versification and SWORD normalization,
  // never an independent canon/reference parser in a frontend.
  Entry result;
  result.html = render_entry(*module, reference.c_str());
  if (module->getKey()->getError())
    throw std::invalid_argument("SWORD could not resolve the reference");
  result.reference = module->getKeyText();
  if (labels) copy_note_source(*module, result);
  if (headings) copy_headings(*module, result, labels != nullptr);
  return result;
}
Chapter Reader::read_chapter(const std::string &id, const std::string &reference,
                             const DisplayOptions &options) {
  return chapter_impl(id, reference, options, nullptr);
}
Chapter Reader::read_chapter(const std::string &id, const std::string &reference,
                             const DisplayOptions &display, const EngineOptions &engine) {
  return chapter_impl(id, reference, display, &engine);
}
Chapter Reader::read_chapter(const std::string &id, const std::string &reference,
                             const DisplayOptions &display, const EngineOptions &engine, bool labels) {
  return chapter_impl(id, reference, display, &engine, &labels);
}
Chapter Reader::chapter_impl(const std::string &id, const std::string &reference,
                             const DisplayOptions &options, const EngineOptions *engine, const bool *labels) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto &module = impl_->verse_module(id, reference);
  std::unique_ptr<ScopedEngineOptions> scope;
  if (engine) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *engine);
  std::unique_ptr<ScopedNoteNumbers> note_scope;
  if (labels) note_scope = std::make_unique<ScopedNoteNumbers>(impl_->note_rendering, module, *labels);
  // Clone the module key so its own locale/versification drives enumeration.
  // Work on this clone, avoiding persistent changes to the module key flags.
  sword::VerseKey key(*static_cast<sword::VerseKey *>(module.getKey()));
  key.setAutoNormalize(true);
  key.setText(reference.c_str());
  if (key.getError() || key.getChapter() < 1 || key.getVerse() < 1)
    throw std::invalid_argument("SWORD could not resolve a chapter");
  Chapter result;
  result.reference = key.getText();
  result.selected_verse = key.getVerse();
  result.selected_anchor = "verse-" + std::to_string(result.selected_verse);
  result.title = std::string(key.getBookName()) + " " + std::to_string(key.getChapter());
  key.setSuffix(0);
  const auto count = key.getVerseMax();
  if (count <= 0) throw std::invalid_argument("Chapter has no verse positions");
  result.verses.reserve(count);
  for (int verse = 1; verse <= count; ++verse) {
    key.setVerse(verse);
    const std::string resolved = key.getText();
    result.verses.push_back({resolved, render_entry(module, resolved.c_str())});
    if (labels) copy_note_source(module, result.verses.back());
    result.verse_ordinals.push_back(static_cast<unsigned>(verse));
  }
  // Keep the module at the requested anchor, as with a single-entry read.
  module.setKey(result.reference.c_str());
  result.html = display_chapter(result, options);
  return result;
}
Chapter Reader::read_commentary_chapter(const std::string &id, const std::string &reference,
                                        const DisplayOptions &display, bool headings) {
  return commentary_chapter_impl(id, reference, display, nullptr, headings);
}
Chapter Reader::read_commentary_chapter(const std::string &id, const std::string &reference,
                                        const DisplayOptions &display, const EngineOptions &engine, bool headings) {
  return commentary_chapter_impl(id, reference, display, &engine, headings);
}
Chapter Reader::read_commentary_chapter(const std::string &id, const std::string &reference,
                                        const DisplayOptions &display, const EngineOptions &engine, bool headings, bool labels) {
  return commentary_chapter_impl(id, reference, display, &engine, headings, &labels);
}
Chapter Reader::commentary_chapter_impl(const std::string &id, const std::string &reference,
                                         const DisplayOptions &display, const EngineOptions *engine, bool headings, const bool *labels) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  auto &module = impl_->verse_module(id, reference);
  if (std::string(module.getType()) != "Commentaries")
    throw std::invalid_argument("Commentary chapter requires a commentary module");
  const auto references = commentary_positions(module, reference);
  sword::VerseKey key(*static_cast<sword::VerseKey *>(module.getKey()));
  key.setAutoNormalize(true); key.setText(reference.c_str());
  Chapter result;
  result.reference = key.getText(); result.selected_verse = key.getVerse();
  result.selected_anchor = "commentary-verse-" + std::to_string(result.selected_verse);
  result.title = std::string(key.getBookName()) + " " + std::to_string(key.getChapter());
  std::unique_ptr<ScopedEngineOptions> scope;
  if (engine) scope = std::make_unique<ScopedEngineOptions>(*impl_->manager, *engine);
  std::unique_ptr<ScopedNoteNumbers> note_scope;
  if (labels) note_scope = std::make_unique<ScopedNoteNumbers>(impl_->note_rendering, module, *labels);
  ScopedCommentaryHeadings heading_scope(*impl_->manager);
  std::vector<unsigned> positions;
  for (const auto &position : references) {
    key.setText(position.c_str());
    positions.push_back(static_cast<unsigned>(key.getVerse()));
    auto html = render_entry(module, position.c_str());
    if (module.getKey()->getError()) throw std::invalid_argument("SWORD could not render a commentary section");
    Entry entry{module.getKeyText(), std::move(html)};
    if (labels) copy_note_source(module, entry);
    if (headings) copy_headings(module, entry, labels != nullptr);
    result.verses.push_back(std::move(entry));
  }
  module.setKey(result.reference.c_str());
  result.verse_ordinals = std::move(positions);
  result.html = display_commentary_chapter(result, result.verse_ordinals, display);
  return result;
}
std::string Reader::adjacent_chapter(const std::string &id, const std::string &reference,
                                    int direction) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (direction != -1 && direction != 1) throw std::invalid_argument("Invalid chapter direction");
  auto &module = impl_->verse_module(id, reference);
  sword::VerseKey key(*static_cast<sword::VerseKey *>(module.getKey()));
  key.setIntros(false);
  key.setAutoNormalize(true);
  key.setText(reference.c_str());
  if (key.getError()) throw std::invalid_argument("SWORD could not resolve the reference");
  key.setSuffix(0);
  key.setVerse(1);
  const std::string before = key.getText();
  key.setChapter(key.getChapter() + direction);
  if (key.getError() || before == key.getText())
    throw std::invalid_argument("Chapter destination is unavailable");
  return key.getText();
}
}
