// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/c_api.h"
#include "xiphos/core/reader.hpp"
#include <stdexcept>
#include <chrono>
#include <type_traits>
#include "xiphos/core/worker.hpp"
#include "xiphos/core/session.hpp"
#include "xiphos/core/study_link_admission.hpp"
#include "search_control.hpp"
struct xc_reader { std::shared_ptr<xiphos::core::Reader> value; explicit xc_reader(const char *p) : value(std::make_shared<xiphos::core::Reader>(p ? p : "")) {} };
struct xc_session {
  std::shared_ptr<xiphos::core::Reader> owner;
  xiphos::core::StudySession value;
  explicit xc_session(xc_reader &r) : owner(r.value), value(*owner) {}
};
struct xc_chapter { xiphos::core::Chapter value; };
struct xc_choices { xiphos::core::ReferenceChoices value; };
struct xc_link { xiphos::core::PassageLink value; };
struct xc_search { xiphos::core::SearchResult value; };
struct xc_search_batch {
  xiphos::core::SearchBatchResult value;
  std::vector<std::unique_ptr<xc_search>> results;
  explicit xc_search_batch(xiphos::core::SearchBatchResult v):value(std::move(v)) {
    results.reserve(value.groups.size());
    for(auto &group:value.groups){
      std::unique_ptr<xc_search> result;
      if(group.result)result=std::make_unique<xc_search>(xc_search{std::move(*group.result)});
      results.push_back(std::move(result));
    }
  }
};
struct xc_reference_list { xiphos::core::ReferenceList value; };
struct xc_search_lists { xiphos::core::SavedSearchLists value; };
struct xc_search_module_names { std::vector<std::string> value; };
struct xc_module_resolution { xiphos::core::ModuleNameResolution value; };
struct xc_lexicons { std::optional<xiphos::core::LexiconPreferences> value; };
struct xc_lexical_entry { xiphos::core::LexicalEntry value; };
struct xc_dictionary_choices { xiphos::core::DictionaryChoices value; };
struct xc_dictionary_history { xiphos::core::DictionaryHistorySnapshot value; };
struct xc_dictionary_entry { xiphos::core::DictionaryEntry value; };
struct xc_book_tree { xiphos::core::BookTree value; };
struct xc_book_children { xiphos::core::BookChildren value; };
struct xc_book_entry {
  xc_book_tree tree;
  bool has_entry;
  std::string html;
  explicit xc_book_entry(xiphos::core::BookEntry value)
    : tree{std::move(value.tree)}, has_entry(value.has_entry), html(std::move(value.html)) {}
};
struct xc_book_document {
  struct Section {
    xc_book_entry entry;
    std::uint32_t depth;
    bool rendered;
    explicit Section(xiphos::core::BookDocumentSection value)
      : entry(std::move(value.entry)), depth(value.depth), rendered(value.rendered) {}
  };
  xc_book_tree tree;
  std::uint32_t display_level;
  std::vector<Section> sections;
  std::string html;
  std::optional<xiphos::core::BookDocumentPresentation> presentation;
  std::optional<xiphos::core::BookDocumentNoteSource> note_source;
  explicit xc_book_document(xiphos::core::BookDocument value)
    : tree{std::move(value.tree)}, display_level(value.display_level), html(std::move(value.html)), presentation(std::move(value.presentation)), note_source(std::move(value.note_source)) {
    sections.reserve(value.sections.size());
    for (auto &section : value.sections) sections.emplace_back(std::move(section));
  }
};
struct xc_verse_image {xiphos::core::VerseImageResource value;};
struct xc_book_image { xiphos::core::BookImageTarget value; };
struct xc_book_image_data {
  xc_book_image source;
  std::string bytes;
  explicit xc_book_image_data(xiphos::core::BookImageResource value)
    : source{std::move(value.target)}, bytes(std::move(value.data)) {}
};
struct xc_note { xiphos::core::StudyNote value; };
struct xc_book_note {
  xc_book_tree tree;
  xc_note content;
  explicit xc_book_note(xiphos::core::BookNote value)
    : tree{std::move(value.tree)}, content{std::move(value.note)} {}
};
struct xc_passage { xiphos::core::StudyPassage value; };
struct xc_entry { xiphos::core::Entry value; };
struct xc_search_html_export {xiphos::core::SearchHtmlExport value;};
struct xc_search_text_export {xiphos::core::SearchTextExport value;};
struct xc_worker { xiphos::core::StudyWorker value; explicit xc_worker(const char *root) : value(root ? root : "") {} };
struct xc_catalog { std::vector<xiphos::core::Module> value; };
struct xc_history { xiphos::core::HistorySnapshot value; };
struct xc_request {
  xc_status status = XC_PENDING;
  uint64_t tab = 0;
  bool options_result = false;
  bool book_selection_result = false, book_selection_present = false;
  bool preview_preference_result = false, preview_preference = true, no_presentation = false;
  std::optional<xiphos::core::EngineOptions> options;
  std::optional<xiphos::core::ModulePresentationOptions> presentation;
  std::unique_ptr<xc_reference_list> references;
  std::unique_ptr<xc_dictionary_choices> dictionary_choices;
  std::unique_ptr<xc_dictionary_history> dictionary_history;
  std::unique_ptr<xc_dictionary_entry> dictionary;
  std::unique_ptr<xc_book_tree> book_tree;
  std::unique_ptr<xc_book_entry> book_entry;
  std::shared_ptr<xiphos::core::SearchControl> search_control;
  std::unique_ptr<xc_search> search_result;
  std::unique_ptr<xc_search_batch> search_batch;
  std::unique_ptr<xc_search_text_export> search_export;
  std::unique_ptr<xc_search_html_export> search_html_export;
  std::optional<xiphos::core::SearchScopeInfo> search_scope;
  std::optional<xiphos::core::SearchTextState> search_text_state;
  std::optional<xiphos::core::SearchGenerationInfo> search_generation;
  std::optional<xiphos::core::SearchRetentionLimits> search_retention_limits;
  std::optional<xiphos::core::SearchGenerationRead> search_generation_read;
  std::unique_ptr<xc_book_document> book_document;
  std::unique_ptr<xc_book_image> book_image;
  std::unique_ptr<xc_book_image_data> book_image_data;
  std::unique_ptr<xc_verse_image> verse_image;
  std::unique_ptr<xc_book_children> book_children;
  std::unique_ptr<xc_book_note> book_note;
  std::unique_ptr<xc_lexical_entry> lexical;
  std::unique_ptr<xc_lexicons> lexicons;
  std::unique_ptr<xc_search_lists> search_lists;
  std::unique_ptr<xc_search_module_names> search_module_names;
  std::unique_ptr<xc_module_resolution> module_resolution;
  std::unique_ptr<xc_note> note;
  std::unique_ptr<xc_entry> entry;
  std::unique_ptr<xc_passage> passage;
  std::unique_ptr<xc_chapter> chapter;
  std::unique_ptr<xc_choices> choices;
  std::unique_ptr<xc_link> link;
  std::unique_ptr<xc_catalog> catalog;
  std::unique_ptr<xc_history> history;
  virtual ~xc_request() = default;
  virtual void update() = 0;
  void store(xiphos::core::SearchHtmlExport v){search_html_export=std::make_unique<xc_search_html_export>(xc_search_html_export{std::move(v)});}
  void store(xiphos::core::SearchTextExport v){search_export=std::make_unique<xc_search_text_export>(xc_search_text_export{std::move(v)});}
  void store(xiphos::core::SearchGenerationInfo v){search_generation=v;}
  void store(xiphos::core::SearchRetentionLimits v){search_retention_limits=v;}
  void store(xiphos::core::SearchGenerationRead v){
    if(v.result)search_result=std::make_unique<xc_search>(xc_search{std::move(*v.result)});
    v.result.reset();search_generation=v.generation;search_generation_read=std::move(v);
  }
  void store(xiphos::core::SearchTextState v) {search_text_state=std::move(v);}
  void store(xiphos::core::SearchScopeInfo v) {search_scope=v;}
  void store(xiphos::core::SearchBatchResult v) { search_batch=std::make_unique<xc_search_batch>(std::move(v)); }
  void store(xiphos::core::SearchResult v) { search_result = std::make_unique<xc_search>(xc_search{std::move(v)}); }
  void store(xiphos::core::StudyPassage v) { passage = std::make_unique<xc_passage>(xc_passage{std::move(v)}); }
  void store(xiphos::core::ModulePresentationOptions v) { presentation = v; }
  void store(uint64_t v) { tab = v; }
  void store(bool v) { preview_preference = v; preview_preference_result = true; }
  void store(std::monostate) { no_presentation = true; }
  void store(xiphos::core::StudyHoverLink v) { std::visit([this](auto value) { store(std::move(value)); }, std::move(v)); }
  void store(std::optional<xiphos::core::EngineOptions> v) { options = v; options_result = true; }
  void store(xiphos::core::ContextualStudyLink v) { std::visit([this](auto value) { store(std::move(value)); }, std::move(v)); }
  void store(xiphos::core::ResolvedStudyLink v) { std::visit([this](auto value) { store(std::move(value)); }, std::move(v)); }
  void store(xiphos::core::ReferenceList v) { references = std::make_unique<xc_reference_list>(xc_reference_list{std::move(v)}); }
  void store(xiphos::core::SavedSearchLists v) {
    static_assert(std::is_nothrow_move_assignable_v<xiphos::core::SavedSearchLists>, "Prepared save snapshots must transfer without allocating");
    if (search_lists) search_lists->value = std::move(v);
    else search_lists = std::make_unique<xc_search_lists>(xc_search_lists{std::move(v)});
  }
  void store(xiphos::core::ModuleNameResolution v) { module_resolution = std::make_unique<xc_module_resolution>(xc_module_resolution{std::move(v)}); }
  void store(std::vector<std::string> v) { search_module_names = std::make_unique<xc_search_module_names>(xc_search_module_names{std::move(v)}); }
  void store(std::optional<xiphos::core::LexiconPreferences> v) { lexicons = std::make_unique<xc_lexicons>(xc_lexicons{std::move(v)}); }
  void store(xiphos::core::LexicalEntry v) { lexical = std::make_unique<xc_lexical_entry>(xc_lexical_entry{std::move(v)}); }
  void store(xiphos::core::DictionaryChoices v) { dictionary_choices = std::make_unique<xc_dictionary_choices>(xc_dictionary_choices{std::move(v)}); }
  void store(xiphos::core::DictionaryHistorySnapshot v) { dictionary_history = std::make_unique<xc_dictionary_history>(xc_dictionary_history{std::move(v)}); }
  void store(xiphos::core::DictionaryEntry v) { dictionary = std::make_unique<xc_dictionary_entry>(xc_dictionary_entry{std::move(v)}); }
  void store(std::optional<xiphos::core::BookDocument> v) {
    book_selection_result = true;
    book_selection_present = v.has_value();
    if (v) book_document = std::make_unique<xc_book_document>(std::move(*v));
  }
  void store(xiphos::core::BookFollowLink v) { std::visit([this](auto value) { store(std::move(value)); }, std::move(v)); }
  void store(xiphos::core::VerseImageResource v){verse_image=std::make_unique<xc_verse_image>(xc_verse_image{std::move(v)});}
  void store(xiphos::core::BookImageResource v) { book_image_data = std::make_unique<xc_book_image_data>(std::move(v)); }
  void store(xiphos::core::BookImageTarget v) { book_image = std::make_unique<xc_book_image>(xc_book_image{std::move(v)}); }
  void store(xiphos::core::BookDocument v) { book_document = std::make_unique<xc_book_document>(std::move(v)); }
  void store(xiphos::core::BookTree v) { book_tree = std::make_unique<xc_book_tree>(xc_book_tree{std::move(v)}); }
  void store(std::optional<xiphos::core::BookTree> v) {
    book_selection_result = true;
    book_selection_present = v.has_value();
    if (v) store(std::move(*v));
  }
  void store(std::optional<xiphos::core::BookEntry> v) {
    book_selection_result = true;
    book_selection_present = v.has_value();
    if (v) store(std::move(*v));
  }
  void store(xiphos::core::BookEntry v) { book_entry = std::make_unique<xc_book_entry>(std::move(v)); }
  void store(xiphos::core::BookChildren v) { book_children = std::make_unique<xc_book_children>(xc_book_children{std::move(v)}); }
  void store(xiphos::core::BookNote v) { book_note = std::make_unique<xc_book_note>(std::move(v)); }
  void store(xiphos::core::StudyNote v) { note = std::make_unique<xc_note>(xc_note{std::move(v)}); }
  void store(xiphos::core::Entry v) { entry = std::make_unique<xc_entry>(xc_entry{std::move(v)}); }
  void store(xiphos::core::Chapter v) { chapter = std::make_unique<xc_chapter>(xc_chapter{std::move(v)}); }
  void store(xiphos::core::ReferenceChoices v) { choices = std::make_unique<xc_choices>(xc_choices{std::move(v)}); }
  void store(xiphos::core::PassageLink v) { link = std::make_unique<xc_link>(xc_link{std::move(v)}); }
  void store(std::vector<xiphos::core::Module> v) { catalog = std::make_unique<xc_catalog>(xc_catalog{std::move(v)}); }
  void store(xiphos::core::HistorySnapshot v) { history = std::make_unique<xc_history>(xc_history{std::move(v)}); }
};
namespace {
xiphos::core::SavedBookSelection copy_book_selection(const xc_saved_book_selection &saved) {
  const auto copy = [](const char *text) -> std::optional<std::string> {
    return text ? std::optional<std::string>(text) : std::nullopt;
  };
  return {copy(saved.module), copy(saved.key), copy(saved.offset_text)};
}
std::optional<xiphos::core::BookNavigation> copy_book_navigation(int direction) {
  using xiphos::core::BookNavigation;
  switch (direction) {
    case XC_BOOK_PARENT_DIRECTION: return BookNavigation::parent;
    case XC_BOOK_FIRST_CHILD_DIRECTION: return BookNavigation::first_child;
    case XC_BOOK_PREVIOUS_SIBLING_DIRECTION: return BookNavigation::previous_sibling;
    case XC_BOOK_NEXT_SIBLING_DIRECTION: return BookNavigation::next_sibling;
    default: return std::nullopt;
  }
}
xc_status borrow_lexical_presentation(const xiphos::core::LexicalPresentation *value,
                                      xc_lexical_presentation *out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = {nullptr, 0, 0};
  if (!value) return XC_INVALID_ARGUMENT;
  *out = {value->css.c_str(), value->requested ? 1 : 0, value->applied ? 1 : 0};
  return XC_OK;
}
xiphos::core::EngineOptions options_from_flags(unsigned flags) {
  xiphos::core::EngineOptions options;
  options.strongs = (flags & XC_ENGINE_STRONGS) != 0;
  options.morphs = (flags & XC_ENGINE_MORPHOLOGY) != 0;
  options.footnotes = (flags & XC_ENGINE_FOOTNOTES) != 0;
  options.greekaccents = (flags & XC_ENGINE_GREEK_ACCENTS) != 0;
  options.lemmas = (flags & XC_ENGINE_LEMMAS) != 0;
  options.scripturerefs = (flags & XC_ENGINE_CROSS_REFERENCES) != 0;
  options.hebrewpoints = (flags & XC_ENGINE_HEBREW_VOWELS) != 0;
  options.hebrewcant = (flags & XC_ENGINE_HEBREW_CANTILLATION) != 0;
  options.italic_headings = (flags & XC_ENGINE_ITALIC_HEADINGS) != 0;
  options.words_in_red = (flags & XC_ENGINE_WORDS_IN_RED) != 0;
  options.xlit = (flags & XC_ENGINE_TRANSLITERATED_FORMS) != 0;
  options.enumerated = (flags & XC_ENGINE_ENUMERATIONS) != 0;
  options.glosses = (flags & XC_ENGINE_GLOSSES) != 0;
  options.morphseg = (flags & XC_ENGINE_MORPHEME_SEGMENTATION) != 0;
  options.transliteration = (flags & XC_ENGINE_TRANSLITERATION) != 0;
  options.variants_primary = (flags & XC_ENGINE_VARIANTS_PRIMARY) != 0;
  options.variants_secondary = (flags & XC_ENGINE_VARIANTS_SECONDARY) != 0;
  options.variants_all = (flags & XC_ENGINE_VARIANTS_ALL) != 0;
  return options;
}
unsigned options_to_flags(const xiphos::core::EngineOptions &o) {
  unsigned flags = 0;
  if (o.strongs) flags |= XC_ENGINE_STRONGS;
  if (o.morphs) flags |= XC_ENGINE_MORPHOLOGY;
  if (o.footnotes) flags |= XC_ENGINE_FOOTNOTES;
  if (o.greekaccents) flags |= XC_ENGINE_GREEK_ACCENTS;
  if (o.lemmas) flags |= XC_ENGINE_LEMMAS;
  if (o.scripturerefs) flags |= XC_ENGINE_CROSS_REFERENCES;
  if (o.hebrewpoints) flags |= XC_ENGINE_HEBREW_VOWELS;
  if (o.hebrewcant) flags |= XC_ENGINE_HEBREW_CANTILLATION;
  if (o.italic_headings) flags |= XC_ENGINE_ITALIC_HEADINGS;
  if (o.words_in_red) flags |= XC_ENGINE_WORDS_IN_RED;
  if (o.xlit) flags |= XC_ENGINE_TRANSLITERATED_FORMS;
  if (o.enumerated) flags |= XC_ENGINE_ENUMERATIONS;
  if (o.glosses) flags |= XC_ENGINE_GLOSSES;
  if (o.morphseg) flags |= XC_ENGINE_MORPHEME_SEGMENTATION;
  if (o.transliteration) flags |= XC_ENGINE_TRANSLITERATION;
  if (o.variants_primary) flags |= XC_ENGINE_VARIANTS_PRIMARY;
  if (o.variants_secondary) flags |= XC_ENGINE_VARIANTS_SECONDARY;
  if (o.variants_all) flags |= XC_ENGINE_VARIANTS_ALL;
  return flags;
}
xc_status failure() noexcept {
  try { throw; }
  catch (const std::out_of_range &) { return XC_MODULE_NOT_FOUND; }
  catch (const std::invalid_argument &) { return XC_INVALID_ARGUMENT; }
  catch (...) { return XC_ENGINE_ERROR; }
}
template<class T> struct Request final : xc_request {
  std::future<T> future;
  void update() override {
    if (status != XC_PENDING || future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    try {
      if constexpr (std::is_void_v<T>) future.get();
      else store(future.get());
      status = XC_OK;
    } catch (...) { xiphos::core::detail::SearchControlAccess::fail_if_queued(search_control); status = failure(); }
  }
};
template<class F> struct FutureValue;
template<class T> struct FutureValue<std::future<T>> { using Type = T; };
static_assert(XC_STUDY_LINK_MAX_URI_BYTES == xiphos::core::maximum_study_link_uri_bytes, "C/core URI byte limits differ");
static_assert(XC_STUDY_LINK_MAX_CONTEXT_BYTES == xiphos::core::maximum_study_link_context_bytes, "C/core context byte limits differ");
bool admitted_link_context(const char *value, bool required = false) {
  return xiphos::core::detail::study_link_c_text(value, xiphos::core::maximum_study_link_context_bytes, required);
}
bool admitted_link_uri(const char *value) {
  return xiphos::core::detail::study_link_c_text(value, xiphos::core::maximum_study_link_uri_bytes, true);
}
bool bounded_image_c_text(const char *text,std::size_t maximum){
  if(!text||!*text)return false;
  for(std::size_t i=0;i<=maximum;++i)if(!text[i])return true;
  return false;
}
template<class F> xc_status queue_request(xc_worker *w, xc_request **out, F operation) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!w) return XC_INVALID_ARGUMENT;
  try {
    using T = typename FutureValue<decltype(operation(w->value))>::Type;
    // Allocate the observer BEFORE accepting an operation that may mutate state.
    auto request = std::make_unique<Request<T>>();
    request->future = operation(w->value);
    *out = request.release(); return XC_OK;
  } catch (...) { return failure(); }
}
const xiphos::core::Module *catalog_at(const xc_catalog *c, size_t i) {
  return c && i < c->value.size() ? &c->value[i] : nullptr;
}
const std::vector<xiphos::core::ReferenceChoice> *choices_at(const xc_choices *c, xc_choice_kind kind) {
  if (!c) return nullptr;
  switch (kind) {
  case XC_BOOK: return &c->value.books;
  case XC_CHAPTER: return &c->value.chapters;
  case XC_VERSE: return &c->value.verses;
  default: return nullptr;
  }
}
const xiphos::core::Module *module_at(const xc_reader *r, size_t i) {
  return r && i < r->value->modules().size() ? &r->value->modules()[i] : nullptr;
}
}
namespace {
const xiphos::core::BookInlineImages *inline_images(const xc_book_document *document) {
  return document && document->presentation && document->presentation->inline_images
    ? &*document->presentation->inline_images : nullptr;
}
}

