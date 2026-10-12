/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XIPHOS_CORE_C_API_H
#define XIPHOS_CORE_C_API_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct xc_reader xc_reader;
typedef struct xc_entry xc_entry;
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
xc_status xc_worker_read_chapter(xc_worker *worker, const char *module, const char *reference, unsigned flags, xc_request **out);
xc_status xc_worker_reference_choices(xc_worker *worker, const char *module, const char *reference, xc_request **out);
xc_status xc_worker_resolve_link(xc_worker *worker, const char *current_module, const char *uri, xc_request **out);
xc_status xc_request_poll(xc_request *request);
void xc_request_release(xc_request *request);
/* Take exactly once after XC_OK; wrong type/not ready/already taken -> NULL.
 * Caller owns the transferred handle and uses its normal release function.
 * Copies survive worker/request close. Tab ID is zero unless ready/tab result. */
uint64_t xc_request_tab(const xc_request *request);
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
const char *xc_entry_reference(const xc_entry *entry);
const char *xc_entry_html(const xc_entry *entry);
void xc_entry_release(xc_entry *entry);
#ifdef __cplusplus
}
#endif
#endif
