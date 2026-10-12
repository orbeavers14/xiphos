/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XIPHOS_CORE_HISTORY_H
#define XIPHOS_CORE_HISTORY_H
#include <stddef.h>
#define XH_HISTORY_LIMIT 50
#ifdef __cplusplus
extern "C" {
#endif
/* Shared legacy rule: identity is the reference, not the selected modules.
 * Returns count when no match exists. All supplied strings must be non-NULL. */
size_t xh_find_reference(const char *const *references, size_t count,
                         const char *reference);
#ifdef __cplusplus
}
#endif
#endif
