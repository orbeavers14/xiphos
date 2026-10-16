/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XIPHOS_CORE_C_API_H
#define XIPHOS_CORE_C_API_H
#include <stddef.h>
#include <stdint.h>
#include "variant.h"
#include "lexicon.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct xc_reader xc_reader;
typedef struct xc_entry xc_entry;
typedef struct xc_passage xc_passage;
typedef struct xc_note xc_note;
typedef struct xc_dictionary_choices xc_dictionary_choices;
typedef struct xc_dictionary_history xc_dictionary_history;
typedef struct xc_dictionary_entry xc_dictionary_entry;
typedef struct xc_book_tree xc_book_tree;
typedef struct xc_book_entry xc_book_entry;
typedef struct xc_book_children xc_book_children;
typedef struct xc_book_note xc_book_note;
/* Submission-only borrowed strings, copied before return. NULL is absent;
 * empty text remains explicit. No implicit XML/profile access or writes. */
typedef struct { const char *module, *key, *offset_text; } xc_saved_book_selection;
typedef enum xc_book_restore_mode {
  XC_BOOK_RESTORE_DISPLAY_KEY = 0, XC_BOOK_RESTORE_INITIAL_WINDOW = 1
} xc_book_restore_mode;
typedef struct xc_lexical_entry xc_lexical_entry;
typedef struct xc_lexicons xc_lexicons;
/* Borrowed submission strings. NULL is unset; empty is an explicit empty name. */
typedef struct {
  const char *greek, *hebrew, *greek_morph_nt, *greek_morph_ot, *hebrew_morph;
} xc_lexicon_names;
typedef struct xc_reference_list xc_reference_list;
typedef struct xc_session xc_session;
typedef struct xc_chapter xc_chapter;
typedef struct xc_link xc_link;
typedef struct xc_choices xc_choices;
typedef struct xc_worker xc_worker;
typedef struct xc_request xc_request;
typedef struct xc_catalog xc_catalog;
typedef struct xc_history xc_history;
typedef enum xc_status { XC_OK = 0, XC_INVALID_ARGUMENT = 1,
  XC_MODULE_NOT_FOUND = 2, XC_ENGINE_ERROR = 3, XC_PENDING = 4 } xc_status;
/* Copied flags and borrowed CSS from an owned HTML snapshot. CSS remains valid
 * until that snapshot is released. requested/applied describe annotation word
 * layout; CSS may contain spacing-only rules while both flags are zero. */
typedef struct { const char *css; int requested, applied; } xc_lexical_presentation;
/* Explicit SWORD filter settings, not saved-preference defaults. All-zero
 * means Off filters with engine headings On; clear restores unconfigured reads.
 * Variant flags retain legacy Primary > Secondary > All precedence; no variant
 * flag leaves the engine's existing variant value unchanged during the scope. */
typedef enum xc_engine_option_flag {
  XC_ENGINE_STRONGS = 1u << 0,
  XC_ENGINE_MORPHOLOGY = 1u << 1,
  XC_ENGINE_FOOTNOTES = 1u << 2,
  XC_ENGINE_GREEK_ACCENTS = 1u << 3,
  XC_ENGINE_LEMMAS = 1u << 4,
  XC_ENGINE_CROSS_REFERENCES = 1u << 5,
  XC_ENGINE_HEBREW_VOWELS = 1u << 6,
  XC_ENGINE_HEBREW_CANTILLATION = 1u << 7,
  XC_ENGINE_ITALIC_HEADINGS = 1u << 8,
  XC_ENGINE_WORDS_IN_RED = 1u << 9,
  XC_ENGINE_TRANSLITERATED_FORMS = 1u << 10,
  XC_ENGINE_ENUMERATIONS = 1u << 11,
  XC_ENGINE_GLOSSES = 1u << 12,
  XC_ENGINE_MORPHEME_SEGMENTATION = 1u << 13,
  XC_ENGINE_TRANSLITERATION = 1u << 14,
  XC_ENGINE_VARIANTS_PRIMARY = 1u << 15,
  XC_ENGINE_VARIANTS_SECONDARY = 1u << 16,
  XC_ENGINE_VARIANTS_ALL = 1u << 17
} xc_engine_option_flag;
/* Async native API: share ONE worker per process. Open starts initialization on
 * its engine thread; accepted requests return XC_OK with a non-NULL request.
 * Initialization/operation errors arrive through poll. Poll never waits; it
 * caches the terminal status. Each request handle must be used serially.
 * Release drops observation, NOT the queued operation (including history).
 * Close drains and joins: run close OFF the UI thread, after submissions stop.
 * No active engine-call cancellation or unrelated SWORD concurrency promise. */
xc_status xc_worker_open(const char *root, xc_worker **out);
void xc_worker_close(xc_worker *worker);
/* No tab/UI publication: resolve through private SDK query keys. Relative
 * display paths use successful SDK normalization; missing snaps fail at poll.
 * mode accepts exactly the two restore constants. On XC_OK, presence is 0 for
 * no saved module and 1 for a take-once xc_book_tree. Copies outlive worker close. */
xc_status xc_worker_resolve_saved_book_selection(xc_worker *worker, const xc_saved_book_selection *saved,
                                                int mode, xc_request **out);
/* -1 unless this is a successful selection resolution/restore/snapshot result;
 * otherwise 0/1. The called operation determines tree versus entry payload.
 * Presence remains stable after taking the tree; take itself is still once. */