namespace {
struct CopiedSearch {xiphos::core::SearchQuery query;xiphos::core::SearchLimits limits;};
CopiedSearch copy_search_inputs(const char *module,const char *expression,const xc_search_options *options,
  const char *range_module,const char *range_expression,bool within_last,const xc_search_text_options *text){
  xc_search_options copy;xc_search_options_default(&copy);if(options)copy=*options;
  if(!module||!expression||copy.mode<XC_SEARCH_REGEX||copy.mode>XC_SEARCH_INDEXED||
     (copy.case_sensitive!=0&&copy.case_sensitive!=1)||!copy.max_results||copy.max_results>xiphos::core::maximum_search_results||
     !copy.max_reference_bytes||copy.max_reference_bytes>xiphos::core::maximum_search_reference_bytes)
    throw std::invalid_argument("Invalid search inputs");
  if(text&&((text->strongs!=0&&text->strongs!=1)||(text->morphology!=0&&text->morphology!=1)||
            (text->footnotes!=0&&text->footnotes!=1)))throw std::invalid_argument("Invalid search text flags");
  xiphos::core::SearchQuery query{module,expression,static_cast<xiphos::core::SearchMode>(copy.mode),copy.case_sensitive!=0};
  query.within_last_results=within_last;
  if(text)query.text_options=xiphos::core::SearchTextOptions(text->strongs!=0,text->morphology!=0,text->footnotes!=0);
  if(range_module){if(!range_expression)throw std::invalid_argument("Missing search range");query.range=xiphos::core::SearchRange{range_module,range_expression};}
  return {std::move(query),{copy.max_results,copy.max_reference_bytes}};
}
bool valid_search_scope(const xc_search_query *query){
  return query&&(query->scope==XC_SEARCH_SCOPE_ALL||query->scope==XC_SEARCH_SCOPE_BOUNDS||query->scope==XC_SEARCH_SCOPE_LAST)&&
     (query->scope==XC_SEARCH_SCOPE_BOUNDS?(query->range_module&&query->range_expression):(!query->range_module&&!query->range_expression));
}
xc_status queue_search_module(xc_worker *w,uint64_t tab,const char *module,const char *expression,
                                  const xc_search_options *options,xc_request **out,const char *range_module,const char *range_expression,bool within_last=false,const xc_search_text_options *text=nullptr) {
  if(!out)return XC_INVALID_ARGUMENT;*out=nullptr;if(!w)return XC_INVALID_ARGUMENT;
  try{
    auto copied=copy_search_inputs(module,expression,options,range_module,range_expression,within_last,text);
    // Allocate observer before accepting the job. The worker owns SDK/control lifetime.
    auto request=std::make_unique<Request<xiphos::core::SearchResult>>();
    auto task=w->value.search_module_task(tab,std::move(copied.query),copied.limits);
    request->search_control=std::move(task.control);request->future=std::move(task.result);
    *out=request.release();return XC_OK;
  }catch(...){return failure();}
}
xc_status batch_failure(xiphos::core::SearchBatchFailure failure){
  switch(failure){
    case xiphos::core::SearchBatchFailure::none:return XC_OK;
    case xiphos::core::SearchBatchFailure::module_not_found:return XC_MODULE_NOT_FOUND;
    case xiphos::core::SearchBatchFailure::invalid_request:return XC_INVALID_ARGUMENT;
    case xiphos::core::SearchBatchFailure::engine_error:return XC_ENGINE_ERROR;
  }
  return XC_ENGINE_ERROR;
}
}
namespace {
xc_status copy_search_text_info(const std::optional<xiphos::core::SearchTextOptions> &value,xc_search_text_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};
  if(value)*out={1,value->strongs?1:0,value->morphology?1:0,value->footnotes?1:0};return XC_OK;
}
xc_status copy_search_generation_info(const xiphos::core::SearchGenerationInfo &value,xc_search_generation_info *out){
  if(!out)return XC_INVALID_ARGUMENT;
  *out={value.id,static_cast<int>(value.retention),value.requested_count,value.group_count,value.failed_groups,value.visible_results,
    value.matched_elements,value.retained_elements,value.accounted_key_bytes,value.cancelled?1:0};return XC_OK;
}

}
extern "C" {
xc_status xc_worker_restore_book_profile(xc_worker *w, uint64_t tab, const char *path, int mode, xc_request **out) {
  if (!path || !*path || (mode != XC_BOOK_RESTORE_DISPLAY_KEY && mode != XC_BOOK_RESTORE_INITIAL_WINDOW)) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) {
    return worker.restore_book_profile(tab, path,
      mode == XC_BOOK_RESTORE_DISPLAY_KEY ? xiphos::core::BookRestoreMode::display_key : xiphos::core::BookRestoreMode::initial_window);
  });
}
xc_status xc_worker_select_book(xc_worker *w, uint64_t tab, const char *id, uint64_t offset, xc_request **out) {
  if (!id || !*id) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.select_book(tab, {id, offset}); });
}
xc_status xc_worker_navigate_book(xc_worker *w, uint64_t tab, int direction, xc_request **out) {
  const auto target = copy_book_navigation(direction);
  if (!target) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.navigate_book(tab, *target); });
}
xc_status xc_worker_restore_book_selection(xc_worker *w, uint64_t tab, const xc_saved_book_selection *saved,
                                          int mode, xc_request **out) {
  if (!saved || (mode != XC_BOOK_RESTORE_DISPLAY_KEY && mode != XC_BOOK_RESTORE_INITIAL_WINDOW)) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) {
    return worker.restore_book_selection(tab, copy_book_selection(*saved),
      mode == XC_BOOK_RESTORE_DISPLAY_KEY ? xiphos::core::BookRestoreMode::display_key : xiphos::core::BookRestoreMode::initial_window);
  });
}
xc_status xc_worker_book_selection(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.book_selection(tab); });
}
xc_status xc_worker_resolve_saved_book_selection(xc_worker *w, const xc_saved_book_selection *saved,
                                                int mode, xc_request **out) {
  if (!saved || (mode != XC_BOOK_RESTORE_DISPLAY_KEY && mode != XC_BOOK_RESTORE_INITIAL_WINDOW)) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) {
    return worker.resolve_saved_book_selection(copy_book_selection(*saved),
      mode == XC_BOOK_RESTORE_DISPLAY_KEY ? xiphos::core::BookRestoreMode::display_key : xiphos::core::BookRestoreMode::initial_window);
  });
}
int xc_request_book_selection_present(const xc_request *r) {
  return r && r->status == XC_OK && r->book_selection_result ? (r->book_selection_present ? 1 : 0) : -1;
}
xc_status xc_worker_book_note_link_at(xc_worker *w, uint64_t tab, const char *id, uint64_t offset,
                                      const char *uri, int render_body, xc_request **out) {
  if (!id || !*id || !uri || !*uri || (render_body != 0 && render_body != 1)) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) { return worker.book_note_link_at(tab, id, offset, uri, render_body != 0); });
}
xc_status xc_worker_book_note(xc_worker *w, uint64_t tab, const char *id, uint64_t offset,
                              const char *identifier, int render_body, xc_request **out) {
  if (!id || !*id || !identifier || !*identifier || (render_body != 0 && render_body != 1)) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) { return worker.book_note(tab, id, offset, identifier, render_body != 0); });
}
xc_book_note *xc_request_take_book_note(xc_request *r) { return r && r->status == XC_OK ? r->book_note.release() : nullptr; }
const xc_book_tree *xc_book_note_tree(const xc_book_note *note) { return note ? &note->tree : nullptr; }
const xc_note *xc_book_note_content(const xc_book_note *note) { return note ? &note->content : nullptr; }
void xc_book_note_release(xc_book_note *note) { delete note; }
xc_status xc_worker_read_book_document(xc_worker *w, uint64_t tab, const char *id, uint64_t offset, xc_request **out) {
  if (!id || !*id) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.read_book_document(tab, id, offset); });
}
xc_status xc_worker_select_book_document(xc_worker *w, uint64_t tab, const char *id, uint64_t offset, xc_request **out) {
  if (!id || !*id) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.select_book_document(tab, {id, offset}); });
}
xc_status xc_worker_refresh_book_document(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.refresh_book_document(tab); });
}
xc_status xc_worker_navigate_book_document(xc_worker *w, uint64_t tab, int direction, xc_request **out) {
  const auto target = copy_book_navigation(direction);
  if (!target) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.navigate_book_document(tab, *target); });
}
xc_status xc_worker_restore_book_document_selection(xc_worker *w, uint64_t tab, const xc_saved_book_selection *saved, int mode, xc_request **out) {
  if (!saved || (mode != XC_BOOK_RESTORE_DISPLAY_KEY && mode != XC_BOOK_RESTORE_INITIAL_WINDOW)) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) {
    return worker.restore_book_document_selection(tab, copy_book_selection(*saved),
      mode == XC_BOOK_RESTORE_DISPLAY_KEY ? xiphos::core::BookRestoreMode::display_key : xiphos::core::BookRestoreMode::initial_window);
  });
}
xc_status xc_worker_restore_book_document_profile(xc_worker *w, uint64_t tab, const char *path, int mode, xc_request **out) {
  if (!path || !*path || (mode != XC_BOOK_RESTORE_DISPLAY_KEY && mode != XC_BOOK_RESTORE_INITIAL_WINDOW)) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) {
    return worker.restore_book_document_profile(tab, path,
      mode == XC_BOOK_RESTORE_DISPLAY_KEY ? xiphos::core::BookRestoreMode::display_key : xiphos::core::BookRestoreMode::initial_window);
  });
}
xc_status xc_worker_book_document_selection(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.book_document_selection(tab); });
}
xc_status xc_worker_book_document_note_link(xc_worker *w, uint64_t tab, const char *uri, int render_body, xc_request **out) {
  if (!uri || (render_body != 0 && render_body != 1)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.book_document_note_link(tab, uri, render_body != 0); });
}
xc_book_document *xc_request_take_book_document(xc_request *r) { return r && r->status == XC_OK ? r->book_document.release() : nullptr; }
const xc_book_tree *xc_book_document_tree(const xc_book_document *document) { return document ? &document->tree : nullptr; }
const char *xc_book_document_html(const xc_book_document *document) { return document ? document->html.c_str() : nullptr; }
uint32_t xc_book_document_display_level(const xc_book_document *document) { return document ? document->display_level : 0; }
size_t xc_book_document_count(const xc_book_document *document) { return document ? document->sections.size() : 0; }
xc_status xc_book_document_get_section(const xc_book_document *document, size_t index, xc_book_document_section *out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = {};
  if (!document || index >= document->sections.size()) return XC_INVALID_ARGUMENT;
  const auto &section = document->sections[index];
  *out = {&section.entry, section.depth, section.rendered ? 1 : 0};
  return XC_OK;
}
const char *xc_book_document_note_source_label(const xc_book_document *document) {
  return document && document->note_source ? document->note_source->reported_label.c_str() : nullptr;
}
xc_status xc_book_document_note_source_offset(const xc_book_document *document, uint64_t *out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = 0;
  if (!document || !document->note_source || !document->note_source->last_rendered_offset) return XC_INVALID_ARGUMENT;
  *out = *document->note_source->last_rendered_offset; return XC_OK;
}
int xc_book_document_note_numbers(const xc_book_document *document) {
  return document && document->presentation && document->presentation->note_numbers
    ? (*document->presentation->note_numbers ? 1 : 0) : -1;
}
const char *xc_book_document_presentation_html(const xc_book_document *document) {
  return document && document->presentation ? document->presentation->html.c_str() : nullptr;
}
xc_status xc_book_document_lexical_layout(const xc_book_document *document, xc_lexical_presentation *out) {
  return borrow_lexical_presentation(document && document->presentation ? &document->presentation->lexical : nullptr, out);
}
xc_status xc_book_document_get_display_options(const xc_book_document *document, xc_book_document_display_options *out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = {-1, -1, 0};
  if (!document || !document->presentation) return XC_INVALID_ARGUMENT;
  const auto &value = *document->presentation;
  *out = {static_cast<int>(value.image_content), static_cast<int>(value.respect_font_faces), value.double_space ? 1 : 0};
  return XC_OK;
}
size_t xc_book_document_cleanup_count(const xc_book_document *document) {
  return document && document->presentation ? document->presentation->cleanup_diagnostics.size() : 0;
}
int xc_book_document_cleanup_kind(const xc_book_document *document, size_t index) {
  if (!document || !document->presentation || index >= document->presentation->cleanup_diagnostics.size()) return -1;
  return document->presentation->cleanup_diagnostics[index].kind == xiphos::core::ContentCleanupKind::image ? 0 : 1;
}
const char *xc_book_document_cleanup_suffix(const xc_book_document *document, size_t index) {
  return document && document->presentation && index < document->presentation->cleanup_diagnostics.size()
    ? document->presentation->cleanup_diagnostics[index].suffix.c_str() : nullptr;
}
const char *xc_book_document_inline_html(const xc_book_document *document) {
  const auto *value = inline_images(document); return value ? value->html.c_str() : nullptr;
}
size_t xc_book_document_inline_image_count(const xc_book_document *document) {
  const auto *value = inline_images(document); return value ? value->embedded : 0;
}
size_t xc_book_document_inline_issue_count(const xc_book_document *document) {
  const auto *value = inline_images(document); return value ? value->issues.size() : 0;
}
const char *xc_book_document_inline_issue_source(const xc_book_document *document, size_t index) {
  const auto *value = inline_images(document); return value && index < value->issues.size() ? value->issues[index].source.c_str() : nullptr;
}
const char *xc_book_document_inline_issue_message(const xc_book_document *document, size_t index) {
  const auto *value = inline_images(document); return value && index < value->issues.size() ? value->issues[index].message.c_str() : nullptr;
}
const char *xc_book_document_stylesheet_css(const xc_book_document *document) {
  return document && document->presentation && document->presentation->stylesheet
    ? document->presentation->stylesheet->css.c_str() : nullptr;
}
const char *xc_book_document_stylesheet_inline_css(const xc_book_document *document) {
  return document && document->presentation && document->presentation->stylesheet
    ? document->presentation->stylesheet->inline_css.c_str() : nullptr;
}
const char *xc_book_document_stylesheet_filename(const xc_book_document *document) {
  return document && document->presentation && document->presentation->stylesheet
    ? document->presentation->stylesheet->filename.c_str() : nullptr;
}
const char *xc_book_document_stylesheet_issue(const xc_book_document *document) {
  return document && document->presentation && document->presentation->stylesheet_issue
    ? document->presentation->stylesheet_issue->c_str() : nullptr;
}
void xc_book_document_release(xc_book_document *document) { delete document; }
xc_status xc_worker_book_image_link(xc_worker *w, uint64_t tab, const char *uri, xc_request **out) {
  if (!uri) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.book_image_link(tab, uri); });
}
xc_book_image *xc_request_take_book_image(xc_request *r) { return r && r->status == XC_OK ? r->book_image.release() : nullptr; }
const char *xc_book_image_module(const xc_book_image *image) { return image ? image->value.context.module.c_str() : nullptr; }
const char *xc_book_image_filename(const xc_book_image *image) { return image ? image->value.command.filename.c_str() : nullptr; }
const char *xc_book_image_media_type(const xc_book_image *image) { return image ? image->value.command.media_type.c_str() : nullptr; }
const char *xc_book_image_declared_module(const xc_book_image *image) {
  return image && image->value.command.declared_module ? image->value.command.declared_module->c_str() : nullptr;
}
const char *xc_book_image_resource_field(const xc_book_image *image, int field) {
  if (!image) return nullptr;
  const auto &context = image->value.context;
  const std::optional<std::string> *value;
  switch (field) {
    case XC_BOOK_RESOURCE_DATA_PATH: value = &context.data_path; break;
    case XC_BOOK_RESOURCE_ABSOLUTE_DATA_PATH: value = &context.absolute_data_path; break;
    case XC_BOOK_RESOURCE_PREFIX_PATH: value = &context.prefix_path; break;
    case XC_BOOK_RESOURCE_PREFERRED_CSS_XHTML: value = &context.preferred_css_xhtml; break;
    default: return nullptr;
  }
  return *value ? (*value)->c_str() : nullptr;
}
void xc_book_image_release(xc_book_image *image) { delete image; }
xc_status xc_worker_follow_book_link(xc_worker *w, uint64_t tab, const char *uri, xc_request **out) {
  if (!uri) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.follow_book_link(tab, uri); });
}
xc_status xc_worker_read_book_image_link(xc_worker *w, uint64_t tab, const char *uri, xc_request **out) {
  if (!uri) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.read_book_image_link(tab, uri); });
}
xc_book_image_data *xc_request_take_book_image_data(xc_request *r) { return r && r->status == XC_OK ? r->book_image_data.release() : nullptr; }
const xc_book_image *xc_book_image_data_source(const xc_book_image_data *image) { return image ? &image->source : nullptr; }
const unsigned char *xc_book_image_data_bytes(const xc_book_image_data *image) {
  return image && !image->bytes.empty() ? reinterpret_cast<const unsigned char *>(image->bytes.data()) : nullptr;
}
size_t xc_book_image_data_size(const xc_book_image_data *image) { return image ? image->bytes.size() : 0; }
void xc_book_image_data_release(xc_book_image_data *image) { delete image; }
xc_status xc_worker_read_verse_inline_image(xc_worker *w,const char *module,const char *source,size_t maximum_bytes,xc_request **out){
  if(!bounded_image_c_text(module,4096)||!bounded_image_c_text(source,32768)||!maximum_bytes||maximum_bytes>xiphos::core::maximum_verse_image_bytes){if(out)*out=nullptr;return XC_INVALID_ARGUMENT;}
  return queue_request(w,out,[=](auto &worker){return worker.read_verse_inline_image(module,source,maximum_bytes);});
}
xc_verse_image *xc_request_take_verse_image(xc_request *r){return r&&r->status==XC_OK?r->verse_image.release():nullptr;}
const char *xc_verse_image_module(const xc_verse_image *image){return image?image->value.module.c_str():nullptr;}
const char *xc_verse_image_source(const xc_verse_image *image){return image?image->value.source.c_str():nullptr;}
const char *xc_verse_image_media_type(const xc_verse_image *image){return image?image->value.media_type.c_str():nullptr;}
const unsigned char *xc_verse_image_bytes(const xc_verse_image *image){return image&&!image->value.data.empty()?reinterpret_cast<const unsigned char *>(image->value.data.data()):nullptr;}
size_t xc_verse_image_size(const xc_verse_image *image){return image?image->value.data.size():0;}
void xc_verse_image_release(xc_verse_image *image){delete image;}
xc_status xc_worker_book_children(xc_worker *w, const char *id, uint64_t parent,
                                  int has_after, uint64_t after, uint32_t limit, xc_request **out) {
  if (!id || !*id || (has_after != 0 && has_after != 1) || !limit || limit > 256) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  const auto cursor = has_after ? std::optional<std::uint64_t>(after) : std::nullopt;
  return queue_request(w, out, [=](auto &worker) { return worker.book_children(id, parent, cursor, limit); });
}
xc_book_children *xc_request_take_book_children(xc_request *r) { return r && r->status == XC_OK ? r->book_children.release() : nullptr; }
const char *xc_book_children_module(const xc_book_children *page) { return page ? page->value.module.c_str() : nullptr; }
size_t xc_book_children_count(const xc_book_children *page) { return page ? page->value.children.size() : 0; }
xc_status xc_book_children_parent(const xc_book_children *page, xc_book_node *out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = {};
  if (!page) return XC_INVALID_ARGUMENT;
  const auto &node = page->value.parent;
  *out = {node.offset, node.path.c_str(), node.title.c_str(), 1, node.has_children ? 1 : 0};
  return XC_OK;
}
xc_status xc_book_children_get_node(const xc_book_children *page, size_t index, xc_book_node *out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = {};
  if (!page || index >= page->value.children.size()) return XC_INVALID_ARGUMENT;
  const auto &node = page->value.children[index];
  *out = {node.offset, node.path.c_str(), node.title.c_str(), 1, node.has_children ? 1 : 0};
  return XC_OK;
}
int xc_book_children_next_after(const xc_book_children *page, uint64_t *out) {
  if (!out) return 0;
  *out = 0;
  if (!page || !page->value.next_after) return 0;
  *out = *page->value.next_after;
  return 1;
}
void xc_book_children_release(xc_book_children *page) { delete page; }
xc_status xc_worker_book_tree(xc_worker *w, const char *id, uint64_t offset, xc_request **out) {
  if (!id || !*id) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.book_tree(id, offset); });
}
xc_status xc_worker_book_tree_path(xc_worker *w, const char *id, const char *path, xc_request **out) {
  if (!id || !*id || !path) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.book_tree_path(id, path); });
}
xc_status xc_worker_read_book(xc_worker *w, uint64_t tab, const char *id, uint64_t offset, xc_request **out) {
  if (!id || !*id) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.read_book(tab, id, offset); });
}
xc_book_tree *xc_request_take_book_tree(xc_request *r) { return r && r->status == XC_OK ? r->book_tree.release() : nullptr; }
xc_book_entry *xc_request_take_book_entry(xc_request *r) { return r && r->status == XC_OK ? r->book_entry.release() : nullptr; }
xc_status xc_book_tree_get_node(const xc_book_tree *tree, int kind, xc_book_node *out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = {};
  if (!tree) return XC_INVALID_ARGUMENT;
  const xiphos::core::BookTreeNode *node = nullptr;
  const auto &value = tree->value;
  switch (kind) {
    case XC_BOOK_CURRENT: node = &value.node; break;
    case XC_BOOK_PARENT: if (value.parent) node = &*value.parent; break;
    case XC_BOOK_FIRST_CHILD: if (value.first_child) node = &*value.first_child; break;
    case XC_BOOK_PREVIOUS_SIBLING: if (value.previous_sibling) node = &*value.previous_sibling; break;
    case XC_BOOK_NEXT_SIBLING: if (value.next_sibling) node = &*value.next_sibling; break;
    default: return XC_INVALID_ARGUMENT;
  }
  if (node) *out = {node->offset, node->path.c_str(), node->title.c_str(), 1, node->has_children ? 1 : 0};
  return XC_OK;
}
const char *xc_book_tree_module(const xc_book_tree *tree) { return tree ? tree->value.module.c_str() : nullptr; }
void xc_book_tree_release(xc_book_tree *tree) { delete tree; }
const xc_book_tree *xc_book_entry_tree(const xc_book_entry *entry) { return entry ? &entry->tree : nullptr; }
const char *xc_book_entry_html(const xc_book_entry *entry) { return entry ? entry->html.c_str() : nullptr; }
int xc_book_entry_has_entry(const xc_book_entry *entry) { return entry && entry->has_entry; }
void xc_book_entry_release(xc_book_entry *entry) { delete entry; }
xc_status xc_worker_open(const char *root, xc_worker **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  try { *out = new xc_worker(root); return XC_OK; } catch (...) { return failure(); }
}
void xc_worker_close(xc_worker *w) { delete w; }
xc_status xc_worker_modules(xc_worker *w, xc_request **out) {
  return queue_request(w, out, [](auto &worker) { return worker.modules(); });
}
void xc_search_options_default(xc_search_options *out) {
  if(out){const xiphos::core::SearchLimits limits;*out={XC_SEARCH_PHRASE,0,limits.max_results,limits.max_reference_bytes};}
}

