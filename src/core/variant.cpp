// SPDX-License-Identifier: GPL-2.0-or-later
// Extracted from gtk/utilities.c; Copyright (C) 2000-2026 Xiphos Developer Team
#include "xiphos/core/variant.h"
XiphosVariantSelection xiphos_core_variant_selection(int choice)
{
        XiphosVariantSelection selection = {0, 0, 0, 1};
        switch (choice) {
        case XIPHOS_VARIANT_PRIMARY:
                selection.primary = 1;
                break;
        case XIPHOS_VARIANT_SECONDARY:
                selection.secondary = 1;
                break;
        case XIPHOS_VARIANT_ALL:
                selection.all = 1;
                break;
        default:
                selection.valid = 0;
                break;
        }
        return selection;
}