int xc_request_book_selection_present(const xc_request *request);
/* Explicit per-tab publication after validation/render success; owned entry
 * result, no Bible/dictionary history visit or preference writes. */
xc_status xc_worker_select_book(xc_worker *worker, uint64_t tab, const char *module, uint64_t offset, xc_request **out);
/* Same explicit context rules as read-only resolution, but successful rendering
 * publishes the tab selection. Presence 1 -> take_book_entry, 0 -> no saved book
 * and no state change. Failure retains the previous committed tab selection. */
xc_status xc_worker_restore_book_selection(xc_worker *worker, uint64_t tab, const xc_saved_book_selection *saved,
                                          int mode, xc_request **out);
/* Same optional-entry publication from an explicitly selected XML file. Copies
 * path before return; reads/parses on the engine thread. No discovery, profile
 * writes, locale/lexicon import or changes to existing engine preferences.
 * Missing/invalid files or targets preserve the committed tab selection. */
xc_status xc_worker_restore_book_profile(xc_worker *worker, uint64_t tab, const char *path, int mode, xc_request **out);
/* Copied optional committed source: presence 1 -> take_book_tree, 0 -> none. */
xc_status xc_worker_book_selection(xc_worker *worker, uint64_t tab, xc_request **out);
/* Owned tree discovery, no tab or live selection mutation. Empty canonical path
 * selects root; numeric titles require their slash path. Read uses the existing
 * tab/module engine policy and selects the exact offset, without a history visit.
 * Strings copy at submission. Invalid/missing nodes fail at poll; existing
 * XC_MODULE_NOT_FOUND also represents a missing tree node/path in these calls.
 * Raw entry HTML has no native book-link/image or display-cleanup policy yet. */
xc_status xc_worker_book_tree(xc_worker *worker, const char *module, uint64_t offset, xc_request **out);
xc_status xc_worker_book_tree_path(xc_worker *worker, const char *module, const char *path, xc_request **out);
xc_status xc_worker_read_book(xc_worker *worker, uint64_t tab, const char *module, uint64_t offset, xc_request **out);
xc_book_tree *xc_request_take_book_tree(xc_request *request);
xc_book_entry *xc_request_take_book_entry(xc_request *request);
typedef enum xc_book_node_kind {
  XC_BOOK_CURRENT = 0, XC_BOOK_PARENT = 1, XC_BOOK_FIRST_CHILD = 2,
  XC_BOOK_PREVIOUS_SIBLING = 3, XC_BOOK_NEXT_SIBLING = 4
} xc_book_node_kind;
/* Borrowed node strings until the owning tree/entry is released. present=0 is
 * an absent relationship, distinct from a present root at offset 0. Invalid
 * handles/kinds clear a non-NULL out value and return XC_INVALID_ARGUMENT. */
typedef struct {
  uint64_t offset;
  const char *path, *title;
  int present, has_children;
} xc_book_node;
xc_status xc_book_tree_get_node(const xc_book_tree *tree, int kind, xc_book_node *out);
const char *xc_book_tree_module(const xc_book_tree *tree);
void xc_book_tree_release(xc_book_tree *tree);
/* Borrowed tree: do not release it separately; its entry owns its lifetime. */
const xc_book_tree *xc_book_entry_tree(const xc_book_entry *entry);
const char *xc_book_entry_html(const xc_book_entry *entry);
int xc_book_entry_has_entry(const xc_book_entry *entry);
void xc_book_entry_release(xc_book_entry *entry);
/* Read-only SDK child order, page limit 1..256; has_after is strict 0/1.
 * after is ignored when has_after=0; otherwise it must identify a direct child
 * of parent. All node strings borrow from the owned page. Out-of-range indices
 * clear out and return XC_INVALID_ARGUMENT. Pages do not freeze an externally
 * modified module across requests. No automatic recursive/full-tree walk. */
xc_status xc_worker_book_children(xc_worker *worker, const char *module, uint64_t parent,
                                  int has_after, uint64_t after, uint32_t limit, xc_request **out);
xc_book_children *xc_request_take_book_children(xc_request *request);
const char *xc_book_children_module(const xc_book_children *page);
size_t xc_book_children_count(const xc_book_children *page);
xc_status xc_book_children_parent(const xc_book_children *page, xc_book_node *out);
xc_status xc_book_children_get_node(const xc_book_children *page, size_t index, xc_book_node *out);
/* 1 and last-child cursor if more exist; otherwise 0, clearing non-NULL out. */
int xc_book_children_next_after(const xc_book_children *page, uint64_t *out);
void xc_book_children_release(xc_book_children *page);
/* Exact typed book source, existing tab/module engine policy, no history visit.
 * render_body is strict 0/1; metadata-only avoids rendering the note body.
 * Ordinary SDK note IDs only; no verse URI or heading-note dispatch is added.
 * Content/tree getters borrow from the owned book-note: do not release them. */
xc_status xc_worker_book_note(xc_worker *worker, uint64_t tab, const char *module, uint64_t offset,
                              const char *identifier, int render_body, xc_request **out);
/* Same result/ownership, requiring the command's decoded module/path/type to
 * match the captured typed source. Does not relax existing verse URI parsing. */
xc_status xc_worker_book_note_link_at(xc_worker *worker, uint64_t tab, const char *module, uint64_t offset,
                                      const char *uri, int render_body, xc_request **out);