void xc_search_text_options_default(xc_search_text_options *out){if(out)*out={0,0,0};}
void xc_search_query_default(xc_search_query *out){if(out)*out={nullptr,nullptr,XC_SEARCH_SCOPE_ALL,nullptr,nullptr,nullptr,nullptr};}
xc_status xc_worker_search(xc_worker *w,uint64_t tab,const xc_search_query *query,xc_request **out){
  if(out)*out=nullptr;
  if(!valid_search_scope(query))return XC_INVALID_ARGUMENT;
  return queue_search_module(w,tab,query->module,query->expression,query->options,out,query->range_module,
    query->range_expression,query->scope==XC_SEARCH_SCOPE_LAST,query->text_options);
}
void xc_search_batch_limits_default(xc_search_batch_limits *out){
  if(out)*out={xiphos::core::maximum_search_results,xiphos::core::maximum_search_reference_bytes};
}
xc_status xc_worker_search_modules(xc_worker *w,uint64_t tab,const char *const *modules,size_t count,
    const xc_search_query *query,const xc_search_batch_limits *total,xc_request **out){
  if(!out)return XC_INVALID_ARGUMENT;*out=nullptr;
  if(!w||!modules||!count||count>xiphos::core::maximum_search_batch_modules||!valid_search_scope(query)||query->module)
    return XC_INVALID_ARGUMENT;
  xc_search_batch_limits budget;xc_search_batch_limits_default(&budget);if(total)budget=*total;
  if(!budget.max_results||budget.max_results>xiphos::core::maximum_search_results||!budget.max_reference_bytes||
     budget.max_reference_bytes>xiphos::core::maximum_search_reference_bytes)return XC_INVALID_ARGUMENT;
  try{
    auto copied=copy_search_inputs(modules[0],query->expression,query->options,query->range_module,query->range_expression,
      query->scope==XC_SEARCH_SCOPE_LAST,query->text_options);
    std::vector<xiphos::core::SearchQuery> queries;queries.reserve(count);
    for(size_t i=0;i<count;++i){
      if(!modules[i])throw std::invalid_argument("NULL batch module");
      auto item=copied.query;item.module=modules[i];queries.push_back(std::move(item));
    }
    auto request=std::make_unique<Request<xiphos::core::SearchBatchResult>>();
    auto task=w->value.search_batch_task(tab,std::move(queries),{copied.limits,{budget.max_results,budget.max_reference_bytes}});
    request->search_control=std::move(task.control);request->future=std::move(task.result);
    *out=request.release();return XC_OK;
  }catch(...){return failure();}
}
xc_search_batch *xc_request_take_search_batch(xc_request *r){return r&&r->status==XC_OK?r->search_batch.release():nullptr;}
xc_status xc_search_batch_get_info(const xc_search_batch *batch,xc_search_batch_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!batch)return XC_INVALID_ARGUMENT;
  const auto &value=batch->value;
  *out={value.requested.size(),value.groups.size(),value.copied_results,value.copied_reference_bytes,value.failures,
    value.limited?1:0,value.cancelled?1:0,value.scope_updated?1:0,value.text_options_updated?1:0};return XC_OK;
}
xc_status xc_search_batch_get_group(const xc_search_batch *batch,size_t index,xc_search_batch_group_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!batch||index>=batch->value.groups.size())return XC_INVALID_ARGUMENT;
  const auto &group=batch->value.groups[index];
  *out={group.request_index,batch_failure(group.failure),batch->results[index]?1:0};return XC_OK;
}
const xc_search *xc_search_batch_group_result(const xc_search_batch *batch,size_t index){
  return batch&&index<batch->results.size()?batch->results[index].get():nullptr;
}
const char *xc_search_batch_requested_module(const xc_search_batch *batch,size_t index){
  return batch&&index<batch->value.requested.size()?batch->value.requested[index].module.c_str():nullptr;
}
xc_status xc_search_batch_get_scope(const xc_search_batch *batch,xc_search_scope_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!batch)return XC_INVALID_ARGUMENT;
  *out={batch->value.scope.present?1:0,batch->value.scope.element_count};return XC_OK;
}
xc_status xc_search_batch_get_text_options(const xc_search_batch *batch,xc_search_text_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!batch)return XC_INVALID_ARGUMENT;
  return copy_search_text_info(batch->value.text_options.options,out);
}
void xc_search_batch_release(xc_search_batch *batch){delete batch;}
void xc_search_html_export_request_default(xc_search_html_export_request *out){
  if(out)*out={0,0,nullptr,0,nullptr,0,1024*1024,xiphos::core::maximum_search_export_rows,0};
}
void xc_search_html_image_options_default(xc_search_html_image_options *out){
  if(out)*out={XC_HTML_IMAGES_REJECT,xiphos::core::maximum_search_export_images,
    xiphos::core::maximum_search_export_bytes,xiphos::core::maximum_search_export_image_pixels};
}
xc_status xc_worker_export_search_html(xc_worker *w,uint64_t tab,const xc_search_html_export_request *input,xc_request **out){
  xc_search_html_image_options images;xc_search_html_image_options_default(&images);
  return xc_worker_export_search_html_with_images(w,tab,input,&images,out);
}
xc_status xc_worker_export_search_html_with_images(xc_worker *w,uint64_t tab,const xc_search_html_export_request *input,
    const xc_search_html_image_options *images,xc_request **out){
  return xc_worker_export_search_rich_html(w,tab,input,images,0,out);
}
xc_status xc_worker_export_search_rich_html(xc_worker *w,uint64_t tab,const xc_search_html_export_request *input,
    const xc_search_html_image_options *images,size_t plain_text_max_bytes,xc_request **out){
  return xc_worker_export_search_rich_html_with_context(w,tab,input,images,plain_text_max_bytes,0,out);
}
xc_status xc_worker_export_search_rich_html_with_context(xc_worker *w,uint64_t tab,const xc_search_html_export_request *input,
    const xc_search_html_image_options *images,size_t plain_text_max_bytes,int include_search_context,xc_request **out){
  if(!out)return XC_INVALID_ARGUMENT;*out=nullptr;
  if(include_search_context!=0&&include_search_context!=1)return XC_INVALID_ARGUMENT;
  if(plain_text_max_bytes>xiphos::core::maximum_search_export_bytes)return XC_INVALID_ARGUMENT;
  if(!images||(images->policy!=XC_HTML_IMAGES_REJECT&&images->policy!=XC_HTML_IMAGES_EMBEDDED_STATIC_PNG)||
     !images->max_images||images->max_images>xiphos::core::maximum_search_export_images||
     !images->max_original_bytes||images->max_original_bytes>xiphos::core::maximum_search_export_bytes||
     !images->max_pixels||images->max_pixels>xiphos::core::maximum_search_export_image_pixels)return XC_INVALID_ARGUMENT;
  if(!w||!input||(input->selection!=0&&input->selection!=1)||
     (input->selection==0&&(!input->rows||!input->row_count||input->row_count>xiphos::core::maximum_search_export_rows))||
     (input->selection==1&&(input->rows||input->row_count))||
     (input->include_scripture!=0&&input->include_scripture!=1)||(input->allow_partial_source!=0&&input->allow_partial_source!=1)||
     !input->max_bytes||input->max_bytes>xiphos::core::maximum_search_export_bytes||!input->max_rows||input->max_rows>xiphos::core::maximum_search_export_rows)return XC_INVALID_ARGUMENT;
  try{
    xiphos::core::SearchHtmlExportRequest copied;
    copied.include_search_context=include_search_context!=0;
    if(plain_text_max_bytes)copied.plain_text_max_bytes=plain_text_max_bytes;
    copied.images={static_cast<xiphos::core::SearchHtmlImagePolicy>(images->policy),images->max_images,images->max_original_bytes,images->max_pixels};
    copied.generation=input->generation;copied.selection=static_cast<xiphos::core::SearchExportSelection>(input->selection);
    copied.include_scripture=input->include_scripture!=0;copied.max_bytes=input->max_bytes;copied.max_rows=input->max_rows;copied.allow_partial_source=input->allow_partial_source!=0;
    if(input->title){std::size_t size=0;while(size<=32768&&input->title[size])++size;if(size>32768)return XC_INVALID_ARGUMENT;copied.title.assign(input->title,size);}
    copied.rows.reserve(input->row_count);for(std::size_t i=0;i<input->row_count;++i)copied.rows.push_back({input->rows[i].group,input->rows[i].row});
    auto request=std::make_unique<Request<xiphos::core::SearchHtmlExport>>();
    auto task=w->value.export_search_html_task(tab,std::move(copied));
    request->search_control=std::move(task.control);request->future=std::move(task.result);*out=request.release();return XC_OK;
  }catch(...){return failure();}
}
xc_search_html_export *xc_request_take_search_html_export(xc_request *r){return r&&r->status==XC_OK?r->search_html_export.release():nullptr;}
xc_status xc_search_html_export_get_info(const xc_search_html_export *r,xc_search_html_export_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!r)return XC_INVALID_ARGUMENT;
  *out={static_cast<int>(r->value.status),r->value.row_count,r->value.failed_row?1:0,r->value.failed_row.value_or(0),r->value.includes_scripture?1:0,static_cast<int>(r->value.selection),static_cast<int>(r->value.issue)};return XC_OK;
}
xc_status xc_search_html_export_get_generation(const xc_search_html_export *r,xc_search_generation_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!r)return XC_INVALID_ARGUMENT;return copy_search_generation_info(r->value.generation,out);
}
xc_status xc_search_html_export_get_images(const xc_search_html_export *r,xc_search_html_image_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!r)return XC_INVALID_ARGUMENT;
  const auto &images=r->value.images;*out={images.count,images.unique_count,images.original_bytes,images.pixels};return XC_OK;
}
const char *xc_search_html_export_html(const xc_search_html_export *r){return r?r->value.html.c_str():nullptr;}
size_t xc_search_html_export_size(const xc_search_html_export *r){return r?r->value.html.size():0;}
const char *xc_search_html_export_plain_text(const xc_search_html_export *r){
  return r&&r->value.plain_text?r->value.plain_text->c_str():nullptr;
}
size_t xc_search_html_export_plain_text_size(const xc_search_html_export *r){
  return r&&r->value.plain_text?r->value.plain_text->size():0;
}
void xc_search_html_export_release(xc_search_html_export *r){delete r;}
void xc_search_text_export_request_default(xc_search_text_export_request *out){
  if(out)*out={0,nullptr,0,nullptr,0,1024*1024};
}
xc_status xc_worker_export_search_visible_text(xc_worker *w,uint64_t tab,const xc_search_text_export_request *input,xc_request **out){
  return xc_worker_export_search_visible_text_with_context(w,tab,input,0,out);
}
xc_status xc_worker_export_search_visible_text_with_context(xc_worker *w,uint64_t tab,const xc_search_text_export_request *input,int include_search_context,xc_request **out){
  if(!out)return XC_INVALID_ARGUMENT;*out=nullptr;
  if(include_search_context!=0&&include_search_context!=1)return XC_INVALID_ARGUMENT;
  if(!w||!input||!input->rows||!input->row_count||input->row_count>xiphos::core::maximum_search_export_rows||
     (input->include_scripture!=0&&input->include_scripture!=1)||!input->max_bytes||input->max_bytes>xiphos::core::maximum_search_export_bytes)
    return XC_INVALID_ARGUMENT;
  try{
    xiphos::core::SearchTextExportRequest copied;
    copied.include_search_context=include_search_context!=0;
    copied.generation=input->generation;copied.include_scripture=input->include_scripture!=0;copied.max_bytes=input->max_bytes;
    if(input->title){
      std::size_t size=0;while(size<=32768&&input->title[size])++size;
      if(size>32768)return XC_INVALID_ARGUMENT;
      copied.title.assign(input->title,size);
    }
    copied.rows.reserve(input->row_count);
    for(std::size_t i=0;i<input->row_count;++i)copied.rows.push_back({input->rows[i].group,input->rows[i].row});
    auto request=std::make_unique<Request<xiphos::core::SearchTextExport>>();
    auto task=w->value.export_search_visible_text_task(tab,std::move(copied));
    request->search_control=std::move(task.control);request->future=std::move(task.result);
    *out=request.release();return XC_OK;
  }catch(...){return failure();}
}
void xc_search_all_text_export_request_default(xc_search_all_text_export_request *out){
  if(out)*out={0,nullptr,0,1024*1024,xiphos::core::maximum_search_export_rows,0};
}
xc_status xc_worker_export_search_all_text(xc_worker *w,uint64_t tab,const xc_search_all_text_export_request *input,xc_request **out){
  return xc_worker_export_search_all_text_with_context(w,tab,input,0,out);
}
xc_status xc_worker_export_search_all_text_with_context(xc_worker *w,uint64_t tab,const xc_search_all_text_export_request *input,int include_search_context,xc_request **out){
  if(!out)return XC_INVALID_ARGUMENT;*out=nullptr;
  if(include_search_context!=0&&include_search_context!=1)return XC_INVALID_ARGUMENT;
  if(!w||!input||(input->include_scripture!=0&&input->include_scripture!=1)||
     (input->allow_partial_source!=0&&input->allow_partial_source!=1)||!input->max_bytes||
     input->max_bytes>xiphos::core::maximum_search_export_bytes||!input->max_rows||input->max_rows>xiphos::core::maximum_search_export_rows)return XC_INVALID_ARGUMENT;
  try{
    xiphos::core::SearchAllTextExportRequest copied;
    copied.include_search_context=include_search_context!=0;
    copied.generation=input->generation;copied.include_scripture=input->include_scripture!=0;
    copied.max_bytes=input->max_bytes;copied.max_rows=input->max_rows;copied.allow_partial_source=input->allow_partial_source!=0;
    if(input->title){std::size_t size=0;while(size<=32768&&input->title[size])++size;if(size>32768)return XC_INVALID_ARGUMENT;copied.title.assign(input->title,size);}
    auto request=std::make_unique<Request<xiphos::core::SearchTextExport>>();
    auto task=w->value.export_search_all_text_task(tab,std::move(copied));
    request->search_control=std::move(task.control);request->future=std::move(task.result);
    *out=request.release();return XC_OK;
  }catch(...){return failure();}
}
int xc_search_text_export_selection(const xc_search_text_export *r){return r?static_cast<int>(r->value.selection):-1;}
xc_search_text_export *xc_request_take_search_text_export(xc_request *r){return r&&r->status==XC_OK?r->search_export.release():nullptr;}
xc_status xc_search_text_export_get_info(const xc_search_text_export *r,xc_search_text_export_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!r)return XC_INVALID_ARGUMENT;
  *out={static_cast<int>(r->value.status),r->value.row_count,r->value.failed_row?1:0,r->value.failed_row.value_or(0),r->value.includes_scripture?1:0};return XC_OK;
}
xc_status xc_search_text_export_get_generation(const xc_search_text_export *r,xc_search_generation_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!r)return XC_INVALID_ARGUMENT;
  return copy_search_generation_info(r->value.generation,out);
}
const char *xc_search_text_export_text(const xc_search_text_export *r){return r?r->value.text.c_str():nullptr;}
size_t xc_search_text_export_size(const xc_search_text_export *r){return r?r->value.text.size():0;}
void xc_search_text_export_release(xc_search_text_export *r){delete r;}
void xc_search_retention_limits_default(xc_search_retention_limits *out){
  if(out)*out={xiphos::core::maximum_search_retained_elements,xiphos::core::maximum_search_retained_key_bytes};
}
void xc_search_generation_read_options_default(xc_search_generation_read_options *out){
  if(out){const xiphos::core::SearchLimits limits;*out={XC_SEARCH_REFERENCES_COMPLETE,limits.max_results,limits.max_reference_bytes};}
}
xc_status xc_worker_search_generation(xc_worker *w,uint64_t tab,xc_request **out){
  return queue_request(w,out,[=](auto &worker){return worker.search_generation(tab);});
}
xc_status xc_worker_search_retention_limits(xc_worker *w,uint64_t tab,xc_request **out){
  return queue_request(w,out,[=](auto &worker){return worker.search_retention_limits(tab);});
}
xc_status xc_worker_set_search_retention_limits(xc_worker *w,uint64_t tab,const xc_search_retention_limits *limits,xc_request **out){
  if(out)*out=nullptr;
  if(!limits||!limits->max_elements||limits->max_elements>xiphos::core::maximum_search_retained_elements||
     !limits->max_key_bytes||limits->max_key_bytes>xiphos::core::maximum_search_retained_key_bytes)return XC_INVALID_ARGUMENT;
  const xiphos::core::SearchRetentionLimits copied{limits->max_elements,limits->max_key_bytes};
  return queue_request(w,out,[=](auto &worker){return worker.set_search_retention_limits(tab,copied);});
}
xc_status xc_worker_read_search_generation(xc_worker *w,uint64_t tab,uint64_t id,size_t group,const xc_search_generation_read_options *options,xc_request **out){
  if(out)*out=nullptr;
  xc_search_generation_read_options copied;xc_search_generation_read_options_default(&copied);if(options)copied=*options;
  if((copied.source!=XC_SEARCH_REFERENCES_VISIBLE&&copied.source!=XC_SEARCH_REFERENCES_COMPLETE)||!copied.max_results||
     copied.max_results>xiphos::core::maximum_search_results||!copied.max_reference_bytes||copied.max_reference_bytes>xiphos::core::maximum_search_reference_bytes)return XC_INVALID_ARGUMENT;
  return queue_request(w,out,[=](auto &worker){return worker.read_search_generation(tab,id,group,
    static_cast<xiphos::core::SearchReferenceSource>(copied.source),{copied.max_results,copied.max_reference_bytes});});
}
xc_status xc_request_search_retention_limits(const xc_request *r,xc_search_retention_limits *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};
  if(!r||r->status!=XC_OK||!r->search_retention_limits)return XC_INVALID_ARGUMENT;
  *out={r->search_retention_limits->max_elements,r->search_retention_limits->max_key_bytes};return XC_OK;
}
xc_status xc_request_search_generation(const xc_request *r,xc_search_generation_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!r||r->status!=XC_OK||!r->search_generation)return XC_INVALID_ARGUMENT;
  return copy_search_generation_info(*r->search_generation,out);
}
xc_status xc_search_get_generation(const xc_search *r,xc_search_generation_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!r)return XC_INVALID_ARGUMENT;
  return copy_search_generation_info(r->value.generation,out);
}
xc_status xc_search_batch_get_generation(const xc_search_batch *r,xc_search_generation_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!r)return XC_INVALID_ARGUMENT;
  return copy_search_generation_info(r->value.generation,out);
}
int xc_search_generation_updated(const xc_search *r){return r?(r->value.generation_updated?1:0):-1;}
int xc_search_batch_generation_updated(const xc_search_batch *r){return r?(r->value.generation_updated?1:0):-1;}
xc_status xc_request_search_generation_read(const xc_request *r,xc_search_generation_read_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!r||r->status!=XC_OK||!r->search_generation_read)return XC_INVALID_ARGUMENT;
  const auto &value=*r->search_generation_read;
  *out={static_cast<int>(value.status),batch_failure(value.failure),value.status==xiphos::core::SearchGenerationReadStatus::ok?1:0};return XC_OK;
}
xc_status xc_request_search_generation_query(const xc_request *r,xc_search_query_metadata *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};
  if(!r||r->status!=XC_OK||!r->search_generation_read||!r->search_generation_read->requested)return XC_INVALID_ARGUMENT;
  const auto &query=*r->search_generation_read->requested;
  *out={query.module.c_str(),query.expression.c_str(),static_cast<int>(query.mode),query.case_sensitive?1:0,query.within_last_results?1:0,
    query.range?query.range->module.c_str():nullptr,query.range?query.range->expression.c_str():nullptr,{}};
  return copy_search_text_info(query.text_options,&out->text_options);
}
xc_status xc_worker_search_text_options(xc_worker *w,uint64_t tab,xc_request **out){
  return queue_request(w,out,[=](auto &worker){return worker.search_text_options(tab);});
}
xc_status xc_worker_clear_search_text_options(xc_worker *w,uint64_t tab,xc_request **out){
  return queue_request(w,out,[=](auto &worker){return worker.clear_search_text_options(tab);});
}
xc_status xc_request_search_text_options(const xc_request *r,xc_search_text_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};
  if(!r||r->status!=XC_OK||!r->search_text_state)return XC_INVALID_ARGUMENT;
  return copy_search_text_info(r->search_text_state->options,out);
}
xc_status xc_search_requested_text_options(const xc_search *r,xc_search_text_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!r)return XC_INVALID_ARGUMENT;
  return copy_search_text_info(r->value.requested.text_options,out);
}
xc_status xc_search_effective_text_options(const xc_search *r,xc_search_text_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!r)return XC_INVALID_ARGUMENT;
  return copy_search_text_info(r->value.effective_text_options,out);
}
int xc_search_index_fallback(const xc_search *r){return r?(r->value.index_fallback?1:0):-1;}
int xc_search_text_options_updated(const xc_search *r){return r?(r->value.text_options_updated?1:0):-1;}
xc_status xc_worker_search_module(xc_worker *w,uint64_t tab,const char *module,const char *expression,
    const xc_search_options *options,xc_request **out){return queue_search_module(w,tab,module,expression,options,out,nullptr,nullptr);}
