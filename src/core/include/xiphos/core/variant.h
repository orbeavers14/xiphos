/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XIPHOS_CORE_VARIANT_H
#define XIPHOS_CORE_VARIANT_H
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    XIPHOS_VARIANT_PRIMARY = 0,
    XIPHOS_VARIANT_SECONDARY = 1,
    XIPHOS_VARIANT_ALL = 2
} XiphosVariantReading;
typedef struct {
    int primary, secondary, all, valid;
} XiphosVariantSelection;
/* Existing reading_selector mapping. Invalid choices return zero flags. */
XiphosVariantSelection xiphos_core_variant_selection(int choice);
#ifdef __cplusplus
}
#endif
#endif