xc_book_note *xc_request_take_book_note(xc_request *request);
const xc_book_tree *xc_book_note_tree(const xc_book_note *note);
const xc_note *xc_book_note_content(const xc_book_note *note);
void xc_book_note_release(xc_book_note *note);
xc_status xc_worker_modules(xc_worker *worker, xc_request **out);
xc_status xc_worker_create_tab(xc_worker *worker, xc_request **out);
xc_status xc_worker_close_tab(xc_worker *worker, uint64_t tab, xc_request **out);
/* Consumer migration capability, not a saved module preference. Accepts exactly
 * 0/1; default off. Enable only when consuming HTML and matching CSS together.
 * FIFO, tab-local, history-neutral; invalid/closed tab errors arrive at poll. */
xc_status xc_worker_set_lexical_layout_enabled(xc_worker *worker, uint64_t tab, int enabled, xc_request **out);
/* Separate default-off migration capability for core note numbering. Requires
 * configured engine options to become available; never persisted. Both setters
 * accept strict 0/1, serialize on the worker and do not add history. Module
 * setter rejects unavailable settings at poll; closed/unknown tabs also fail there. */
xc_status xc_worker_set_note_numbering_enabled(xc_worker *worker, uint64_t tab, int enabled, xc_request **out);
/* Default-off heading-link migration capability; strict 0/1, FIFO/tab-local,
 * history-neutral and not saved. Effective only with configured note source/
 * numbering capability. Only exact presentation matches outside note IDs are
 * repaired; unsupported candidates retain their existing heading HTML. Native
 * hosts pass opaque scoped IDs through existing note click/hover requests. */
xc_status xc_worker_set_heading_note_links_enabled(xc_worker *worker, uint64_t tab, int enabled, xc_request **out);
xc_status xc_worker_set_note_numbers(xc_worker *worker, uint64_t tab, const char *module, int enabled, xc_request **out);
/* Repeatable typed presentation result. Copies 0/1 value and availability;
 * default preference is On, independently of availability. Outputs reset to zero
 * for null/pending/failed/wrong-kind results; reports their status. */
xc_status xc_request_note_numbers(const xc_request *request, int *enabled, int *available);
xc_status xc_worker_visit(xc_worker *worker, uint64_t tab, const char *module, const char *reference, xc_request **out);
xc_status xc_worker_navigate(xc_worker *worker, uint64_t tab, int direction, xc_request **out);
xc_status xc_worker_navigate_chapter(xc_worker *worker, uint64_t tab, const char *module, const char *reference, int direction, xc_request **out);
/* Synchronized Bible/optional commentary snapshots. commentary must be non-NULL;
 * empty requests no commentary. Selection/type errors arrive through poll.
 * Visits and navigation commit history only after both renders succeed. */
xc_status xc_worker_read_passage(xc_worker *worker, uint64_t tab, const char *module, const char *commentary, const char *reference, xc_request **out);
xc_status xc_worker_visit_passage(xc_worker *worker, uint64_t tab, const char *module, const char *commentary, const char *reference, xc_request **out);
/* Explicit Bible links replace the Bible; commentary links replace commentary,
 * retaining the other pane. Returns the owned paired passage after recording. */
xc_status xc_worker_visit_linked_passage(xc_worker *worker, uint64_t tab, const char *bible, const char *commentary, const char *target_module, const char *reference, xc_request **out);
xc_status xc_worker_navigate_passage(xc_worker *worker, uint64_t tab, int direction, xc_request **out);
xc_status xc_worker_navigate_passage_chapter(xc_worker *worker, uint64_t tab, const char *module, const char *commentary, const char *reference, int direction, xc_request **out);
xc_status xc_worker_history(xc_worker *worker, uint64_t tab, xc_request **out);
xc_status xc_worker_clear_history(xc_worker *worker, uint64_t tab, xc_request **out);
/* Settings belong to one tab and are copied on submission. Set/clear do not
 * add history; subsequent visits/navigation/tab chapters use those settings.
 * Unknown flag bits fail immediately with *out=NULL. Closed tabs fail at poll.
 * Non-tab chapter reads below preserve the unconfigured reader behavior. */
/* Core-owned per-module presentation defaults/preferences. Getter returns a
 * scalar request result; setter accepts exactly 0/1 for regular commentaries. */
xc_status xc_worker_module_presentation_options(xc_worker *worker, uint64_t tab, const char *module, xc_request **out);
xc_status xc_worker_set_commentary_chapter_mode(xc_worker *worker, uint64_t tab, const char *module, int enabled, xc_request **out);
/* Repeated getter: 0/1 when ready/presentation kind; -1 otherwise. */
int xc_request_commentary_chapter_mode(const xc_request *request);
/* Same typed result rules as chapter mode; setters accept only 0 or 1. */
int xc_request_headings(const xc_request *request);
/* Ready presentation result returns XC_OK and states -1=unknown, 0=off, 1=on.
 * Otherwise returns pending/failure/wrong-kind status; outputs reset to unknown.
 * Getter may repeat; setters accept only 0/1 and do not visit history. */
xc_status xc_request_content_options(const xc_request *request, int *images, int *font_faces);
/* Double spacing defaults off; exact legacy Doublespace=On enables it.
 * Getter: repeatable 0/1 for ready presentation result, -1 otherwise.
 * Setter: strict 0/1, FIFO/tab/module-local and history-neutral. Its CSS is
 * exposed only to opted-in HTML/CSS consumers. Annotation spacing takes
 * precedence; unsupported annotation layout retains spacing-only CSS. */
