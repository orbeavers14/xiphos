/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XIPHOS_CORE_C_API_H
#define XIPHOS_CORE_C_API_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct xc_reader xc_reader;
typedef struct xc_entry xc_entry;
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
const char *xc_entry_reference(const xc_entry *entry);
const char *xc_entry_html(const xc_entry *entry);
void xc_entry_release(xc_entry *entry);
#ifdef __cplusplus
}
#endif
#endif
