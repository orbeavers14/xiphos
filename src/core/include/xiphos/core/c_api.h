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
typedef struct xc_note xc_note;
typedef struct xc_dictionary_entry xc_dictionary_entry;
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
xc_status xc_worker_modules(xc_worker *worker, xc_request **out);
xc_status xc_worker_create_tab(xc_worker *worker, xc_request **out);
xc_status xc_worker_close_tab(xc_worker *worker, uint64_t tab, xc_request **out);
xc_status xc_worker_visit(xc_worker *worker, uint64_t tab, const char *module, const char *reference, xc_request **out);
xc_status xc_worker_navigate(xc_worker *worker, uint64_t tab, int direction, xc_request **out);
xc_status xc_worker_navigate_chapter(xc_worker *worker, uint64_t tab, const char *module, const char *reference, int direction, xc_request **out);
xc_status xc_worker_history(xc_worker *worker, uint64_t tab, xc_request **out);
xc_status xc_worker_clear_history(xc_worker *worker, uint64_t tab, xc_request **out);
/* Settings belong to one tab and are copied on submission. Set/clear do not
 * add history; subsequent visits/navigation/tab chapters use those settings.
 * Unknown flag bits fail immediately with *out=NULL. Closed tabs fail at poll.
 * Non-tab chapter reads below preserve the unconfigured reader behavior. */
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
 * return their existing kinds. Source context/URI must be nonempty. No visit.
 * After XC_OK take_reference_list, take_link or take_note identifies the kind. */
xc_status xc_worker_resolve_study_link_at(xc_worker *worker, uint64_t tab, const char *current_module, const char *current_reference, const char *uri, xc_request **out);
xc_status xc_worker_resolve_study_link(xc_worker *worker, uint64_t tab, const char *current_module, const char *uri, xc_request **out);
xc_status xc_worker_study_note_link(xc_worker *worker, uint64_t tab, const char *current_module, const char *uri, xc_request **out);
/* Copied per-tab lexicons, separate from engine modops.conf fields. Enable
 * explicitly sets legacy defaults without profile access; set replaces all
 * five nullable names. Getter transfers a snapshot even when unconfigured. */
xc_status xc_worker_enable_lexicon_preferences(xc_worker *worker, uint64_t tab, xc_request **out);
xc_status xc_worker_set_lexicon_preferences(xc_worker *worker, uint64_t tab, const xc_lexicon_names *names, xc_request **out);
xc_status xc_worker_lexicon_preferences(xc_worker *worker, uint64_t tab, xc_request **out);
/* URI must be nonempty; origin is XIPHOS_LEXICON_MAIN/DIALOG. Nullable source
 * module/reference become empty (valid for source-independent Strong's).
 * Greek morphology validates source on the worker. No Bible history visit.
 * Source reference is authoritative; query passage remains metadata. */
xc_status xc_worker_read_lexical(xc_worker *worker, uint64_t tab, const char *module, const char *reference, const char *uri, int origin, xc_request **out);
/* Explicit dictionary preview with tab settings; module/key nonempty. No visit. */
xc_status xc_worker_read_dictionary(xc_worker *worker, uint64_t tab, const char *module, const char *key, xc_request **out);
/* Expression may be empty; context/module must be nonempty. No history visit. */
xc_status xc_worker_read_reference_list(xc_worker *worker, uint64_t tab, const char *module, const char *expression, const char *context, xc_request **out);
xc_status xc_worker_study_note(xc_worker *worker, uint64_t tab, const char *module, const char *reference, const char *identifier, xc_request **out);
xc_status xc_worker_read_tab_entry(xc_worker *worker, uint64_t tab, const char *module, const char *reference, xc_request **out);
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
void xc_entry_release(xc_entry *entry);
#ifdef __cplusplus
}
#endif
#endif