int xc_request_double_space(const xc_request *request);
xc_status xc_worker_set_double_space(xc_worker *worker, uint64_t tab, const char *module, int enabled, xc_request **out);
xc_status xc_worker_set_image_content(xc_worker *worker, uint64_t tab, const char *module, int enabled, xc_request **out);
xc_status xc_worker_set_respect_font_faces(xc_worker *worker, uint64_t tab, const char *module, int enabled, xc_request **out);
xc_status xc_worker_set_headings(xc_worker *worker, uint64_t tab, const char *module, int enabled, xc_request **out);
xc_status xc_worker_set_engine_options(xc_worker *worker, uint64_t tab, unsigned flags, xc_request **out);
/* Opt-in legacy module defaults; no preference files are read or written.
 * Precedence: module override, tab-wide override, enabled legacy defaults,
 * otherwise unconfigured SDK state. Clearing one layer retains other layers.
 * Installed module and live tab validation happens on the worker at poll time.
 * Settings and queries do not change history. Arguments/results are copied. */
/* Explicit read-only modops.conf import replaces this tab's engine settings
 * for all installed modules; missing sections use shared legacy defaults.
 * Failed imports preserve state/history. Path is copied; no files are written. */
/* Explicit save of configured engine settings. Preserves unrelated values in
 * an existing file, verifying a temporary SWORD configuration before replacing
 * it. Comments/layout normalize like legacy saves. No automatic persistence. */
xc_status xc_worker_save_module_preferences(xc_worker *worker, uint64_t tab, const char *path, xc_request **out);
xc_status xc_worker_import_module_preferences(xc_worker *worker, uint64_t tab, const char *path, xc_request **out);
xc_status xc_worker_enable_module_preferences(xc_worker *worker, uint64_t tab, xc_request **out);
/* Choice uses XIPHOS_VARIANT_PRIMARY/SECONDARY/ALL. Requires configured
 * settings and a declared filter; preserves unrelated options and history. */
xc_status xc_worker_select_module_variant(xc_worker *worker, uint64_t tab, const char *module, int choice, xc_request **out);
xc_status xc_worker_set_module_engine_options(xc_worker *worker, uint64_t tab, const char *module, unsigned flags, xc_request **out);
xc_status xc_worker_clear_module_engine_options(xc_worker *worker, uint64_t tab, const char *module, xc_request **out);
xc_status xc_worker_module_engine_options(xc_worker *worker, uint64_t tab, const char *module, xc_request **out);
/* Call after polling. Copies flags and configured (0 = SDK defaults); returns
 * pending/error status unchanged. Wrong result kind/NULL outputs are invalid. */
xc_status xc_request_engine_options(const xc_request *request, unsigned *flags, int *configured);
xc_status xc_worker_clear_engine_options(xc_worker *worker, uint64_t tab, xc_request **out);
/* Refresh with tab settings without adding/moving history. */
/* Explicit note preview uses tab settings without moving/adding history. */
/* Resolves an internal passage target or renders a note, without visiting.
 * After XC_OK take_link or take_note identifies the result kind. */
/* Contextual showRef dispatch returns a reference list; passage URIs and notes
 * return their existing kinds. Strong's/morphology return an owned lexical
 * preview after explicit tab lexicon configuration. Source context/URI must
 * be nonempty for this contextual API. No Bible history visit.
 * After XC_OK take_reference_list, take_link, take_note or take_lexical_entry
 * identifies the kind. Older resolve_study_link retains its two-kind contract. */
/* Explicit click dispatch records successful lexical visits in dictionary
 * history; other command kinds retain preview/target behavior. No Bible visit.
 * Legacy Unicode-uppercase requested key is recorded; raw command metadata is retained. */
/* Main study panes distinguish default Bible navigation from note/lexical
 * source context. Same result kinds/lexical recording as follow_study_link_at. */
xc_status xc_worker_follow_pane_link_at(xc_worker *worker, uint64_t tab, const char *navigation_module, const char *source_module, const char *reference, const char *uri, xc_request **out);
xc_status xc_worker_follow_study_link_at(xc_worker *worker, uint64_t tab, const char *current_module, const char *current_reference, const char *uri, xc_request **out);
xc_status xc_worker_resolve_study_link_at(xc_worker *worker, uint64_t tab, const char *current_module, const char *current_reference, const char *uri, xc_request **out);
/* Main hover only; use existing note/reference-list/lexical snapshot takes.
 * showRef and passage links return an explicit no-presentation marker; no
 * status-text/dictionary-URI/dialog hover is implemented by this slice.
 * No history visits; preference is copied per tab and defaults enabled. */
xc_status xc_worker_hover_study_link_at(xc_worker *worker, uint64_t tab, const char *current_module, const char *current_reference, const char *uri, xc_request **out);
xc_status xc_worker_set_cross_reference_preview_list(xc_worker *worker, uint64_t tab, int enabled, xc_request **out);
xc_status xc_worker_cross_reference_preview_list(xc_worker *worker, uint64_t tab, xc_request **out);
xc_status xc_request_cross_reference_preview_list(const xc_request *request, int *enabled);
/* True only for a completed successful explicit no-presentation hover result.
 * This copied marker is not a transferred snapshot; repeated queries are valid. */
