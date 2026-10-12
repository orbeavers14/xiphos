/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "xiphos/core/c_api.h"
#include "xiphos/core/history.h"
int c_header_test(void) {
  const char *references[] = {"A", "B"};
  xc_session *session = NULL;
  xc_request *request = NULL;
  return xc_worker_set_engine_options(NULL, 0, XC_ENGINE_STRONGS, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_worker_clear_engine_options(NULL, 0, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_worker_read_tab_entry(NULL, 0, "M", "John 3:16", &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_worker_read_tab_chapter(NULL, 0, "M", "John 3:16", 0, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_worker_modules(NULL, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_request_poll(NULL) == XC_INVALID_ARGUMENT && xc_api_version() == 1 && xh_find_reference(references, 2, "B") == 1 &&
         xh_find_reference(references, 2, "Missing") == 2 &&
         xc_session_open(NULL, &session) == XC_INVALID_ARGUMENT && session == NULL;
}