xc_status xc_worker_search_module_in_range(xc_worker *w,uint64_t tab,const char *module,const char *expression,
    const char *range_module,const char *range_expression,const xc_search_options *options,xc_request **out){
  if(!range_module||!range_expression){if(out)*out=nullptr;return XC_INVALID_ARGUMENT;}
  return queue_search_module(w,tab,module,expression,options,out,range_module,range_expression);
}
xc_status xc_worker_search_module_in_last_results(xc_worker *w,uint64_t tab,const char *module,const char *expression,
    const xc_search_options *options,xc_request **out){
  return queue_search_module(w,tab,module,expression,options,out,nullptr,nullptr,true);
}
xc_status xc_worker_last_search_scope(xc_worker *w,uint64_t tab,xc_request **out){
  return queue_request(w,out,[=](auto &worker){return worker.last_search_scope(tab);});
}
xc_status xc_worker_clear_search_scope(xc_worker *w,uint64_t tab,xc_request **out){
  return queue_request(w,out,[=](auto &worker){return worker.clear_search_scope(tab);});
}
xc_status xc_request_search_scope(const xc_request *r,xc_search_scope_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};
  if(!r||r->status!=XC_OK||!r->search_scope)return XC_INVALID_ARGUMENT;
  *out={r->search_scope->present?1:0,r->search_scope->element_count};return XC_OK;
}
xc_status xc_search_get_scope(const xc_search *r,xc_search_scope_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!r)return XC_INVALID_ARGUMENT;
  *out={r->value.scope.present?1:0,r->value.scope.element_count};return XC_OK;
}
int xc_search_scope_updated(const xc_search *r){return r?(r->value.scope_updated?1:0):-1;}
int xc_search_within_last_results(const xc_search *r){return r?(r->value.requested.within_last_results?1:0):-1;}
const char *xc_search_range_module(const xc_search *r){return r&&r->value.requested.range?r->value.requested.range->module.c_str():nullptr;}
const char *xc_search_range_expression(const xc_search *r){return r&&r->value.requested.range?r->value.requested.range->expression.c_str():nullptr;}
xc_search *xc_request_take_search(xc_request *r){return r&&r->status==XC_OK?r->search_result.release():nullptr;}
xc_status xc_search_get_info(const xc_search *result,xc_search_info *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};if(!result)return XC_INVALID_ARGUMENT;
  const auto &value=result->value;
  *out={static_cast<int>(value.requested.mode),value.engine_type,value.requested.case_sensitive?1:0,
        value.limited?1:0,value.total_results,value.references.size(),value.cancelled?1:0,value.executed?1:0};return XC_OK;
}
xc_status xc_request_cancel_search(xc_request *request){
  if(!request||!request->search_control)return XC_INVALID_ARGUMENT;
  request->search_control->request_cancel();return XC_OK;
}
xc_status xc_request_search_progress(const xc_request *request,xc_search_progress *out){
  if(!out)return XC_INVALID_ARGUMENT;*out={};
  if(!request||!request->search_control)return XC_INVALID_ARGUMENT;
  const auto &control=*request->search_control;
  *out={static_cast<int>(control.phase()),control.percent(),control.cancel_requested()?1:0};return XC_OK;
}
const char *xc_search_module(const xc_search *r){return r?r->value.requested.module.c_str():nullptr;}
const char *xc_search_source_module(const xc_search *r){return r?r->value.source_module().c_str():nullptr;}
const char *xc_search_expression(const xc_search *r){return r?r->value.requested.expression.c_str():nullptr;}
const char *xc_search_effective_expression(const xc_search *r){return r?r->value.effective_expression.c_str():nullptr;}
const char *xc_search_reference(const xc_search *r,size_t index){return r&&index<r->value.references.size()?r->value.references[index].c_str():nullptr;}
void xc_search_release(xc_search *result){delete result;}
xc_status xc_worker_create_tab(xc_worker *w, xc_request **out) {
  return queue_request(w, out, [](auto &worker) { return worker.create_tab(); });
}
xc_status xc_worker_close_tab(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [tab](auto &worker) { return worker.close_tab(tab); });
}
xc_status xc_worker_visit(xc_worker *w, uint64_t tab, const char *id, const char *ref, xc_request **out) {
  if (!id || !*id || !ref || !*ref) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.visit(tab, id, ref); });
}
xc_status xc_worker_navigate(xc_worker *w, uint64_t tab, int direction, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.navigate(tab, direction); });
}
xc_status xc_worker_navigate_chapter(xc_worker *w, uint64_t tab, const char *id, const char *ref, int direction, xc_request **out) {
  if (!id || !*id || !ref || !*ref) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.navigate_chapter(tab, id, ref, direction); });
}
xc_status xc_worker_read_passage(xc_worker *w, uint64_t tab, const char *id, const char *commentary,
                                  const char *ref, xc_request **out) {
  if (!id || !*id || !commentary || !ref || !*ref) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.read_passage(tab, id, commentary, ref); });
}
xc_status xc_worker_visit_passage(xc_worker *w, uint64_t tab, const char *id, const char *commentary,
                                   const char *ref, xc_request **out) {
  if (!id || !*id || !commentary || !ref || !*ref) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.visit_passage(tab, id, commentary, ref); });
}
xc_status xc_worker_visit_linked_passage(xc_worker *w, uint64_t tab, const char *bible, const char *commentary,
                                        const char *target, const char *ref, xc_request **out) {
  if (!bible || !*bible || !commentary || !target || !*target || !ref || !*ref) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) { return worker.visit_linked_passage(tab, bible, commentary, {target, ref}); });
}
xc_status xc_worker_navigate_passage(xc_worker *w, uint64_t tab, int direction, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.navigate_passage(tab, direction); });
}
xc_status xc_worker_navigate_passage_chapter(xc_worker *w, uint64_t tab, const char *id, const char *commentary,
                                            const char *ref, int direction, xc_request **out) {
  if (!id || !*id || !commentary || !ref || !*ref) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.navigate_passage_chapter(tab, id, commentary, ref, direction); });
}
xc_status xc_worker_history(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.history(tab); });
}
xc_status xc_worker_clear_history(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.clear_history(tab); });
}
xc_status xc_worker_module_presentation_options(xc_worker *w, uint64_t tab, const char *id, xc_request **out) {
  if (!id || !*id) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.module_presentation_options(tab, id); });
}
xc_status xc_worker_set_commentary_chapter_mode(xc_worker *w, uint64_t tab, const char *id, int enabled, xc_request **out) {
  if (!id || !*id || (enabled != 0 && enabled != 1)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.set_commentary_chapter_mode(tab, id, enabled != 0); });
}
xc_status xc_worker_set_headings(xc_worker *w, uint64_t tab, const char *id, int enabled, xc_request **out) {
  if (!id || !*id || (enabled != 0 && enabled != 1)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.set_headings(tab, id, enabled != 0); });
}
xc_status xc_worker_set_lexical_layout_enabled(xc_worker *w, uint64_t tab, int enabled, xc_request **out) {
  if (enabled != 0 && enabled != 1) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.set_lexical_layout_enabled(tab, enabled != 0); });
}
xc_status xc_worker_set_note_numbering_enabled(xc_worker *w, uint64_t tab, int enabled, xc_request **out) {
  if (enabled != 0 && enabled != 1) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.set_note_numbering_enabled(tab, enabled != 0); });
}
xc_status xc_worker_set_heading_note_links_enabled(xc_worker *w, uint64_t tab, int enabled, xc_request **out) {
  if (enabled != 0 && enabled != 1) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.set_heading_note_links_enabled(tab, enabled != 0); });
}
xc_status xc_worker_set_note_numbers(xc_worker *w, uint64_t tab, const char *id, int enabled, xc_request **out) {
  if (!id || !*id || (enabled != 0 && enabled != 1)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.set_note_numbers(tab, id, enabled != 0); });
}
xc_status xc_request_note_numbers(const xc_request *r, int *enabled, int *available) {
  if (enabled) *enabled = 0;
  if (available) *available = 0;
  if (!r || !enabled || !available) return XC_INVALID_ARGUMENT;
  if (r->status != XC_OK) return r->status;
  if (!r->presentation) return XC_INVALID_ARGUMENT;
  *enabled = r->presentation->note_numbers ? 1 : 0;
  *available = r->presentation->note_numbering_available ? 1 : 0;
  return XC_OK;
}
xc_status xc_worker_set_double_space(xc_worker *w, uint64_t tab, const char *id, int enabled, xc_request **out) {
  if (!id || !*id || (enabled != 0 && enabled != 1)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.set_double_space(tab, id, enabled != 0); });
}
int xc_request_double_space(const xc_request *r) {
  return r && r->status == XC_OK && r->presentation ? (r->presentation->double_space ? 1 : 0) : -1;
}
xc_status xc_worker_set_image_content(xc_worker *w, uint64_t tab, const char *id, int enabled, xc_request **out) {
  if (!id || !*id || (enabled != 0 && enabled != 1)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.set_image_content(tab, id, enabled != 0); });
}
xc_status xc_worker_set_respect_font_faces(xc_worker *w, uint64_t tab, const char *id, int enabled, xc_request **out) {
  if (!id || !*id || (enabled != 0 && enabled != 1)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.set_respect_font_faces(tab, id, enabled != 0); });
}
xc_status xc_request_content_options(const xc_request *r, int *images, int *font_faces) {
  if (images) *images = -1;
  if (font_faces) *font_faces = -1;
  if (!r || !images || !font_faces) return XC_INVALID_ARGUMENT;
  if (r->status != XC_OK) return r->status;
  if (!r->presentation) return XC_INVALID_ARGUMENT;
  *images = static_cast<int>(r->presentation->image_content);
  *font_faces = static_cast<int>(r->presentation->respect_font_faces);
  return XC_OK;
}
int xc_request_headings(const xc_request *r) {
  return r && r->status == XC_OK && r->presentation ? (r->presentation->headings ? 1 : 0) : -1;
}
int xc_request_commentary_chapter_mode(const xc_request *r) {
  return r && r->status == XC_OK && r->presentation ? (r->presentation->commentary_by_chapter ? 1 : 0) : -1;
}
xc_status xc_worker_set_engine_options(xc_worker *w, uint64_t tab, unsigned flags, xc_request **out) {
  constexpr unsigned known = (1u << 18) - 1;
  if (flags & ~known) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  const auto options = options_from_flags(flags);
  return queue_request(w, out, [=](auto &worker) { return worker.set_engine_options(tab, options); });
}
xc_status xc_worker_save_module_preferences(xc_worker *w, uint64_t tab, const char *path, xc_request **out) {
  if (!path || !*path) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.save_module_preferences(tab, path); });
}
xc_status xc_worker_import_module_preferences(xc_worker *w, uint64_t tab, const char *path, xc_request **out) {
  if (!path || !*path) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.import_module_preferences(tab, path); });
}
xc_status xc_worker_enable_module_preferences(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.enable_module_preferences(tab); });
}
xc_status xc_worker_select_module_variant(xc_worker *w, uint64_t tab, const char *id, int choice, xc_request **out) {
  if (!id || !*id || !xiphos_core_variant_selection(choice).valid) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.select_module_variant(tab, id, choice); });
}
xc_status xc_worker_set_module_engine_options(xc_worker *w, uint64_t tab, const char *id, unsigned flags, xc_request **out) {
  if (!id || !*id || (flags & ~((1u << 18) - 1))) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  const auto options = options_from_flags(flags);
  return queue_request(w, out, [=](auto &worker) { return worker.set_module_engine_options(tab, id, options); });
}
xc_status xc_worker_clear_module_engine_options(xc_worker *w, uint64_t tab, const char *id, xc_request **out) {
  if (!id || !*id) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.clear_module_engine_options(tab, id); });
}
xc_status xc_worker_module_engine_options(xc_worker *w, uint64_t tab, const char *id, xc_request **out) {
  if (!id || !*id) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.module_engine_options(tab, id); });
}
xc_status xc_request_engine_options(const xc_request *r, unsigned *flags, int *configured) {
  if (flags) *flags = 0;
  if (configured) *configured = 0;
  if (!r || !flags || !configured) return XC_INVALID_ARGUMENT;
  if (r->status != XC_OK) return r->status;
  if (!r->options_result) return XC_INVALID_ARGUMENT;
  if (r->options) { *flags = options_to_flags(*r->options); *configured = 1; }
  return XC_OK;
}
xc_status xc_worker_clear_engine_options(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.clear_engine_options(tab); });
}
xc_status xc_worker_follow_pane_link_at(xc_worker *w, uint64_t tab, const char *navigation, const char *source,
                                       const char *ref, const char *uri, xc_request **out) {
  if (!admitted_link_context(navigation, true) || !admitted_link_context(source, true) || !admitted_link_context(ref, true) || !admitted_link_uri(uri)) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) { return worker.follow_pane_link_at(tab, navigation, source, ref, uri); });
}
xc_status xc_worker_follow_study_link_at(xc_worker *w, uint64_t tab, const char *id, const char *context, const char *uri, xc_request **out) {
  if (!admitted_link_context(id) || !admitted_link_context(context, true) || !admitted_link_uri(uri)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.follow_study_link_at(tab, id ? id : "", context, uri); });
}
xc_status xc_worker_resolve_study_link_at(xc_worker *w, uint64_t tab, const char *id, const char *context, const char *uri, xc_request **out) {
  if (!admitted_link_context(id) || !admitted_link_context(context, true) || !admitted_link_uri(uri)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.resolve_study_link_at(tab, id ? id : "", context, uri); });
}
xc_status xc_worker_hover_study_link_at(xc_worker *w, uint64_t tab, const char *id, const char *context, const char *uri, xc_request **out) {
  if (!admitted_link_context(id) || !admitted_link_context(context, true) || !admitted_link_uri(uri)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.hover_study_link_at(tab, id ? id : "", context, uri); });
}
xc_status xc_worker_set_cross_reference_preview_list(xc_worker *w, uint64_t tab, int enabled, xc_request **out) {
  if (enabled != 0 && enabled != 1) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.set_cross_reference_preview_list(tab, enabled != 0); });
}
xc_status xc_worker_cross_reference_preview_list(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.cross_reference_preview_list(tab); });
}
xc_status xc_request_cross_reference_preview_list(const xc_request *r, int *enabled) {
  if (enabled) *enabled = 0;
  if (!r || !enabled) return XC_INVALID_ARGUMENT;
  if (r->status != XC_OK) return r->status;
  if (!r->preview_preference_result) return XC_INVALID_ARGUMENT;
  *enabled = r->preview_preference ? 1 : 0;
  return XC_OK;
}
int xc_request_no_presentation(const xc_request *r) { return r && r->status == XC_OK && r->no_presentation; }
xc_status xc_worker_resolve_study_link(xc_worker *w, uint64_t tab, const char *id, const char *uri, xc_request **out) {
  if (!admitted_link_context(id) || !admitted_link_uri(uri)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.resolve_study_link(tab, id ? id : "", uri); });
}
xc_status xc_worker_study_note_link(xc_worker *w, uint64_t tab, const char *id, const char *uri, xc_request **out) {
  if (!admitted_link_context(id) || !admitted_link_uri(uri)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.study_note_link(tab, id ? id : "", uri); });
}
xc_status xc_worker_enable_lexicon_preferences(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.set_lexicon_preferences(tab, {}); });
}
xc_status xc_worker_set_lexicon_preferences(xc_worker *w, uint64_t tab, const xc_lexicon_names *names, xc_request **out) {
  if (!names) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) {
    const auto copy = [](const char *value) -> std::optional<std::string> { return value ? std::make_optional(std::string(value)) : std::nullopt; };
    xiphos::core::LexiconPreferences preferences;
    preferences.greek = copy(names->greek);
    preferences.hebrew = copy(names->hebrew);
    preferences.greek_morph_nt = copy(names->greek_morph_nt);
    preferences.greek_morph_ot = copy(names->greek_morph_ot);
    preferences.hebrew_morph = copy(names->hebrew_morph);
    return worker.set_lexicon_preferences(tab, std::move(preferences));
  });
}
xc_status xc_worker_lexicon_preferences(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.lexicon_preferences(tab); });
}
xc_status xc_worker_import_lexicon_profile(xc_worker *w, uint64_t tab, const char *path, xc_request **out) {
  if (!path || !*path) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.import_lexicon_profile(tab, path); });
}
xc_status xc_worker_resolve_module_name(xc_worker *w, uint64_t tab, const char *requested, xc_request **out) {
  if (!bounded_image_c_text(requested, 4096)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.resolve_module_name(tab, requested); });
}
xc_status xc_worker_read_search_lists(xc_worker *w, uint64_t tab, const char *path, xc_request **out) {
  if (!path || !*path) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.read_search_lists(tab, path); });
}
xc_status xc_worker_save_search_list(xc_worker *w, uint64_t tab, const char *path,
    const xc_saved_search_list_edit *edit, xc_request **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  constexpr auto maximum = xiphos::core::maximum_saved_search_text_bytes;
  if (!w || !edit || !bounded_image_c_text(path, maximum) ||
      (edit->kind != XC_SAVED_SEARCH_MODULES && edit->kind != XC_SAVED_SEARCH_RANGES) ||
      !edit->label || (*edit->label && !bounded_image_c_text(edit->label, maximum)) ||
      !edit->value || (*edit->value && !bounded_image_c_text(edit->value, maximum))) return XC_INVALID_ARGUMENT;
  try {
    // Allocate both layers before accepting a persistent mutation. Poll then
    // moves the already prepared core snapshot into this handle without allocation.
    auto request = std::make_unique<Request<xiphos::core::SavedSearchLists>>();
    request->search_lists = std::make_unique<xc_search_lists>();
    request->future = w->value.save_search_list(tab, path,
      {static_cast<xiphos::core::SavedSearchListKind>(edit->kind), edit->label, edit->value});
    *out = request.release();
    return XC_OK;
  } catch (...) { return failure(); }
}
xc_status xc_worker_remove_search_list(xc_worker *w, uint64_t tab, const char *path,
    int kind, const char *label, xc_request **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  constexpr auto maximum = xiphos::core::maximum_saved_search_text_bytes;
  if (!w || !bounded_image_c_text(path, maximum) ||
      (kind != XC_SAVED_SEARCH_MODULES && kind != XC_SAVED_SEARCH_RANGES) ||
      !label || (*label && !bounded_image_c_text(label, maximum))) return XC_INVALID_ARGUMENT;
  try {
    auto request = std::make_unique<Request<xiphos::core::SavedSearchLists>>();
    request->search_lists = std::make_unique<xc_search_lists>();
    request->future = w->value.remove_search_list(tab, path,
      static_cast<xiphos::core::SavedSearchListKind>(kind), label);
    *out = request.release();
    return XC_OK;
  } catch (...) { return failure(); }
}
xc_status xc_worker_parse_search_module_list(xc_worker *w, uint64_t tab, const char *text, xc_request **out) {
  if (!text || (*text && !bounded_image_c_text(text, xiphos::core::maximum_saved_search_text_bytes))) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.parse_search_module_list(tab, text); });
}
xc_status xc_worker_save_lexicon_profile(xc_worker *w, uint64_t tab, const char *path, xc_request **out) {
  if (!path || !*path) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.save_lexicon_profile(tab, path); });
}
xc_status xc_worker_visit_lexical(xc_worker *w, uint64_t tab, const char *module, const char *reference,
                               const char *uri, int origin, xc_request **out) {
  if (!uri || !*uri || (origin != XIPHOS_LEXICON_MAIN && origin != XIPHOS_LEXICON_DIALOG)) {
    if (out) *out = nullptr;
    return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) { return worker.visit_lexical(tab, module ? module : "", reference ? reference : "", uri, origin); });
}
xc_status xc_worker_read_lexical(xc_worker *w, uint64_t tab, const char *module, const char *reference,
                               const char *uri, int origin, xc_request **out) {
  if (!uri || !*uri || (origin != XIPHOS_LEXICON_MAIN && origin != XIPHOS_LEXICON_DIALOG)) {
    if (out) *out = nullptr;
    return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) { return worker.read_lexical(tab, module ? module : "", reference ? reference : "", uri, origin); });
}
xc_status xc_worker_dictionary_choices(xc_worker *w, uint64_t tab, const char *id, const char *key, xc_request **out) {
  if (!id || !*id || !key || !*key) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.dictionary_choices(tab, id, key); });
}
xc_dictionary_choices *xc_request_take_dictionary_choices(xc_request *r) { return r && r->status == XC_OK ? r->dictionary_choices.release() : nullptr; }
const char *xc_dictionary_choices_module(const xc_dictionary_choices *c) { return c ? c->value.module.c_str() : nullptr; }
const char *xc_dictionary_choices_key(const xc_dictionary_choices *c) { return c ? c->value.key.c_str() : nullptr; }
size_t xc_dictionary_choices_count(const xc_dictionary_choices *c) { return c ? c->value.keys.size() : 0; }
const char *xc_dictionary_choices_at(const xc_dictionary_choices *c, size_t i) { return c && i < c->value.keys.size() ? c->value.keys[i].c_str() : nullptr; }
void xc_dictionary_choices_release(xc_dictionary_choices *c) { delete c; }
xc_status xc_worker_visit_dictionary(xc_worker *w, uint64_t tab, const char *id, const char *key, xc_request **out) {
  if (!id || !*id || !key || !*key) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.visit_dictionary(tab, id, key); });
}
xc_status xc_worker_navigate_dictionary_history(xc_worker *w, uint64_t tab, int direction, xc_request **out) {
  if (direction != -1 && direction != 1) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.navigate_dictionary_history(tab, direction); });
}
xc_status xc_worker_dictionary_history(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.dictionary_history(tab); });
}
xc_status xc_worker_clear_dictionary_history(xc_worker *w, uint64_t tab, xc_request **out) {
  return queue_request(w, out, [=](auto &worker) { return worker.clear_dictionary_history(tab); });
}
xc_dictionary_history *xc_request_take_dictionary_history(xc_request *r) { return r && r->status == XC_OK ? r->dictionary_history.release() : nullptr; }
size_t xc_dictionary_history_count(const xc_dictionary_history *h) { return h ? h->value.entries.size() : 0; }
size_t xc_dictionary_history_cursor(const xc_dictionary_history *h) { return h ? h->value.cursor : 0; }
int xc_dictionary_history_can_back(const xc_dictionary_history *h) { return h && h->value.can_back; }
int xc_dictionary_history_can_forward(const xc_dictionary_history *h) { return h && h->value.can_forward; }
const char *xc_dictionary_history_module(const xc_dictionary_history *h, size_t i) { return h && i < h->value.entries.size() ? h->value.entries[i].module.c_str() : nullptr; }
const char *xc_dictionary_history_key(const xc_dictionary_history *h, size_t i) { return h && i < h->value.entries.size() ? h->value.entries[i].key.c_str() : nullptr; }
void xc_dictionary_history_release(xc_dictionary_history *h) { delete h; }
xc_status xc_worker_navigate_dictionary(xc_worker *w, uint64_t tab, const char *id, const char *key, int direction, xc_request **out) {
  if (!id || !*id || !key || !*key || (direction != -1 && direction != 1)) {
    if (out) *out = nullptr;
    return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) { return worker.navigate_dictionary(tab, id, key, direction); });
}
xc_status xc_worker_read_dictionary(xc_worker *w, uint64_t tab, const char *id, const char *key, xc_request **out) {
  if (!id || !*id || !key || !*key) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.read_dictionary(tab, id, key); });
}
xc_status xc_worker_read_reference_list(xc_worker *w, uint64_t tab, const char *id, const char *expression, const char *context, xc_request **out) {
  if (!id || !*id || !expression || !context || !*context) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.read_reference_list(tab, id, expression, context); });
}
xc_status xc_worker_study_note(xc_worker *w, uint64_t tab, const char *id, const char *ref, const char *identifier, xc_request **out) {
  if (!id || !*id || !ref || !*ref || !identifier || !*identifier) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  return queue_request(w, out, [=](auto &worker) { return worker.study_note(tab, id, ref, identifier); });
}
xc_status xc_worker_read_tab_entry(xc_worker *w, uint64_t tab, const char *id, const char *ref, xc_request **out) {
  if (!id || !*id || !ref || !*ref) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.read_entry(tab, id, ref); });
}
xc_status xc_worker_read_tab_commentary_chapter(xc_worker *w, uint64_t tab, const char *id, const char *ref, unsigned flags, xc_request **out) {
  if (!id || !*id || !ref || !*ref || (flags & ~(XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED))) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  const xiphos::core::DisplayOptions options{(flags & XC_CHAPTER_VERSE_NUMBERS) != 0, (flags & XC_CHAPTER_HIGHLIGHT_SELECTED) != 0};
  return queue_request(w, out, [=](auto &worker) { return worker.read_commentary_chapter(tab, id, ref, options); });
}
xc_status xc_worker_read_tab_chapter(xc_worker *w, uint64_t tab, const char *id, const char *ref, unsigned flags, xc_request **out) {
  if (!id || !*id || !ref || !*ref || (flags & ~(XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED))) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  const xiphos::core::DisplayOptions options{(flags & XC_CHAPTER_VERSE_NUMBERS) != 0, (flags & XC_CHAPTER_HIGHLIGHT_SELECTED) != 0};
  return queue_request(w, out, [=](auto &worker) { return worker.read_chapter(tab, id, ref, options); });
}
xc_status xc_worker_read_chapter(xc_worker *w, const char *id, const char *ref, unsigned flags, xc_request **out) {
  if (!id || !*id || !ref || !*ref || (flags & ~(XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED))) {
    if (out) *out = nullptr; return XC_INVALID_ARGUMENT;
  }
  const xiphos::core::DisplayOptions options{(flags & XC_CHAPTER_VERSE_NUMBERS) != 0, (flags & XC_CHAPTER_HIGHLIGHT_SELECTED) != 0};
  return queue_request(w, out, [=](auto &worker) { return worker.read_chapter(id, ref, options); });
}
xc_status xc_worker_reference_choices(xc_worker *w, const char *id, const char *ref, xc_request **out) {
  if (!id || !*id || !ref || !*ref) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.reference_choices(id, ref); });
}
xc_status xc_worker_resolve_link(xc_worker *w, const char *id, const char *uri, xc_request **out) {
  if (!admitted_link_context(id) || !admitted_link_uri(uri)) { if (out) *out = nullptr; return XC_INVALID_ARGUMENT; }
  return queue_request(w, out, [=](auto &worker) { return worker.resolve_link(id ? id : "", uri); });
}
xc_status xc_request_poll(xc_request *r) {
  if (!r) return XC_INVALID_ARGUMENT;
  try { r->update(); } catch (...) { r->status = failure(); }
  return r->status;
}
void xc_request_release(xc_request *r) { delete r; }
uint64_t xc_request_tab(const xc_request *r) { return r && r->status == XC_OK ? r->tab : 0; }
xc_reference_list *xc_request_take_reference_list(xc_request *r) { return r && r->status == XC_OK ? r->references.release() : nullptr; }
xc_lexical_entry *xc_request_take_lexical_entry(xc_request *r) { return r && r->status == XC_OK ? r->lexical.release() : nullptr; }
xc_search_lists *xc_request_take_search_lists(xc_request *r) { return r && r->status == XC_OK ? r->search_lists.release() : nullptr; }
xc_module_resolution *xc_request_take_module_resolution(xc_request *r) { return r && r->status == XC_OK ? r->module_resolution.release() : nullptr; }
const char *xc_module_resolution_requested(const xc_module_resolution *r) { return r ? r->value.requested.c_str() : nullptr; }
const char *xc_module_resolution_module(const xc_module_resolution *r) { return r && r->value.module ? r->value.module->c_str() : nullptr; }
int xc_module_resolution_via_abbreviation(const xc_module_resolution *r) { return r && r->value.via_abbreviation ? 1 : 0; }
void xc_module_resolution_release(xc_module_resolution *r) { delete r; }
xc_search_module_names *xc_request_take_search_module_names(xc_request *r) { return r && r->status == XC_OK ? r->search_module_names.release() : nullptr; }
xc_lexicons *xc_request_take_lexicons(xc_request *r) { return r && r->status == XC_OK ? r->lexicons.release() : nullptr; }
xc_dictionary_entry *xc_request_take_dictionary_entry(xc_request *r) { return r && r->status == XC_OK ? r->dictionary.release() : nullptr; }
xc_note *xc_request_take_note(xc_request *r) { return r && r->status == XC_OK ? r->note.release() : nullptr; }
xc_passage *xc_request_take_passage(xc_request *r) { return r && r->status == XC_OK ? r->passage.release() : nullptr; }
const char *xc_passage_module(const xc_passage *p) { return p ? p->value.module.c_str() : nullptr; }
const char *xc_passage_reference(const xc_passage *p) { return p ? p->value.text.reference.c_str() : nullptr; }
const char *xc_passage_html(const xc_passage *p) { return p ? p->value.text.html.c_str() : nullptr; }
int xc_passage_has_commentary(const xc_passage *p) { return p && p->value.commentary ? 1 : 0; }
const char *xc_passage_commentary_module(const xc_passage *p) { return p ? p->value.commentary_module.c_str() : nullptr; }
const char *xc_passage_commentary_reference(const xc_passage *p) { return p && p->value.commentary ? p->value.commentary->reference.c_str() : nullptr; }
const char *xc_passage_commentary_html(const xc_passage *p) { return p && p->value.commentary ? p->value.commentary->html.c_str() : nullptr; }
const char *xc_passage_commentary_heading_html(const xc_passage *p) { return p && p->value.commentary ? p->value.commentary->heading_html.c_str() : nullptr; }
xc_status xc_entry_lexical_layout(const xc_entry *entry, xc_lexical_presentation *out) {
  return borrow_lexical_presentation(entry ? &entry->value.lexical : nullptr, out);
}
xc_status xc_passage_lexical_layout(const xc_passage *passage, xc_lexical_presentation *out) {
  return borrow_lexical_presentation(passage ? &passage->value.text.lexical : nullptr, out);
}
xc_status xc_passage_commentary_lexical_layout(const xc_passage *passage, xc_lexical_presentation *out) {
  return borrow_lexical_presentation(passage && passage->value.commentary ? &passage->value.commentary->lexical : nullptr, out);
}
xc_status xc_chapter_lexical_layout(const xc_chapter *chapter, xc_lexical_presentation *out) {
  return borrow_lexical_presentation(chapter ? &chapter->value.lexical : nullptr, out);
}
xc_status xc_chapter_verse_lexical_layout(const xc_chapter *chapter, size_t verse, xc_lexical_presentation *out) {
  return borrow_lexical_presentation(chapter && verse < chapter->value.verses.size() ? &chapter->value.verses[verse].lexical : nullptr, out);
}
size_t xc_entry_cleanup_count(const xc_entry *e) { return e ? e->value.cleanup_diagnostics.size() : 0; }
int xc_entry_cleanup_kind(const xc_entry *e, size_t index) {
  if (!(e) || index >= e->value.cleanup_diagnostics.size()) return -1;
  return e->value.cleanup_diagnostics[index].kind == xiphos::core::ContentCleanupKind::image ? 0 : 1;
}
const char *xc_entry_cleanup_suffix(const xc_entry *e, size_t index) {
  return e && index < e->value.cleanup_diagnostics.size() ? e->value.cleanup_diagnostics[index].suffix.c_str() : nullptr;
}
size_t xc_passage_cleanup_count(const xc_passage *p) { return p ? p->value.text.cleanup_diagnostics.size() : 0; }
int xc_passage_cleanup_kind(const xc_passage *p, size_t index) {
  if (!(p) || index >= p->value.text.cleanup_diagnostics.size()) return -1;
  return p->value.text.cleanup_diagnostics[index].kind == xiphos::core::ContentCleanupKind::image ? 0 : 1;
}
const char *xc_passage_cleanup_suffix(const xc_passage *p, size_t index) {
  return p && index < p->value.text.cleanup_diagnostics.size() ? p->value.text.cleanup_diagnostics[index].suffix.c_str() : nullptr;
}
size_t xc_passage_commentary_cleanup_count(const xc_passage *p) { return p && p->value.commentary ? (*p->value.commentary).cleanup_diagnostics.size() : 0; }
int xc_passage_commentary_cleanup_kind(const xc_passage *p, size_t index) {
  if (!(p && p->value.commentary) || index >= (*p->value.commentary).cleanup_diagnostics.size()) return -1;
  return (*p->value.commentary).cleanup_diagnostics[index].kind == xiphos::core::ContentCleanupKind::image ? 0 : 1;
}
const char *xc_passage_commentary_cleanup_suffix(const xc_passage *p, size_t index) {
  return p && p->value.commentary && index < (*p->value.commentary).cleanup_diagnostics.size() ? (*p->value.commentary).cleanup_diagnostics[index].suffix.c_str() : nullptr;
}
size_t xc_chapter_verse_cleanup_count(const xc_chapter *c, size_t verse) { return c && verse < c->value.verses.size() ? c->value.verses[verse].cleanup_diagnostics.size() : 0; }
int xc_chapter_verse_cleanup_kind(const xc_chapter *c, size_t verse, size_t index) {
  if (!(c && verse < c->value.verses.size()) || index >= c->value.verses[verse].cleanup_diagnostics.size()) return -1;
  return c->value.verses[verse].cleanup_diagnostics[index].kind == xiphos::core::ContentCleanupKind::image ? 0 : 1;
}
const char *xc_chapter_verse_cleanup_suffix(const xc_chapter *c, size_t verse, size_t index) {
  return c && verse < c->value.verses.size() && index < c->value.verses[verse].cleanup_diagnostics.size() ? c->value.verses[verse].cleanup_diagnostics[index].suffix.c_str() : nullptr;
}
unsigned xc_chapter_verse_ordinal(const xc_chapter *c, size_t verse) {
  return c && verse < c->value.verse_ordinals.size() ? c->value.verse_ordinals[verse] : 0;
}
void xc_passage_release(xc_passage *p) { delete p; }
xc_entry *xc_request_take_entry(xc_request *r) { return r && r->status == XC_OK ? r->entry.release() : nullptr; }
xc_chapter *xc_request_take_chapter(xc_request *r) { return r && r->status == XC_OK ? r->chapter.release() : nullptr; }
xc_choices *xc_request_take_choices(xc_request *r) { return r && r->status == XC_OK ? r->choices.release() : nullptr; }
xc_link *xc_request_take_link(xc_request *r) { return r && r->status == XC_OK ? r->link.release() : nullptr; }
xc_catalog *xc_request_take_catalog(xc_request *r) { return r && r->status == XC_OK ? r->catalog.release() : nullptr; }
xc_history *xc_request_take_history(xc_request *r) { return r && r->status == XC_OK ? r->history.release() : nullptr; }
size_t xc_catalog_count(const xc_catalog *c) { return c ? c->value.size() : 0; }
const char *xc_catalog_id(const xc_catalog *c, size_t i) { auto m = catalog_at(c, i); return m ? m->id.c_str() : nullptr; }
const char *xc_catalog_description(const xc_catalog *c, size_t i) { auto m = catalog_at(c, i); return m ? m->description.c_str() : nullptr; }
const char *xc_catalog_type(const xc_catalog *c, size_t i) { auto m = catalog_at(c, i); return m ? m->type.c_str() : nullptr; }
const char *xc_catalog_language(const xc_catalog *c, size_t i) { auto m = catalog_at(c, i); return m ? m->language.c_str() : nullptr; }
int xc_catalog_is_regular_commentary(const xc_catalog *c, size_t i) { auto m = catalog_at(c, i); return m && m->regular_commentary ? 1 : 0; }
int xc_catalog_is_dictionary(const xc_catalog *c, size_t i) { return c && i < c->value.size() && c->value[i].dictionary ? 1 : 0; }
int xc_catalog_has_book_tree(const xc_catalog *c, size_t i) { return c && i < c->value.size() && c->value[i].indexed_book ? 1 : 0; }
int xc_catalog_has_verse_search(const xc_catalog *c, size_t i) { return c && i < c->value.size() && c->value[i].verse_search ? 1 : 0; }
unsigned xc_catalog_lexicon_roles(const xc_catalog *c, size_t i) { return c && i < c->value.size() ? c->value[i].lexicon_roles : 0; }
unsigned xc_catalog_declared_engine_flags(const xc_catalog *c, size_t i) {
  const auto *module = catalog_at(c, i);
  if (!module) return 0;
  const auto &o = module->options.declared;
  unsigned flags = 0;
  if (o.strongs) flags |= XC_ENGINE_STRONGS;
  if (o.morphs) flags |= XC_ENGINE_MORPHOLOGY;
  if (o.footnotes) flags |= XC_ENGINE_FOOTNOTES;
  if (o.greekaccents) flags |= XC_ENGINE_GREEK_ACCENTS;
  if (o.lemmas) flags |= XC_ENGINE_LEMMAS;
  if (o.scripturerefs) flags |= XC_ENGINE_CROSS_REFERENCES;
  if (o.hebrewpoints) flags |= XC_ENGINE_HEBREW_VOWELS;
  if (o.hebrewcant) flags |= XC_ENGINE_HEBREW_CANTILLATION;
  if (o.italic_headings) flags |= XC_ENGINE_ITALIC_HEADINGS;
  if (o.words_in_red) flags |= XC_ENGINE_WORDS_IN_RED;
  if (o.xlit) flags |= XC_ENGINE_TRANSLITERATED_FORMS;
  if (o.enumerated) flags |= XC_ENGINE_ENUMERATIONS;
  if (o.glosses) flags |= XC_ENGINE_GLOSSES;
  if (o.morphseg) flags |= XC_ENGINE_MORPHEME_SEGMENTATION;
  if (o.transliteration) flags |= XC_ENGINE_TRANSLITERATION;
  if (o.variants_primary) flags |= XC_ENGINE_VARIANTS_PRIMARY;
  if (o.variants_secondary) flags |= XC_ENGINE_VARIANTS_SECONDARY;
  if (o.variants_all) flags |= XC_ENGINE_VARIANTS_ALL;
  return flags;
}
int xc_catalog_has_headings(const xc_catalog *c, size_t i) {
  const auto *module = catalog_at(c, i); return module && module->options.headings;
}
int xc_catalog_transliteration_available(const xc_catalog *c, size_t i) {
  const auto *module = catalog_at(c, i); return module && module->transliteration_available;
}
void xc_catalog_release(xc_catalog *c) { delete c; }
int xc_history_can_back(const xc_history *h) { return h && h->value.can_back; }
int xc_history_can_forward(const xc_history *h) { return h && h->value.can_forward; }
size_t xc_history_count(const xc_history *h) { return h ? h->value.entries.size() : 0; }
const char *xc_history_module(const xc_history *h) {
  return h && h->value.cursor < h->value.entries.size() ? h->value.entries[h->value.cursor].module.c_str() : nullptr;
}
const char *xc_history_commentary(const xc_history *h) {
  return h && h->value.cursor < h->value.entries.size() ? h->value.entries[h->value.cursor].commentary.c_str() : nullptr;
}
void xc_history_release(xc_history *h) { delete h; }
unsigned xc_api_version(void) { return 1; }
xc_status xc_open(const char *root, xc_reader **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  try { *out = new xc_reader(root); return XC_OK; } catch (...) { return failure(); }
}
void xc_close(xc_reader *r) { delete r; }
size_t xc_module_count(const xc_reader *r) { return r ? r->value->modules().size() : 0; }
const char *xc_module_id(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m ? m->id.c_str() : nullptr; }
const char *xc_module_description(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m ? m->description.c_str() : nullptr; }
const char *xc_module_type(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m ? m->type.c_str() : nullptr; }
const char *xc_module_language(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m ? m->language.c_str() : nullptr; }
int xc_module_is_regular_commentary(const xc_reader *r, size_t i) { auto m = module_at(r, i); return m && m->regular_commentary ? 1 : 0; }
int xc_module_is_dictionary(const xc_reader *r, size_t i) { return r && i < r->value->modules().size() && r->value->modules()[i].dictionary ? 1 : 0; }
int xc_module_has_book_tree(const xc_reader *r, size_t i) { return r && i < r->value->modules().size() && r->value->modules()[i].indexed_book ? 1 : 0; }
int xc_module_has_verse_search(const xc_reader *r, size_t i) { return r && i < r->value->modules().size() && r->value->modules()[i].verse_search ? 1 : 0; }
unsigned xc_module_lexicon_roles(const xc_reader *r, size_t i) { return r && i < r->value->modules().size() ? r->value->modules()[i].lexicon_roles : 0; }
xc_status xc_read(xc_reader *r, const char *id, const char *ref, xc_entry **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!r || !id || !*id || !ref || !*ref) return XC_INVALID_ARGUMENT;
  try { *out = new xc_entry{r->value->read(id, ref)}; return XC_OK; }
  catch (...) { return failure(); }
}
xc_status xc_session_open(xc_reader *r, xc_session **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!r) return XC_INVALID_ARGUMENT;
  try { *out = new xc_session(*r); return XC_OK; } catch (...) { return failure(); }
}
void xc_session_close(xc_session *s) { delete s; }
xc_status xc_session_visit(xc_session *s, const char *id, const char *ref, xc_entry **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!s || !id || !*id || !ref || !*ref) return XC_INVALID_ARGUMENT;
  try {
    auto result = std::make_unique<xc_entry>();
    result->value = s->value.visit(id, ref);
    *out = result.release(); return XC_OK;
  } catch (...) { return failure(); }
}
xc_status xc_session_navigate(xc_session *s, int direction, xc_entry **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!s) return XC_INVALID_ARGUMENT;
  try {
    auto result = std::make_unique<xc_entry>();
    result->value = s->value.navigate(direction);
    *out = result.release(); return XC_OK;
  } catch (...) { return failure(); }
}
int xc_session_can_back(const xc_session *s) { return s && s->value.history().can_back(); }
int xc_session_can_forward(const xc_session *s) { return s && s->value.history().can_forward(); }
size_t xc_session_history_count(const xc_session *s) { return s ? s->value.history().entries().size() : 0; }
const char *xc_session_module(const xc_session *s) {
  if (!s || s->value.history().entries().empty()) return nullptr;
  const auto &h = s->value.history(); return h.entries()[h.cursor()].module.c_str();
}
void xc_session_clear_history(xc_session *s) { if (s) s->value.clear_history(); }
xc_status xc_reference_choices(xc_reader *r, const char *id, const char *ref, xc_choices **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!r || !id || !*id || !ref || !*ref) return XC_INVALID_ARGUMENT;
  try { *out = new xc_choices{r->value->reference_choices(id, ref)}; return XC_OK; }
  catch (...) { return failure(); }
}
size_t xc_choices_count(const xc_choices *c, xc_choice_kind kind) { auto v = choices_at(c, kind); return v ? v->size() : 0; }
const char *xc_choices_label(const xc_choices *c, xc_choice_kind kind, size_t i) {
  auto v = choices_at(c, kind); return v && i < v->size() ? (*v)[i].label.c_str() : nullptr;
}
const char *xc_choices_reference(const xc_choices *c, xc_choice_kind kind, size_t i) {
  auto v = choices_at(c, kind); return v && i < v->size() ? (*v)[i].reference.c_str() : nullptr;
}
const char *xc_choices_current_label(const xc_choices *c, xc_choice_kind kind) {
  if (!c) return nullptr;
  switch (kind) {
  case XC_BOOK: return c->value.book_label.c_str();
  case XC_CHAPTER: return c->value.chapter_label.c_str();
  case XC_VERSE: return c->value.verse_label.c_str();
  default: return nullptr;
  }
}
void xc_choices_release(xc_choices *c) { delete c; }
xc_status xc_resolve_link(xc_reader *r, const char *current, const char *uri, xc_link **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!r || !admitted_link_context(current) || !admitted_link_uri(uri)) return XC_INVALID_ARGUMENT;
  try { *out = new xc_link{r->value->resolve_link(current ? current : "", uri)}; return XC_OK; }
  catch (...) { return failure(); }
}
const char *xc_link_module(const xc_link *link) { return link ? link->value.module.c_str() : nullptr; }
const char *xc_link_reference(const xc_link *link) { return link ? link->value.reference.c_str() : nullptr; }
void xc_link_release(xc_link *link) { delete link; }
xc_status xc_read_chapter(xc_reader *r, const char *id, const char *ref, xc_chapter **out) {
  return xc_read_chapter_with_options(r, id, ref,
          XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED, out);
}
xc_status xc_read_chapter_with_options(xc_reader *r, const char *id, const char *ref,
                                      unsigned flags, xc_chapter **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!r || !id || !*id || !ref || !*ref) return XC_INVALID_ARGUMENT;
  if (flags & ~(XC_CHAPTER_VERSE_NUMBERS | XC_CHAPTER_HIGHLIGHT_SELECTED)) return XC_INVALID_ARGUMENT;
  const xiphos::core::DisplayOptions options{
    (flags & XC_CHAPTER_VERSE_NUMBERS) != 0,
    (flags & XC_CHAPTER_HIGHLIGHT_SELECTED) != 0};
  try { *out = new xc_chapter{r->value->read_chapter(id, ref, options)}; return XC_OK; }
  catch (...) { return failure(); }
}
size_t xc_chapter_selected_verse(const xc_chapter *c) { return c ? c->value.selected_verse : 0; }
const char *xc_chapter_selected_anchor(const xc_chapter *c) { return c ? c->value.selected_anchor.c_str() : nullptr; }
const char *xc_chapter_html(const xc_chapter *c) { return c ? c->value.html.c_str() : nullptr; }
const char *xc_chapter_reference(const xc_chapter *c) { return c ? c->value.reference.c_str() : nullptr; }
const char *xc_chapter_title(const xc_chapter *c) { return c ? c->value.title.c_str() : nullptr; }
size_t xc_chapter_count(const xc_chapter *c) { return c ? c->value.verses.size() : 0; }
const char *xc_chapter_verse_reference(const xc_chapter *c, size_t i) {
  return c && i < c->value.verses.size() ? c->value.verses[i].reference.c_str() : nullptr;
}
const char *xc_chapter_verse_html(const xc_chapter *c, size_t i) {
  return c && i < c->value.verses.size() ? c->value.verses[i].html.c_str() : nullptr;
}
const char *xc_chapter_verse_heading_html(const xc_chapter *c, size_t i) {
  return c && i < c->value.verses.size() ? c->value.verses[i].heading_html.c_str() : nullptr;
}
void xc_chapter_release(xc_chapter *c) { delete c; }
xc_status xc_session_navigate_chapter(xc_session *s, const char *id, const char *ref,
                                     int direction, xc_entry **out) {
  if (!out) return XC_INVALID_ARGUMENT;
  *out = nullptr;
  if (!s || !id || !*id || !ref || !*ref) return XC_INVALID_ARGUMENT;
  try {
    auto result = std::make_unique<xc_entry>();
    result->value = s->value.navigate_chapter(id, ref, direction);
    *out = result.release(); return XC_OK;
  } catch (...) { return failure(); }
}
size_t xc_reference_list_count(const xc_reference_list *r) { return r ? r->value.entries.size() : 0; }
const char *xc_reference_list_module(const xc_reference_list *r) { return r ? r->value.module.c_str() : nullptr; }
const char *xc_reference_list_context(const xc_reference_list *r) { return r ? r->value.context.c_str() : nullptr; }
const char *xc_reference_list_expression(const xc_reference_list *r) { return r ? r->value.expression.c_str() : nullptr; }
const char *xc_reference_list_html(const xc_reference_list *r) { return r ? r->value.html.c_str() : nullptr; }
const char *xc_reference_list_entry_reference(const xc_reference_list *r, size_t i) { return r && i < r->value.entries.size() ? r->value.entries[i].reference.c_str() : nullptr; }
const char *xc_reference_list_entry_html(const xc_reference_list *r, size_t i) { return r && i < r->value.entries.size() ? r->value.entries[i].html.c_str() : nullptr; }
int xc_reference_list_limited(const xc_reference_list *r) { return r && r->value.limited; }
unsigned xc_reference_list_skipped(const xc_reference_list *r) { return r ? r->value.skipped : 0; }
void xc_reference_list_release(xc_reference_list *r) { delete r; }
static const std::vector<xiphos::core::SavedSearchList> *search_list_items(const xc_search_lists *lists, int kind) {
  if (!lists) return nullptr;
  if (kind == XC_SAVED_SEARCH_MODULES) return &lists->value.module_lists;
  if (kind == XC_SAVED_SEARCH_RANGES) return &lists->value.ranges;
  return nullptr;
}
size_t xc_search_lists_count(const xc_search_lists *lists, int kind) {
  const auto *items = search_list_items(lists, kind); return items ? items->size() : 0;
}
const char *xc_search_lists_label(const xc_search_lists *lists, int kind, size_t row) {
  const auto *items = search_list_items(lists, kind);
  return items && row < items->size() && (*items)[row].label ? (*items)[row].label->c_str() : nullptr;
}
const char *xc_search_lists_value(const xc_search_lists *lists, int kind, size_t row) {
  const auto *items = search_list_items(lists, kind);
  return items && row < items->size() && (*items)[row].value ? (*items)[row].value->c_str() : nullptr;
}
xc_status xc_search_lists_effective_index(const xc_search_lists *lists, int kind, size_t row, size_t *index) {
  if (!index) return XC_INVALID_ARGUMENT;
  *index = 0;
  const auto *items = search_list_items(lists, kind);
  if (!items || row >= items->size()) return XC_INVALID_ARGUMENT;
  const auto match = xiphos::core::saved_search_list_index(*items, (*items)[row].label);
  if (!match) return XC_ENGINE_ERROR;
  *index = *match; return XC_OK;
}
void xc_search_lists_release(xc_search_lists *lists) { delete lists; }
size_t xc_search_module_names_count(const xc_search_module_names *names) { return names ? names->value.size() : 0; }
const char *xc_search_module_names_at(const xc_search_module_names *names, size_t row) { return names && row < names->value.size() ? names->value[row].c_str() : nullptr; }
void xc_search_module_names_release(xc_search_module_names *names) { delete names; }
int xc_lexicons_configured(const xc_lexicons *lexicons) { return lexicons && lexicons->value ? 1 : 0; }
const char *xc_lexicons_greek(const xc_lexicons *lexicons) { return lexicons && lexicons->value && lexicons->value->greek ? lexicons->value->greek->c_str() : nullptr; }
const char *xc_lexicons_hebrew(const xc_lexicons *lexicons) { return lexicons && lexicons->value && lexicons->value->hebrew ? lexicons->value->hebrew->c_str() : nullptr; }
const char *xc_lexicons_greek_morph_nt(const xc_lexicons *lexicons) { return lexicons && lexicons->value && lexicons->value->greek_morph_nt ? lexicons->value->greek_morph_nt->c_str() : nullptr; }
const char *xc_lexicons_greek_morph_ot(const xc_lexicons *lexicons) { return lexicons && lexicons->value && lexicons->value->greek_morph_ot ? lexicons->value->greek_morph_ot->c_str() : nullptr; }
const char *xc_lexicons_hebrew_morph(const xc_lexicons *lexicons) { return lexicons && lexicons->value && lexicons->value->hebrew_morph ? lexicons->value->hebrew_morph->c_str() : nullptr; }
void xc_lexicons_release(xc_lexicons *lexicons) { delete lexicons; }
int xc_lexical_entry_kind(const xc_lexical_entry *entry) { return entry ? (entry->value.command.kind == xiphos::core::LexicalLinkKind::strongs ? XIPHOS_LEXICON_STRONGS : XIPHOS_LEXICON_MORPHOLOGY) : -1; }
const char *xc_lexical_entry_module(const xc_lexical_entry *entry) { return entry ? entry->value.command.module.c_str() : nullptr; }
const char *xc_lexical_entry_type(const xc_lexical_entry *entry) { return entry ? entry->value.command.type.c_str() : nullptr; }
const char *xc_lexical_entry_value(const xc_lexical_entry *entry) { return entry ? entry->value.command.value.c_str() : nullptr; }
const char *xc_lexical_entry_passage(const xc_lexical_entry *entry) { return entry ? entry->value.command.passage.c_str() : nullptr; }
const char *xc_lexical_entry_source_reference(const xc_lexical_entry *entry) { return entry ? entry->value.source_reference.c_str() : nullptr; }
const char *xc_lexical_entry_dictionary_module(const xc_lexical_entry *entry) { return entry ? entry->value.entry.module.c_str() : nullptr; }
const char *xc_lexical_entry_dictionary_key(const xc_lexical_entry *entry) { return entry ? entry->value.entry.key.c_str() : nullptr; }
const char *xc_lexical_entry_dictionary_html(const xc_lexical_entry *entry) { return entry ? entry->value.entry.html.c_str() : nullptr; }
void xc_lexical_entry_release(xc_lexical_entry *entry) { delete entry; }
const char *xc_dictionary_entry_module(const xc_dictionary_entry *entry) { return entry ? entry->value.module.c_str() : nullptr; }
const char *xc_dictionary_entry_key(const xc_dictionary_entry *entry) { return entry ? entry->value.key.c_str() : nullptr; }
const char *xc_dictionary_entry_html(const xc_dictionary_entry *entry) { return entry ? entry->value.html.c_str() : nullptr; }
void xc_dictionary_entry_release(xc_dictionary_entry *entry) { delete entry; }
const char *xc_note_module(const xc_note *n) { return n ? n->value.module.c_str() : nullptr; }
const char *xc_note_reference(const xc_note *n) { return n ? n->value.reference.c_str() : nullptr; }
const char *xc_note_identifier(const xc_note *n) { return n ? n->value.identifier.c_str() : nullptr; }
const char *xc_note_type(const xc_note *n) { return n ? n->value.type.c_str() : nullptr; }
const char *xc_note_label(const xc_note *n) { return n ? n->value.label.c_str() : nullptr; }
const char *xc_note_html(const xc_note *n) { return n ? n->value.html.c_str() : nullptr; }
const char *xc_note_reference_list(const xc_note *n) { return n ? n->value.reference_list.c_str() : nullptr; }
void xc_note_release(xc_note *n) { delete n; }
const char *xc_entry_reference(const xc_entry *e) { return e ? e->value.reference.c_str() : nullptr; }
const char *xc_entry_heading_html(const xc_entry *e) { return e ? e->value.heading_html.c_str() : nullptr; }
const char *xc_entry_html(const xc_entry *e) { return e ? e->value.html.c_str() : nullptr; }
void xc_entry_release(xc_entry *e) { delete e; }
}