int xc_request_no_presentation(const xc_request *request);
xc_status xc_worker_resolve_study_link(xc_worker *worker, uint64_t tab, const char *current_module, const char *uri, xc_request **out);
xc_status xc_worker_study_note_link(xc_worker *worker, uint64_t tab, const char *current_module, const char *uri, xc_request **out);
/* Copied per-tab lexicons, separate from engine modops.conf fields. Enable
 * explicitly sets legacy defaults without profile access; set replaces all
 * five nullable names. Getter transfers a snapshot even when unconfigured. */
xc_status xc_worker_enable_lexicon_preferences(xc_worker *worker, uint64_t tab, xc_request **out);
xc_status xc_worker_set_lexicon_preferences(xc_worker *worker, uint64_t tab, const xc_lexicon_names *names, xc_request **out);
xc_status xc_worker_lexicon_preferences(xc_worker *worker, uint64_t tab, xc_request **out);
/* Explicit selected XML file only. Import returns a configured snapshot via
 * xc_request_take_lexicons, preserves unavailable names and publishes all five
 * choices after successful parsing. Saved locale is not applied. Save requires
 * configured, nonempty choices and returns a void completion. Neither operation
 * visits/moves Bible or dictionary history. Dropping an observer does not cancel
 * an accepted operation; no automatic user-profile reads/writes occur. */
xc_status xc_worker_import_lexicon_profile(xc_worker *worker, uint64_t tab, const char *path, xc_request **out);
xc_status xc_worker_save_lexicon_profile(xc_worker *worker, uint64_t tab, const char *path, xc_request **out);
/* URI must be nonempty; origin is XIPHOS_LEXICON_MAIN/DIALOG. Nullable source
 * module/reference become empty (valid for source-independent Strong's).
 * Greek morphology validates source on the worker. No Bible history visit.
 * Source reference is authoritative; query passage remains metadata. */
/* Main-origin click renders/records the legacy Unicode-uppercase requested key.
 * Dialog-origin clicks retain raw key and do not visit dictionary history.
 * Raw command metadata is preserved; process locale is host-owned. */
xc_status xc_worker_visit_lexical(xc_worker *worker, uint64_t tab, const char *module, const char *reference, const char *uri, int origin, xc_request **out);
xc_status xc_worker_read_lexical(xc_worker *worker, uint64_t tab, const char *module, const char *reference, const char *uri, int origin, xc_request **out);
/* Copied nine-key chooser, preserving order/repeats at SDK boundaries. No visit.
 * Requires an open tab; preserves the Reader's dictionary key text. */
xc_status xc_worker_dictionary_choices(xc_worker *worker, uint64_t tab, const char *module, const char *key, xc_request **out);
xc_dictionary_choices *xc_request_take_dictionary_choices(xc_request *request);
const char *xc_dictionary_choices_module(const xc_dictionary_choices *choices);
const char *xc_dictionary_choices_key(const xc_dictionary_choices *choices);
size_t xc_dictionary_choices_count(const xc_dictionary_choices *choices);
const char *xc_dictionary_choices_at(const xc_dictionary_choices *choices, size_t index);
void xc_dictionary_choices_release(xc_dictionary_choices *choices);
/* Explicit dictionary visit records requested module/key after successful read.
 * No case normalization/Bible visit. History navigation commits after rendering.
 * Existing preview and key-step requests do not record dictionary history. */
xc_status xc_worker_visit_dictionary(xc_worker *worker, uint64_t tab, const char *module, const char *key, xc_request **out);
xc_status xc_worker_navigate_dictionary_history(xc_worker *worker, uint64_t tab, int direction, xc_request **out);
xc_status xc_worker_dictionary_history(xc_worker *worker, uint64_t tab, xc_request **out);
xc_status xc_worker_clear_dictionary_history(xc_worker *worker, uint64_t tab, xc_request **out);
xc_dictionary_history *xc_request_take_dictionary_history(xc_request *request);
size_t xc_dictionary_history_count(const xc_dictionary_history *history);
size_t xc_dictionary_history_cursor(const xc_dictionary_history *history);
int xc_dictionary_history_can_back(const xc_dictionary_history *history);
int xc_dictionary_history_can_forward(const xc_dictionary_history *history);
const char *xc_dictionary_history_module(const xc_dictionary_history *history, size_t index);
const char *xc_dictionary_history_key(const xc_dictionary_history *history, size_t index);
void xc_dictionary_history_release(xc_dictionary_history *history);
/* One dictionary key step (-1/+1), preserving SDK boundary behavior and tab
 * options. Returns the owned dictionary kind; does not visit Bible history. */
xc_status xc_worker_navigate_dictionary(xc_worker *worker, uint64_t tab, const char *module, const char *key, int direction, xc_request **out);
/* Explicit dictionary preview with tab settings; module/key nonempty. No visit. */
xc_status xc_worker_read_dictionary(xc_worker *worker, uint64_t tab, const char *module, const char *key, xc_request **out);
/* Expression may be empty; context/module must be nonempty. No history visit. */
xc_status xc_worker_read_reference_list(xc_worker *worker, uint64_t tab, const char *module, const char *expression, const char *context, xc_request **out);
xc_status xc_worker_study_note(xc_worker *worker, uint64_t tab, const char *module, const char *reference, const char *identifier, xc_request **out);
xc_status xc_worker_read_tab_entry(xc_worker *worker, uint64_t tab, const char *module, const char *reference, xc_request **out);
/* Commentary chapter uses sparse/linked entries in the owned xc_chapter;
 * selected verse is an engine ordinal, not a sparse-array index. No history. */
