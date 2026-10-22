// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "module_resolution.hpp"
#include "reference_list.hpp"
#include "render.hpp"
#include "study_note.hpp"
#include "heading_notes.hpp"
#include "engine_options.hpp"
#include "content.hpp"
#include "link.hpp"
#include "navigation.hpp"
#include "lexical.hpp"
#include "book_tree.hpp"
#include "book_document.hpp"
#include "book_resource.hpp"
#include "verse_image.hpp"
#include "search.hpp"
#include "book_selection.hpp"
#include <memory>
#include <string>
#include <vector>
namespace xiphos::core {
namespace detail { class SearchScope; }
struct Module {
  std::string id, description, type, language;
  ModuleOptions options;
  bool transliteration_available = false;
  bool dictionary = false;
  unsigned lexicon_roles = 0;
  bool regular_commentary = false;
  bool indexed_book = false;
  bool verse_search = false;
};


struct DictionaryChoices { std::string module, key; std::vector<std::string> keys; };
class Reader {
public:
  // Empty root uses SWORD's standard discovery. Explicit root disables home
  // augmentation, permitting deterministic, isolated tests and previews.
  explicit Reader(const std::string &root);
  ~Reader();
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;
  const std::vector<Module> &modules() const;
  // Explicit read-only alias resolution on this Reader owner. Catalog metadata
  // stages lazily; invalid alias metadata does not disable ordinary reading.
  ModuleNameResolution resolve_module_name(const std::string &requested);
  // Bible/regular-commentary search resolves target aliases while preserving
  // raw requests and explicit bounds providers. Copied bounded
  // output and scoped accent/vowel filter settings. No history, session scope,
  // index creation or native UI. Optional cooperative controls never expose SDK state.
  SearchResult search_module(SearchQuery query,const SearchLimits &limits = {},std::shared_ptr<SearchControl> control = {});
  // Read-only owned one-hop tree sources. Neither call selects/renders a book,
  // changes history, or writes missing paths. Root offset 0 is not reading
  // startup's legacy offset-4 selection. Unsupported drivers are rejected.
  BookTree book_tree(const std::string &module, std::uint64_t offset);
  BookTree book_tree_path(const std::string &module, const std::string &path);
  // Validates a typed candidate with private SDK query keys. Relative display
  // paths normalize through SWORD. No live selection, render or writes.
  BookTree resolve_book_target(const BookSelectionTarget &target);
  // SDK child order, at most `limit` copied nodes (1..256). The optional cursor
  // is the last child from the preceding page, never a UI list index.
  BookChildren book_children(const std::string &module, std::uint64_t parent,
                             std::optional<std::uint64_t> after = std::nullopt, std::uint32_t limit = 64);
  // Configured raw SDK HTML paired with its exact typed tree source. Selects
  // the requested live node, as ordinary reads do; no root-to-first-node clamp,
  // display cleanup, semantic link dispatch, history or persistence yet.
  BookEntry read_book(const std::string &module, std::uint64_t offset, const EngineOptions &options);
  // Unconfigured overload retains the manager baseline, as other Reader reads do.
  BookEntry read_book(const std::string &module, std::uint64_t offset);
  // Configured DisplayLevel depth-first document. Every section owns its exact
  // source; bounded failures publish nothing. Raw single-entry APIs stay stable.
  BookDocument read_book_document(const std::string &module, std::uint64_t offset,
                                  const EngineOptions &options, const BookDocumentLimits &limits = {});
  BookDocument read_book_document(const std::string &module, std::uint64_t offset,
                                  const BookDocumentLimits &limits = {});
  // Opt-in SDK note-label scope and terminal attribute capture for later
  // combined-document numbering. Neither overload runs presentation numbering.
  BookDocument read_book_document(const std::string &module, std::uint64_t offset,
      const EngineOptions &options, bool labels, const BookDocumentLimits &limits = {});
  // Exact SDK path/CSS metadata only; no path rebasing, resource reads or writes.
  BookResourceContext book_resource_context(const std::string &module);
  BookImageResource read_book_image(const std::string &module, const std::string &uri,
                                    std::size_t maximum_bytes = maximum_book_image_bytes);
  // Inline HTML image URLs have different decoding from command query values.
  // Uses the same bounded module-directory file policy and owned byte result.
  BookImageResource read_book_inline_image(const std::string &module, const std::string &source,
                                           std::size_t maximum_bytes = maximum_book_image_bytes);
  // Exact rendered Bible/commentary image source, resolved under that module's
  // actual SDK directory. No network reads, key/history/filter changes or decode.
  VerseImageResource read_verse_inline_image(const std::string &module,const std::string &source,
      std::size_t maximum_bytes=maximum_verse_image_bytes);
  // Actual SDK showImage command: query decoding and source-URI decoding are
  // distinct. Declared alternate modules cannot rebase the supplied row module.
  VerseImageResource read_verse_image_link(const std::string &module,const std::string &uri,
      std::size_t maximum_bytes=maximum_verse_image_bytes);
  // Missing module stylesheet is ordinary absence. Config chooses
  // PreferredCSSXHTML exactly (including empty), otherwise style.css. Local
  // regular files are bounded to the module data directory; no global CSS or
  // recursive CSS imports/assets are discovered by this read-only operation.
  std::optional<BookStylesheetResource> read_book_stylesheet(const std::string &module,
      std::size_t maximum_bytes = maximum_book_stylesheet_bytes);
  // Owned ordinary book-note source; does not parse a verse URI or heading ID.
  BookNote book_note(const std::string &module, std::uint64_t offset, const std::string &identifier,
                     const EngineOptions &options, bool render_body = true);
  BookNote book_note(const std::string &module, std::uint64_t offset, const std::string &identifier,
                     bool render_body = true);
  ReferenceChoices reference_choices(const std::string &module, const std::string &reference);
  // Single-passage URI aliases resolve before unknown offered names fall back
  // to the exact installed current/main ID. No navigation or history mutation.
  PassageLink resolve_link(const std::string &current_module, const std::string &url);
  // Owned targets/guard metadata from the existing parser, without rendering
  // verse bodies or changing the module key/filter state.
  ParsedReferenceList reference_targets(const std::string &module, const std::string &expression,
                                         const std::string &context);
  ReferenceList read_reference_list(const std::string &module, const std::string &expression,
                                    const std::string &context);
  ReferenceList read_reference_list(const std::string &module, const std::string &expression,
                                    const std::string &context, const EngineOptions &options);
  // Configured owned heading source with scoped IDs, suitable for future native
  // publication. The existing study_note/metadata calls resolve these IDs using
  // source fragment context, never the body's SDK note namespace. No history.
  HeadingNoteSource heading_notes(const std::string &module, const std::string &reference,
                                  std::size_t fragment, const EngineOptions &options,
                                  bool render_note_labels, bool render_bodies = true);
  StudyNote study_note(const std::string &module, const std::string &reference,
                       const std::string &identifier);
  StudyNote study_note(const std::string &module, const std::string &reference,
                       const std::string &identifier, const EngineOptions &options);
  StudyNote study_note_metadata(const std::string &module, const std::string &reference,
                                 const std::string &identifier);
  StudyNote study_note_metadata(const std::string &module, const std::string &reference,
                                 const std::string &identifier, const EngineOptions &options);
  // Selection uses a cloned source key; returns an owned selected module name.
  std::string select_lexicon(const LexicalLink &command, const std::string &source_reference,
                            int origin, const LexiconPreferences &preferences);
  // Explicit dictionary module/key. SWORD owns matching/normalization semantics.
  DictionaryEntry read_dictionary(const std::string &module, const std::string &key);
  DictionaryEntry read_dictionary(const std::string &module, const std::string &key, const EngineOptions &options);
  // One SDK key step; direction is -1 (previous) or +1 (next).
  DictionaryChoices dictionary_choices(const std::string &module, const std::string &key);
  DictionaryEntry navigate_dictionary(const std::string &module, const std::string &key, int direction);
  DictionaryEntry navigate_dictionary(const std::string &module, const std::string &key, int direction, const EngineOptions &options);
  // Configured opt-in note-label reads use owned per-filter scopes and capture
  // source/heading metadata before later renders replace it. No automatic *n/*x
  // insertion occurs here. Existing overloads keep the unconfigured label path.
  Entry read(const std::string &module, const std::string &reference, const EngineOptions &options, bool render_note_labels);
  Entry read_commentary_entry(const std::string &module, const std::string &reference, bool headings, const EngineOptions &options, bool render_note_labels);
  Chapter read_chapter(const std::string &module, const std::string &reference, const DisplayOptions &display, const EngineOptions &engine, bool render_note_labels);
  Chapter read_commentary_chapter(const std::string &module, const std::string &reference, const DisplayOptions &display, const EngineOptions &engine, bool headings, bool render_note_labels);
  Entry read_commentary_entry(const std::string &module, const std::string &reference, bool headings = true);
  Entry read_commentary_entry(const std::string &module, const std::string &reference, bool headings, const EngineOptions &options);
  // SDK stripText with optional configured engine state, for text export.
  // Copied byte guard only; no HTML stripping approximation or history mutation.
  std::string read_stripped(const std::string &module,const std::string &reference,
                            const std::optional<EngineOptions> &options,std::size_t max_bytes);
  Entry read(const std::string &module, const std::string &reference);
  Entry read(const std::string &module, const std::string &reference, const EngineOptions &options);
  Chapter read_chapter(const std::string &module, const std::string &reference,
                       const DisplayOptions &display, const EngineOptions &engine);
  Chapter read_chapter(const std::string &module, const std::string &reference,
                       const DisplayOptions &options = {});
  // Sparse/linked main-commentary traversal; headings/cache/editing pending.
  Chapter read_commentary_chapter(const std::string &module, const std::string &reference,
                                  const DisplayOptions &display = {}, bool headings = true);
  Chapter read_commentary_chapter(const std::string &module, const std::string &reference,
                                  const DisplayOptions &display, const EngineOptions &engine, bool headings = true);
  std::string adjacent_chapter(const std::string &module, const std::string &reference,
                               int direction);
private:
  friend class StudySession;
  SearchResult search_impl(SearchQuery query,const SearchLimits &limits,std::shared_ptr<SearchControl> control,
                           std::shared_ptr<const detail::SearchScope> *scope,
                           const std::optional<SearchTextOptions> &previous_text = {},
                           bool nested_job=false,unsigned progress_begin=0,unsigned progress_end=100,bool allow_empty_output=false,
                           detail::SearchRetainedKeys *retained=nullptr,SearchRetentionLimits retention_limits = {});
  BookNote book_note_impl(const std::string &module, std::uint64_t offset, const std::string &identifier,
                          const EngineOptions *options, bool render_body);
  BookDocument book_document_impl(const std::string &module, std::uint64_t offset, const EngineOptions *options, const BookDocumentLimits &limits, const bool *labels = nullptr);
  BookEntry book_impl(const std::string &module, std::uint64_t offset, const EngineOptions *options);
  DictionaryEntry dictionary_impl(const std::string &module, const std::string &key, const EngineOptions *options, int direction = 0);
  ReferenceList reference_list_impl(const std::string &module, const std::string &expression,
                                    const std::string &context, const EngineOptions *options);
  StudyNote note_impl(const std::string &module, const std::string &reference,
                       const std::string &identifier, const EngineOptions *options, bool render_body = true);
  Entry read_impl(const std::string &module, const std::string &reference, const EngineOptions *options,
                  bool headings = false, bool commentary_only = false, const bool *render_note_labels = nullptr,
                  const std::size_t *max_html_bytes = nullptr);
  Chapter chapter_impl(const std::string &module, const std::string &reference,
                       const DisplayOptions &display, const EngineOptions *engine, const bool *render_note_labels = nullptr);
  Chapter commentary_chapter_impl(const std::string &module, const std::string &reference,
                                   const DisplayOptions &display, const EngineOptions *engine, bool headings, const bool *render_note_labels = nullptr);
  VerseImageResource verse_image_impl(const std::string &module,const std::string &source,
      std::size_t maximum_bytes,bool command);
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
