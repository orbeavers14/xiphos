/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "xiphos/core/c_api.h"
#include "xiphos/core/history.h"
int c_header_test(void) {
  const char *references[] = {"A", "B"};
  xc_session *session = NULL;
  return xc_api_version() == 1 && xh_find_reference(references, 2, "B") == 1 &&
         xh_find_reference(references, 2, "Missing") == 2 &&
         xc_session_open(NULL, &session) == XC_INVALID_ARGUMENT && session == NULL;
}