xc_status xc_worker_read_tab_commentary_chapter(xc_worker *worker, uint64_t tab, const char *module, const char *reference, unsigned display_flags, xc_request **out);
xc_status xc_worker_read_tab_chapter(xc_worker *worker, uint64_t tab, const char *module, const char *reference, unsigned display_flags, xc_request **out);
xc_status xc_worker_read_chapter(xc_worker *worker, const char *module, const char *reference, unsigned flags, xc_request **out);
xc_status xc_worker_reference_choices(xc_worker *worker, const char *module, const char *reference, xc_request **out);
xc_status xc_worker_resolve_link(xc_worker *worker, const char *current_module, const char *uri, xc_request **out);
xc_status xc_request_poll(xc_request *request);
void xc_request_release(xc_request *request);
/* Take exactly once after XC_OK; wrong type/not ready/already taken -> NULL.
 * Caller owns the transferred handle and uses its normal release function.
 * Copies survive worker/request close. Tab ID is zero unless ready/tab result. */
uint64_t xc_request_tab(const xc_request *request);
xc_reference_list *xc_request_take_reference_list(xc_request *request);
xc_dictionary_entry *xc_request_take_dictionary_entry(xc_request *request);
xc_lexical_entry *xc_request_take_lexical_entry(xc_request *request);
xc_lexicons *xc_request_take_lexicons(xc_request *request);
xc_note *xc_request_take_note(xc_request *request);
xc_entry *xc_request_take_entry(xc_request *request);
xc_passage *xc_request_take_passage(xc_request *request);
/* Borrowed strings valid until passage release; NULL handle -> NULL/zero.
 * Absent commentary -> empty module string and NULL commentary reference/HTML. */
const char *xc_passage_module(const xc_passage *passage);
const char *xc_passage_reference(const xc_passage *passage);
const char *xc_passage_html(const xc_passage *passage);
int xc_passage_has_commentary(const xc_passage *passage);
const char *xc_passage_commentary_module(const xc_passage *passage);
const char *xc_passage_commentary_reference(const xc_passage *passage);
const char *xc_passage_commentary_html(const xc_passage *passage);
/* Borrowed from the same owner as body HTML; null for absent/out-of-range. */
const char *xc_passage_commentary_heading_html(const xc_passage *passage);
/* Repeatable snapshot metadata getters. XC_OK includes disabled/fallback states.
 * Null/absent/out-of-range owner or null output -> XC_INVALID_ARGUMENT; nonnull
 * outputs reset to {NULL,0,0} on error. Copy CSS before releasing its owner.
 * Chapter and verse metadata share the same document-wide fallback decision. */
xc_status xc_entry_lexical_layout(const xc_entry *entry, xc_lexical_presentation *out);
xc_status xc_passage_lexical_layout(const xc_passage *passage, xc_lexical_presentation *out);
xc_status xc_passage_commentary_lexical_layout(const xc_passage *passage, xc_lexical_presentation *out);
xc_status xc_chapter_lexical_layout(const xc_chapter *chapter, xc_lexical_presentation *out);
xc_status xc_chapter_verse_lexical_layout(const xc_chapter *chapter, size_t verse, xc_lexical_presentation *out);
/* Copied core cleanup diagnostics: kind 0=image, 1=font face; -1 out of range.
 * Counts are zero for absent/null owners. Suffix strings are borrowed from the
 * same owned entry/passage/chapter; null for absent/out-of-range. */
size_t xc_entry_cleanup_count(const xc_entry *entry);
int xc_entry_cleanup_kind(const xc_entry *entry, size_t index);
const char *xc_entry_cleanup_suffix(const xc_entry *entry, size_t index);
size_t xc_passage_cleanup_count(const xc_passage *passage);
int xc_passage_cleanup_kind(const xc_passage *passage, size_t index);
const char *xc_passage_cleanup_suffix(const xc_passage *passage, size_t index);
size_t xc_passage_commentary_cleanup_count(const xc_passage *passage);
int xc_passage_commentary_cleanup_kind(const xc_passage *passage, size_t index);
const char *xc_passage_commentary_cleanup_suffix(const xc_passage *passage, size_t index);
size_t xc_chapter_verse_cleanup_count(const xc_chapter *chapter, size_t verse);
int xc_chapter_verse_cleanup_kind(const xc_chapter *chapter, size_t verse, size_t index);
const char *xc_chapter_verse_cleanup_suffix(const xc_chapter *chapter, size_t verse, size_t index);
/* Copied SDK verse ordinal; zero for absent/out-of-range metadata. */
unsigned xc_chapter_verse_ordinal(const xc_chapter *chapter, size_t verse);
void xc_passage_release(xc_passage *passage);
xc_chapter *xc_request_take_chapter(xc_request *request);
xc_choices *xc_request_take_choices(xc_request *request);
xc_link *xc_request_take_link(xc_request *request);
xc_catalog *xc_request_take_catalog(xc_request *request);
xc_history *xc_request_take_history(xc_request *request);
size_t xc_catalog_count(const xc_catalog *catalog);
const char *xc_catalog_id(const xc_catalog *catalog, size_t index);
const char *xc_catalog_description(const xc_catalog *catalog, size_t index);
const char *xc_catalog_type(const xc_catalog *catalog, size_t index);
const char *xc_catalog_language(const xc_catalog *catalog, size_t index);
/* Copied dictionary classification (excludes DailyDevotion) and declared
 * exact first-Feature roles. Missing catalog/index returns zero. */
