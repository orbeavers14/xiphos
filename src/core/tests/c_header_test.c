/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "xiphos/core/c_api.h"
int c_header_test(void) { return xc_api_version() == 1; }
