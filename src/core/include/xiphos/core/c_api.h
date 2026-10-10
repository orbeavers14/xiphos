/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XIPHOS_CORE_C_API_H
#define XIPHOS_CORE_C_API_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct xc_reader xc_reader;
typedef struct xc_entry xc_entry;
typedef struct xc_session xc_session;
typedef enum xc_status { XC_OK = 0, XC_INVALID_ARGUMENT = 1,
  XC_MODULE_NOT_FOUND = 2, XC_ENGINE_ERROR = 3 } xc_status;
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
const char *xc_entry_reference(const xc_entry *entry);
const char *xc_entry_html(const xc_entry *entry);
void xc_entry_release(xc_entry *entry);
#ifdef __cplusplus
}
#endif
#endif