int xc_catalog_is_dictionary(const xc_catalog *catalog, size_t index);
/* Generic Books with a runtime TreeKeyIdx source, not full native book parity. */
int xc_catalog_has_book_tree(const xc_catalog *catalog, size_t index);
int xc_catalog_is_regular_commentary(const xc_catalog *catalog, size_t index);
unsigned xc_catalog_lexicon_roles(const xc_catalog *catalog, size_t index);
/* Declared module features using the legacy GlobalOptionFilter checks, not
 * effective SDK/markup support or saved settings. Italic headings also require
 * display headings enabled. Transliteration is a global SDK capability and
 * retains its separate general menu role. Invalid catalog/index -> zero. */
unsigned xc_catalog_declared_engine_flags(const xc_catalog *catalog, size_t index);
int xc_catalog_has_headings(const xc_catalog *catalog, size_t index);
int xc_catalog_transliteration_available(const xc_catalog *catalog, size_t index);
void xc_catalog_release(xc_catalog *catalog);
int xc_history_can_back(const xc_history *history);
int xc_history_can_forward(const xc_history *history);
size_t xc_history_count(const xc_history *history);
const char *xc_history_module(const xc_history *history);
/* Current recorded pair; NULL for empty history, empty string for no commentary. */
const char *xc_history_commentary(const xc_history *history);
void xc_history_release(xc_history *history);
/* Version of this experimental ABI, not a parity or stability guarantee. */
unsigned xc_api_version(void);
/* NULL/empty root uses normal SWORD discovery. No downloads or data writes.
 * out must be non-NULL; *out is NULL on failure. Close exactly once. */
xc_status xc_open(const char *root, xc_reader **out);
void xc_close(xc_reader *reader);
/* Snapshot strings are UTF-8, owned by reader, valid until close. Invalid
 * reader/index returns zero/NULL. Serialize calls and close per handle. */
size_t xc_module_count(const xc_reader *reader);
const char *xc_module_id(const xc_reader *reader, size_t index);
const char *xc_module_description(const xc_reader *reader, size_t index);
const char *xc_module_type(const xc_reader *reader, size_t index);
const char *xc_module_language(const xc_reader *reader, size_t index);
int xc_module_is_dictionary(const xc_reader *reader, size_t index);
int xc_module_has_book_tree(const xc_reader *reader, size_t index);
int xc_module_is_regular_commentary(const xc_reader *reader, size_t index);
unsigned xc_module_lexicon_roles(const xc_reader *reader, size_t index);
/* out is NULL on failure. Entry owns copied strings independently of reader;
 * release exactly once. An empty HTML string is a valid missing-text entry.
 * Returned reference is SWORD-normalized; normalization is not strict input
 * validation. This slice accepts verse-key modules only. */
xc_status xc_read(xc_reader *reader, const char *module, const char *reference,
                  xc_entry **out);
/* Per-tab history; owns a retained reader and survives xc_close(reader).
 * Serialize all sessions sharing a reader, including destruction. Results
 * retain the same ownership contract as xc_read. Failed reads do not move
 * history. Navigation direction is -1 (back) or +1 (forward).
 * History preserves legacy reference-only identity and 50-entry retention.
 * Session module strings are borrowed until the next session mutation/close.
 * Commentary restoration and persistence are not implemented by this slice. */
xc_status xc_session_open(xc_reader *reader, xc_session **out);
void xc_session_close(xc_session *session);
xc_status xc_session_visit(xc_session *session, const char *module,
                           const char *reference, xc_entry **out);
xc_status xc_session_navigate(xc_session *session, int direction, xc_entry **out);
int xc_session_can_back(const xc_session *session);
int xc_session_can_forward(const xc_session *session);
size_t xc_session_history_count(const xc_session *session);
/* Module recorded at the history cursor; may differ from a duplicate-reference
 * visit's module, matching legacy retention of the first visit's metadata. */
const char *xc_session_module(const xc_session *session);
void xc_session_clear_history(xc_session *session);
/* Owned navigation snapshot. Book choices honor legacy first/last-testament
 * discovery; labels/reference targets use the module's SWORD key. No history
 * changes. Invalid kind/index returns 0/NULL. Strings survive reader close. */
typedef enum xc_choice_kind { XC_BOOK = 0, XC_CHAPTER = 1, XC_VERSE = 2 } xc_choice_kind;
xc_status xc_reference_choices(xc_reader *reader, const char *module,
                               const char *reference, xc_choices **out);
size_t xc_choices_count(const xc_choices *choices, xc_choice_kind kind);
const char *xc_choices_label(const xc_choices *choices, xc_choice_kind kind, size_t index);
const char *xc_choices_reference(const xc_choices *choices, xc_choice_kind kind, size_t index);
const char *xc_choices_current_label(const xc_choices *choices, xc_choice_kind kind);
void xc_choices_release(xc_choices *choices);
/* Resolve single-reference sword:// or bible:// links. Empty URI module uses
 * current_module. Exact installed IDs only; no abbreviations/fallback, ranges,
 * fragments or passagestudy actions yet. No history/key mutation or I/O dispatch.
 * Output strings owned until release, including after reader close. */
xc_status xc_resolve_link(xc_reader *reader, const char *current_module,
                          const char *uri, xc_link **out);
