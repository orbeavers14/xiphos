// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "session.hpp"
#include "profile.hpp"
#include "bookmarks.hpp"
#include "library_organization.hpp"
#include "andbible.hpp"
#include "verse_decoration.hpp"
#include "annotations.hpp"
#include "editable_document.hpp"
#include "remote_install.hpp"
#include "repository_store.hpp"
#include "legacy_repositories.hpp"
#include "legacy_migration.hpp"
#include "legacy_assets.hpp"
#include "recovery.hpp"
#include "sync.hpp"
#include "sync_transport.hpp"
#include <cstdint>
#include <future>
#include <memory>
#include <variant>
namespace xiphos::core {
using StudyTab = std::uint64_t;
struct StudyBatchSearch { std::shared_ptr<SearchControl> control; std::future<SearchBatchResult> result; };
struct StudySearch { std::shared_ptr<SearchControl> control; std::future<SearchResult> result; };
// Read-only resolved general-book/prayer URI target. Raw stripped display key
// retains numeric-prefix/path interpretation for explicit later application.
struct BookUriTarget { BookTree tree; std::string key; std::optional<std::string> anchor; };
struct DictionaryUriEntry { DictionaryEntry entry; std::optional<std::string> anchor; };
using ContextualStudyLink = std::variant<PassageLink, StudyNote, ReferenceList, LexicalEntry, VerseImageResource, DictionaryUriEntry, BookUriTarget>;
using BookPaneLink = std::variant<ContextualStudyLink, BookFollowLink>;
using ResolvedStudyLink = std::variant<PassageLink, StudyNote>;
using StudyHoverLink = std::variant<std::monostate, StudyNote, ReferenceList, LexicalEntry, DictionaryEntry>;
struct StudyTabCapture {
  StudyTab tab=0;
  bool commentary_showing=false;
  TabVisibility visibility;
  bool parallel_showing=false; // Explicit host visibility, never inferred from selection.
};
struct NativeTabCapture {
  StudyTab tab=0;
  std::string key;
  TabVisibility visibility;
  bool commentary_showing=false,parallel_showing=false;
  std::string host_state;
};
struct RestoredSessionTab {
  std::string key;
  StudyTab tab=0;
  TabVisibility visibility;
  bool commentary_showing=false,parallel_showing=false;
};
struct RestoredSession {
  std::vector<RestoredSessionTab> tabs;
  std::optional<StudyTab> active_tab;
};
struct DictionaryHistorySnapshot {
  std::vector<DictionaryLocation> entries;
  std::size_t cursor = 0;
  bool can_back = false, can_forward = false;
};
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
  struct RemoteCatalogTask {std::shared_ptr<TransferControl> control;std::future<RemoteCatalog> result;};
  struct ModuleMaintenanceTask {std::shared_ptr<TransferControl> control;std::future<ModuleMaintenanceResult> result;};
  struct RemoteInstallTask {std::shared_ptr<TransferControl> control;std::future<RemoteInstallResult> result;};
  RemoteCatalogTask fetch_remote_catalog(HttpsRepository repository,TransferOptions options);
  RemoteInstallTask install_remote_module(RemoteCatalog catalog,std::string module,TransferOptions options,bool replace=false);
  std::future<LocalInstallPreview> prepare_local_install(std::string source,std::string module);
  struct LocalInstallTask {std::shared_ptr<TransferControl> control;std::future<LocalInstallResult> result;};
  LocalInstallTask install_local_module_task(LocalInstallRequest request);
  ModuleMaintenanceTask maintain_module_task(ModuleMaintenanceRequest request);
  std::future<LocalInstallResult> install_local_module(LocalInstallRequest request);
  std::future<LibraryCatalog> library_catalog();
  std::future<LibraryCatalog> local_library_catalog(std::string root);
  std::future<ModuleNameResolution> resolve_module_name(StudyTab tab, std::string requested);
  std::future<VerseImageResource> read_verse_inline_image(std::string module,std::string source,
      std::size_t maximum_bytes=maximum_verse_image_bytes);
  std::future<VerseImageResource> read_verse_image_link(std::string module,std::string uri,
      std::size_t maximum_bytes=maximum_verse_image_bytes);
  // Owned, read-only source queries use the same engine thread, without a tab.
  std::future<DictionaryEntry> preview_keyed_search(StudyTab tab,std::string module,std::string key,SearchKeyKind kind,std::size_t maximum_html_bytes);
  StudySearch search_keyed_module_task(StudyTab tab,SearchQuery query,SearchLimits limits = {});
  StudySearch search_module_task(StudyTab tab,SearchQuery query,SearchLimits limits = {});
  std::future<SearchResult> search_module(StudyTab tab,SearchQuery query,SearchLimits limits = {});
  StudyBatchSearch search_mixed_batch_task(StudyTab tab,std::vector<SearchQuery> queries,SearchBatchLimits limits = {});
  StudyBatchSearch search_batch_task(StudyTab tab,std::vector<SearchQuery> queries,SearchBatchLimits limits = {});
  std::future<SearchBatchResult> search_batch(StudyTab tab,std::vector<SearchQuery> queries,SearchBatchLimits limits = {});
  std::future<SearchGenerationInfo> search_generation(StudyTab tab);
  std::future<SearchRetentionLimits> set_search_retention_limits(StudyTab tab,SearchRetentionLimits limits);
  std::future<SearchRetentionLimits> search_retention_limits(StudyTab tab);
  std::future<SearchGenerationRead> read_search_generation(StudyTab tab,std::uint64_t id,std::size_t group,SearchReferenceSource source=SearchReferenceSource::complete,SearchLimits limits = {});
  struct SearchHtmlExportTask {std::shared_ptr<SearchControl> control;std::future<SearchHtmlExport> result;};
  SearchHtmlExportTask export_search_html_task(StudyTab tab,SearchHtmlExportRequest request);
  std::future<SearchHtmlExport> export_search_html(StudyTab tab,SearchHtmlExportRequest request);
  struct SearchExportTask {std::shared_ptr<SearchControl> control;std::future<SearchTextExport> result;};
  SearchExportTask export_search_all_text_task(StudyTab tab,SearchAllTextExportRequest request);
  std::future<SearchTextExport> export_search_all_text(StudyTab tab,SearchAllTextExportRequest request);
  SearchExportTask export_search_visible_text_task(StudyTab tab,SearchTextExportRequest request);
  std::future<SearchTextExport> export_search_visible_text(StudyTab tab,SearchTextExportRequest request);
  std::future<SearchScopeInfo> last_search_scope(StudyTab tab);
  std::future<SearchScopeInfo> clear_search_scope(StudyTab tab);
  std::future<SearchTextState> search_text_options(StudyTab tab);
  std::future<SearchTextState> clear_search_text_options(StudyTab tab);
  std::future<BookTree> book_tree(std::string module, std::uint64_t offset);
  std::future<BookTree> book_tree_path(std::string module, std::string path);
  std::future<std::optional<BookTree>> resolve_saved_book_selection(SavedBookSelection saved, BookRestoreMode mode);
  std::future<BookChildren> book_children(std::string module, std::uint64_t parent,
                                        std::optional<std::uint64_t> after = std::nullopt, std::uint32_t limit = 64);
  // Read with existing tab/module engine policy; no history visit.
  std::future<BookEntry> read_book(StudyTab tab, std::string module, std::uint64_t offset);
  std::future<BookDocument> read_book_document(StudyTab tab, std::string module, std::uint64_t offset);
  std::future<BookDocument> select_book_document(StudyTab tab, BookSelectionTarget target);
  std::future<std::optional<BookDocument>> refresh_book_document(StudyTab tab);
  std::future<std::optional<BookDocument>> navigate_book_document(StudyTab tab, BookNavigation direction);
  std::future<std::optional<BookDocument>> restore_book_document_selection(StudyTab tab, SavedBookSelection saved, BookRestoreMode mode);
  std::future<std::optional<BookDocument>> restore_book_document_profile(StudyTab tab, std::string path, BookRestoreMode mode);
  std::future<std::optional<BookDocument>> book_document_selection(StudyTab tab);
  std::future<BookNote> book_document_note_link(StudyTab tab, std::string uri, bool render_body = true);
  std::future<BookImageTarget> book_image_link(StudyTab tab, std::string uri);
  std::future<BookImageResource> read_book_image_link(StudyTab tab, std::string uri);
  std::future<BookFollowLink> follow_book_link(StudyTab tab, std::string uri);
  // Committed book notes/images keep their source; module URIs reuse contextual
  // routing with the supplied live main reference for Bible range previews.
  std::future<BookPaneLink> follow_book_pane_link_at(StudyTab tab, std::string main_module,
                                                   std::string main_reference, std::string uri);
  std::future<BookEntry> select_book(StudyTab tab, BookSelectionTarget target);
  std::future<std::optional<BookEntry>> navigate_book(StudyTab tab, BookNavigation direction);
  std::future<std::optional<BookEntry>> restore_book_selection(StudyTab tab, SavedBookSelection saved, BookRestoreMode mode);
  // Explicit read-only file import on the engine thread. Uses only saved book
  // fields, retaining current engine preferences and existing profile bytes.
  std::future<std::optional<BookEntry>> restore_book_profile(StudyTab tab, std::string path, BookRestoreMode mode);
  std::future<std::optional<BookTree>> book_selection(StudyTab tab);
  std::future<BookNote> book_note(StudyTab tab, std::string module, std::uint64_t offset,
                                 std::string identifier, bool render_body = true);
  std::future<BookNote> book_note_link_at(StudyTab tab, std::string module, std::uint64_t offset,
                                         std::string uri, bool render_body = true);
  std::future<StudyTab> create_tab();
  std::future<void> close_tab(StudyTab tab);
  std::future<void> set_lexical_layout_enabled(StudyTab tab, bool enabled);
  std::future<void> set_note_numbering_enabled(StudyTab tab, bool enabled);
  std::future<void> set_heading_note_links_enabled(StudyTab tab, bool enabled);
  std::future<void> set_note_numbers(StudyTab tab, std::string module, bool enabled);
  std::future<void> set_lexicon_preferences(StudyTab tab, LexiconPreferences preferences);
  std::future<std::optional<LexiconPreferences>> lexicon_preferences(StudyTab tab);
  std::future<void> set_cross_reference_preview_list(StudyTab tab, bool enabled);
  std::future<bool> cross_reference_preview_list(StudyTab tab);
  // Main-reader no-record hover: shared note policy, committed list context,
  // lexical raw-value snapshots, or explicit no-presentation for showRef/
  // passage links. Status/dictionary-URI/dialog hover remains separate work.
  std::future<StudyHoverLink> hover_study_link_at(StudyTab tab, std::string current_module,
                                               std::string current_reference, std::string uri);
  // Explicit XML profile operations; import publishes a prepared replacement
  // to one tab and returns copied choices. Saved locale is not applied.
  std::future<std::optional<LexiconPreferences>> import_lexicon_profile(StudyTab tab, std::string path);
  // Explicit selected-file read on the FIFO owner; validates the tab but changes no state.
  // Explicit profile storage only: FIFO, copied snapshots, tab checked before IO.
  std::future<AnnotationTarget> annotation_target(StudyTab tab, std::string module, std::string reference, bool generic=false);
  std::future<AnnotationProfile> edit_verse_annotation(StudyTab tab, std::string path, std::string module, std::string reference, bool generic, AnnotationAction action, std::string note = {});
  std::future<AnnotationProfile> edit_verse_annotation_checked(StudyTab tab,std::string path,std::optional<std::string> revision,std::string module,std::string reference,bool generic,AnnotationAction action,std::string note);
  std::future<AnnotationProfile> edit_annotations_checked(StudyTab tab,std::string path,std::optional<std::string> revision,AnnotationEdit edit);
  std::future<AnnotationCache> read_verse_annotation_cache(StudyTab tab, std::string path, std::string module, std::string reference);
  std::future<AnnotationProfile> read_annotations(StudyTab tab, std::string path);
  std::future<AnnotationProfile> edit_annotations(StudyTab tab, std::string path, AnnotationEdit edit);
  std::future<AnnotationCache> read_annotation_cache(StudyTab tab, std::string path, std::string module, std::string book);
  std::future<BookmarkTags> read_bookmark_tags(StudyTab tab, std::string path, std::string module, std::string context, BookmarkTagLimits limits = {});
  std::future<BookmarkSnapshot> read_bookmarks(StudyTab tab, std::string path);
  std::future<BookmarkSnapshot> edit_bookmarks(StudyTab tab, std::string path, std::optional<std::string> revision, BookmarkEdit edit);
  std::future<std::optional<Location>> reading_selection(StudyTab tab);
  std::future<std::optional<DictionaryLocation>> dictionary_selection(StudyTab tab);
  std::future<void> clear_reading_selection(StudyTab tab);
  std::future<void> clear_dictionary_selection(StudyTab tab);
  std::future<void> release_module_editor(StudyTab tab);
  std::future<ModuleEditEntry> read_module_edit(StudyTab tab,ModuleEditTarget target);
  std::future<ModuleEditEntry> save_module_edit(StudyTab tab,ModuleEditTarget target,std::string expected,std::string text,bool erase=false);
  std::future<EditableDocument> read_editable_document(StudyTab tab,std::string path);
  std::future<EditableDocument> save_editable_document(StudyTab tab,std::string path,std::optional<std::string> expected_revision,std::string text);
  std::future<ModuleMaintenancePreview> preview_module_maintenance(std::string module);
  std::future<ModuleMaintenanceResult> maintain_module(ModuleMaintenanceRequest request);
  std::future<void> configure_sync_network(SyncSettings settings,SyncConnection connection);
  std::future<SyncNetworkEvents> poll_sync_network();
  std::future<bool> transmit_sync_network(SyncNavigation navigation,bool received_navigation=false);
  std::future<void> configure_sync(SyncSettings settings);
  std::future<SyncDecision> plan_incoming_sync(SyncNavigation navigation);
  std::future<bool> can_transmit_sync(bool received_navigation);
  std::future<RecoverySnapshot> read_recovery(std::string path);
  std::future<RecoverySnapshot> save_recovery(std::string path,std::optional<std::string> expected,RecoverySnapshot snapshot);
  std::future<ParallelChapter> read_decorated_parallel(StudyTab tab,std::string bookmarks,std::string annotations);
  std::future<DecoratedReading> read_decorated_reading(StudyTab tab,std::string bookmarks,std::string annotations,std::string module,std::string reference,bool chapter,DisplayOptions display={},bool commentary=false);
  std::future<VerseDecoration> read_verse_decoration(StudyTab tab,std::string bookmarks,std::string annotations,std::string module,std::string reference,bool current,bool annotation_highlight,bool verse_highlight);
  std::future<LibraryOrganizationSnapshot> read_library_organization(std::string path);
  std::future<LibraryOrganizationSnapshot> set_library_organization(std::string path,std::optional<std::string> revision,LibraryOrganizationEntry entry);
  std::future<AndBibleImport> read_andbible_bookmarks(std::string path);
  std::future<BookmarkSnapshot> export_bookmark_document(std::string source,std::string source_revision,std::string destination,std::optional<std::string> destination_revision);
  std::future<BookmarkImportReview> review_bookmark_import(std::string path,std::optional<std::string> revision,std::string xml);
  std::future<BookmarkSnapshot> merge_bookmark_document(std::string path,std::optional<std::string> revision,std::string xml,std::string folder,BookmarkImportPolicy policy);
  std::future<BookmarkSnapshot> append_bookmark_document(std::string path,std::optional<std::string> revision,std::string xml,std::string folder);
  struct LegacyAssetsTask {std::shared_ptr<TransferControl> control;std::future<LegacyAssets> result;};
  LegacyAssetsTask inspect_legacy_assets_task(std::string source);
  LegacyAssetsTask copy_legacy_assets_task(std::string source,std::string revision,std::string destination);
  std::future<LegacyAssets> inspect_legacy_assets(std::string source);
  std::future<LegacyAssets> copy_legacy_assets(std::string source,std::string revision,std::string destination);
  std::future<LegacyMigration> prepare_legacy_migration(LegacyMigrationInput input,LegacyModuleMapping mapping={});
  std::future<LegacyMigration> read_legacy_migration(std::string path,LegacyModuleMapping mapping={});
  std::future<LegacyMigration> save_legacy_migration(std::string path,LegacyMigration migration);
  std::future<NativeSessionSnapshot> export_legacy_migration_session(std::string path,LegacyMigration migration);
  std::future<void> restore_legacy_migration_sources(std::string folder,LegacyMigration migration);
  std::future<LegacyRepositoryPreview> preview_legacy_repositories(std::string path);
  std::future<RepositorySnapshot> read_repositories(std::string path);
  std::future<RepositorySnapshot> save_repositories(std::string path,std::optional<std::string> expected,RepositorySnapshot snapshot);
  std::future<NativeSessionSnapshot> read_native_session(std::string path);
  std::future<NativeSessionSnapshot> capture_native_session(std::vector<NativeTabCapture> tabs,std::optional<std::string> active_key = {});
  std::future<NativeSessionSnapshot> save_native_session(std::string path,std::vector<NativeTabCapture> tabs,std::optional<std::string> active_key = {});
  // Replace the entire live tab set only after validation/rendering of all tabs.
  // All previous tab IDs become invalid; new IDs are never reused. Strict policy
  // rejects unavailable required selections/history; dormant preferences survive.
  std::future<RestoredSession> restore_native_session(NativeSessionSnapshot snapshot,SessionCapabilities capabilities = {});
  std::future<RestoredSession> load_native_session(std::string path,SessionCapabilities capabilities = {});
  std::future<SavedTabSession> capture_tabs(std::vector<StudyTabCapture> tabs,std::optional<std::string> version = {});
  std::future<SavedTabSession> save_tabs(std::string path,std::vector<StudyTabCapture> tabs,std::optional<std::string> version = {});
  std::future<SavedTabSession> read_tab_session(StudyTab tab, std::string path);
  std::future<SavedTabSession> save_tab_session(StudyTab tab, std::string path, SavedTabSession session);
  std::future<SavedParallelProfile> read_parallel_profile(StudyTab tab, std::string path);
  std::future<SavedParallelProfile> edit_parallel_profile_checked(StudyTab tab,std::string path,std::optional<std::string> revision,ParallelProfileEdit edit);
  std::future<SavedParallelProfile> edit_parallel_profile(StudyTab tab, std::string path, ParallelProfileEdit edit);
  std::future<SavedSearchLists> read_search_lists(StudyTab tab, std::string path);
  std::future<std::vector<std::string>> parse_search_module_list(StudyTab tab, std::string text);
  std::future<SavedSearchLists> save_search_list(StudyTab tab, std::string path, SavedSearchListEdit edit);
  std::future<SavedSearchLists> remove_search_list(StudyTab tab, std::string path, SavedSearchListKind kind, std::string label);
  std::future<void> save_lexicon_profile(StudyTab tab, std::string path);
  std::future<LexicalEntry> visit_lexical(StudyTab tab, std::string module, std::string reference, std::string uri, int origin);
  std::future<LexicalEntry> read_lexical(StudyTab tab, std::string module, std::string reference,
                                      std::string uri, int origin);
  std::future<DictionaryChoices> dictionary_choices(StudyTab tab, std::string module, std::string key);
  std::future<DictionaryEntry> visit_dictionary_exact(StudyTab tab,std::string module,std::string key);
  std::future<DictionaryEntry> visit_dictionary(StudyTab tab, std::string module, std::string key);
  std::future<DictionaryEntry> navigate_dictionary_history(StudyTab tab, int direction);
  std::future<DictionaryHistorySnapshot> dictionary_history(StudyTab tab);
  std::future<void> clear_dictionary_history(StudyTab tab);
  std::future<DictionaryEntry> navigate_dictionary(StudyTab tab, std::string module, std::string key, int direction);
  std::future<ParallelChapter> visit_parallel(StudyTab tab, std::vector<std::string> modules,
      std::string reference, EngineOptions options, bool headings,
      std::size_t max_bytes = maximum_parallel_snapshot_bytes);
  // Explicit selected-file import; null key uses effective startup fallback,
  // otherwise first listed raw key. Renders before committing one tab selection.
  // Never edits profile metadata; storage-only activation is a separate command.
  std::future<ParallelChapter> visit_parallel_profile(StudyTab tab, std::string path,
      std::optional<std::string> key, std::string reference, EngineOptions options,
      bool headings, std::size_t max_bytes = maximum_parallel_snapshot_bytes, std::string source_module = {});
  std::future<ParallelChapter> read_committed_parallel(StudyTab tab);
  std::future<ParallelChapter> navigate_parallel_chapter(StudyTab tab, int direction);
  std::future<std::optional<ParallelSelection>> parallel_selection(StudyTab tab);
  std::future<void> clear_parallel_selection(StudyTab tab);
  std::future<ParallelChapter> read_parallel(StudyTab tab, std::vector<std::string> modules,
      std::string reference, EngineOptions options, bool headings,
      std::size_t max_bytes = maximum_parallel_snapshot_bytes);
  // Bounded admission before FIFO; host supplies local today. No history visit.
  std::future<DevotionalEntry> read_devotional(StudyTab tab, std::string module,
      std::string entered, int today_month, int today_day);
  std::future<DictionaryEntry> read_dictionary(StudyTab tab, std::string module, std::string key);
  std::future<PassageHtmlExport> export_passage_html(StudyTab tab, std::string module,
      std::string expression, std::string context, std::size_t max_bytes = maximum_passage_text_export_bytes);
  std::future<PassageTextExport> export_passage_text(StudyTab tab, std::string module,
      std::string expression, std::string context, std::size_t max_bytes = maximum_passage_text_export_bytes);
  std::future<ReferenceList> read_localized_reference_list(StudyTab tab,std::string module,std::string expression,
      std::string context,std::string locale_file);
  std::future<ReferenceList> read_reference_list(StudyTab tab, std::string module, std::string expression, std::string context);
  // Study-link inputs use study_link_admission.hpp byte bounds; oversized/NUL
  // caller-owned values produce ready exceptional futures before queueing.
  // Valid copied requests keep FIFO/tab/lifetime behavior.
  // showRef previews use the supplied source reference and never visit history.
  // Requested modules use exact membership, falling back to the navigation/main
  // module (current_module for older single-context calls), never URI aliases.
  // Same dispatch, explicitly recording successful lexical dictionary visits.
  std::future<ContextualStudyLink> follow_pane_link_at(StudyTab tab, std::string navigation_module, std::string source_module,
                                                    std::string current_reference, std::string uri);
  std::future<ContextualStudyLink> follow_study_link_at(StudyTab tab, std::string current_module, std::string current_reference, std::string uri);
  std::future<ContextualStudyLink> resolve_study_link_at(StudyTab tab, std::string current_module, std::string current_reference, std::string uri);
  std::future<ResolvedStudyLink> resolve_study_link(StudyTab tab, std::string current_module, std::string uri);
  std::future<StudyNote> study_note_link(StudyTab tab, std::string current_module, std::string uri);
  std::future<StudyNote> study_note(StudyTab tab, std::string module, std::string reference, std::string identifier);
  std::future<WordStudy> word_study(StudyTab tab, std::string module, std::string reference, std::string surface);
  std::future<Entry> read_entry(StudyTab tab, std::string module, std::string reference);
  std::future<EntryHtmlSource> capture_entry_html(StudyTab tab,std::string module,std::string reference,HtmlSourceLimits limits = {});
  std::future<Entry> read_entry_snapshot(StudyTab tab,std::string module,std::string reference);
  std::future<Entry> read_commentary_entry_snapshot(StudyTab tab,std::string module,std::string reference);
  std::future<StudyPassage> select_commentary(StudyTab tab,std::string commentary);
  std::future<StudyPassage> read_passage(StudyTab tab, std::string module, std::string commentary, std::string reference);
  std::future<StudyPassage> visit_passage(StudyTab tab, std::string module, std::string commentary, std::string reference);
  std::future<StudyPassage> visit_linked_passage(StudyTab tab, std::string bible, std::string commentary, PassageLink target, bool commentary_visible = true);
  std::future<StudyPassage> navigate_passage(StudyTab tab, int direction);
  std::future<StudyPassage> navigate_passage_chapter(StudyTab tab, std::string module, std::string commentary,
                                                  std::string reference, int direction);
  std::future<Entry> visit(StudyTab tab, std::string module, std::string reference);
  std::future<Entry> navigate(StudyTab tab, int direction);
  std::future<Entry> navigate_chapter(StudyTab tab, std::string module, std::string reference, int direction);
  std::future<void> set_engine_options(StudyTab tab, EngineOptions options);
  std::future<void> clear_engine_options(StudyTab tab);
  std::future<void> enable_module_preferences(StudyTab tab);
  std::future<void> import_module_preferences(StudyTab tab, std::string path);
  std::future<void> save_module_preferences(StudyTab tab, std::string path);
  std::future<void> select_module_variant(StudyTab tab, std::string module, int choice);
  std::future<void> set_module_engine_options(StudyTab tab, std::string module, EngineOptions options);
  std::future<void> clear_module_engine_options(StudyTab tab, std::string module);
  std::future<std::optional<EngineOptions>> module_engine_options(StudyTab tab, std::string module);
  std::future<ModulePresentationOptions> module_presentation_options(StudyTab tab, std::string module);
  std::future<void> set_double_space(StudyTab tab, std::string module, bool enabled);
  std::future<void> set_image_content(StudyTab tab, std::string module, bool enabled);
  std::future<void> set_respect_font_faces(StudyTab tab, std::string module, bool enabled);
  std::future<void> set_headings(StudyTab tab, std::string module, bool enabled);
  std::future<void> set_commentary_chapter_mode(StudyTab tab, std::string module, bool enabled);
  std::future<Chapter> read_commentary_chapter(StudyTab tab, std::string module, std::string reference, DisplayOptions options = {});
  std::future<Chapter> read_chapter(StudyTab tab, std::string module, std::string reference, DisplayOptions options = {});
  std::future<HistorySnapshot> history(StudyTab tab);
  std::future<void> clear_history(StudyTab tab);
  std::future<Chapter> read_chapter(std::string module, std::string reference, DisplayOptions options = {});
  std::future<ReferenceChoices> reference_choices(std::string module, std::string reference);
  std::future<ReferenceChoices> reference_choices(std::string module, std::string reference, std::string locale_file);
  std::future<PassageLink> map_reference(std::string source, std::string destination, std::string reference);
  std::future<PassageLink> resolve_localized_reference(std::string module, std::string expression, std::string locale_file);
  std::future<PassageLink> resolve_link(std::string module, std::string uri);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