const char *xc_link_module(const xc_link *link);
const char *xc_link_reference(const xc_link *link);
void xc_link_release(xc_link *link);
/* Chapter snapshots include every verse, including empty entries, using the
 * module's versification. Strings survive reader/session close until release.
 * These are raw rendered entries, not full Xiphos chapter-decoration parity. */
xc_status xc_read_chapter(xc_reader *reader, const char *module, const char *reference,
                          xc_chapter **out);
/* These display flags affect only the copied chapter fragment, not global
 * SWORD options. Unknown bits are invalid. Original xc_read_chapter defaults
 * to both flags. Theme colors are supplied by the native host. */
#define XC_CHAPTER_VERSE_NUMBERS 1u
#define XC_CHAPTER_HIGHLIGHT_SELECTED 2u
xc_status xc_read_chapter_with_options(xc_reader *reader, const char *module,
                                      const char *reference, unsigned flags,
                                      xc_chapter **out);
size_t xc_chapter_selected_verse(const xc_chapter *chapter);
const char *xc_chapter_selected_anchor(const xc_chapter *chapter);
const char *xc_chapter_html(const xc_chapter *chapter);
const char *xc_chapter_reference(const xc_chapter *chapter);
const char *xc_chapter_title(const xc_chapter *chapter);
size_t xc_chapter_count(const xc_chapter *chapter);
const char *xc_chapter_verse_reference(const xc_chapter *chapter, size_t index);
const char *xc_chapter_verse_html(const xc_chapter *chapter, size_t index);
/* Borrowed from the same owner as body HTML; null for absent/out-of-range. */
const char *xc_chapter_verse_heading_html(const xc_chapter *chapter, size_t index);
void xc_chapter_release(xc_chapter *chapter);
/* Moves to verse 1 of the adjacent chapter, across books/testaments as SWORD
 * permits, and records a successful visit. -1/+1 only; no wrap at Bible ends. */
xc_status xc_session_navigate_chapter(xc_session *session, const char *module,
                                     const char *reference, int direction, xc_entry **out);
/* Snapshot strings are owned by their result until release; NULL -> NULL.
 * Invalid list indexes -> NULL; NULL count/limit/skipped -> zero. */
size_t xc_reference_list_count(const xc_reference_list *references);
const char *xc_reference_list_module(const xc_reference_list *references);
const char *xc_reference_list_context(const xc_reference_list *references);
const char *xc_reference_list_expression(const xc_reference_list *references);
const char *xc_reference_list_html(const xc_reference_list *references);
const char *xc_reference_list_entry_reference(const xc_reference_list *references, size_t index);
const char *xc_reference_list_entry_html(const xc_reference_list *references, size_t index);
int xc_reference_list_limited(const xc_reference_list *references);
unsigned xc_reference_list_skipped(const xc_reference_list *references);
void xc_reference_list_release(xc_reference_list *references);
/* Strings borrow from owned snapshots until release. NULL name preserves an
 * unset preference; configured distinguishes a wholly unconfigured tab. */
int xc_lexicons_configured(const xc_lexicons *lexicons);
const char *xc_lexicons_greek(const xc_lexicons *lexicons);
const char *xc_lexicons_hebrew(const xc_lexicons *lexicons);
const char *xc_lexicons_greek_morph_nt(const xc_lexicons *lexicons);
const char *xc_lexicons_greek_morph_ot(const xc_lexicons *lexicons);
const char *xc_lexicons_hebrew_morph(const xc_lexicons *lexicons);
void xc_lexicons_release(xc_lexicons *lexicons);
/* Kind is XIPHOS_LEXICON_STRONGS/MORPHOLOGY; invalid handle returns -1. */
int xc_lexical_entry_kind(const xc_lexical_entry *entry);
const char *xc_lexical_entry_module(const xc_lexical_entry *entry);
const char *xc_lexical_entry_type(const xc_lexical_entry *entry);
const char *xc_lexical_entry_value(const xc_lexical_entry *entry);
const char *xc_lexical_entry_passage(const xc_lexical_entry *entry);
const char *xc_lexical_entry_source_reference(const xc_lexical_entry *entry);
const char *xc_lexical_entry_dictionary_module(const xc_lexical_entry *entry);
const char *xc_lexical_entry_dictionary_key(const xc_lexical_entry *entry);
const char *xc_lexical_entry_dictionary_html(const xc_lexical_entry *entry);
void xc_lexical_entry_release(xc_lexical_entry *entry);
const char *xc_dictionary_entry_module(const xc_dictionary_entry *entry);
const char *xc_dictionary_entry_key(const xc_dictionary_entry *entry);
const char *xc_dictionary_entry_html(const xc_dictionary_entry *entry);
void xc_dictionary_entry_release(xc_dictionary_entry *entry);
const char *xc_note_module(const xc_note *note);
const char *xc_note_reference(const xc_note *note);
const char *xc_note_identifier(const xc_note *note);
const char *xc_note_type(const xc_note *note);
const char *xc_note_label(const xc_note *note);
const char *xc_note_html(const xc_note *note);
const char *xc_note_reference_list(const xc_note *note);
void xc_note_release(xc_note *note);
const char *xc_entry_reference(const xc_entry *entry);
const char *xc_entry_html(const xc_entry *entry);
/* Borrowed from the same owner as body HTML; null for absent/out-of-range. */
const char *xc_entry_heading_html(const xc_entry *entry);
void xc_entry_release(xc_entry *entry);
#ifdef __cplusplus
}
#endif
#endif
