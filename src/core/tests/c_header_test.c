/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "xiphos/core/c_api.h"
#include "xiphos/core/history.h"
int c_header_test(void) {
  const char *references[] = {"A", "B"};
  xc_session *session = NULL;
  xc_request *request = NULL;
  xc_lexical_presentation layout = {"stale", 1, 1};
  uint64_t book_cursor = 123;
  xc_book_node book_node = {4, "stale", "stale", 1, 1};
  return xc_book_children_count(NULL) == 0 && xc_book_children_module(NULL) == NULL &&
         xc_book_children_parent(NULL, &book_node) == XC_INVALID_ARGUMENT && !book_node.present &&
         xc_book_children_get_node(NULL, 0, &book_node) == XC_INVALID_ARGUMENT && !book_node.path &&
         !xc_book_children_next_after(NULL, &book_cursor) && book_cursor == 0 &&
         xc_request_take_book_children(NULL) == NULL &&
         xc_worker_book_children(NULL, "Book", 0, 0, 0, 64, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_reference_list_count(NULL) == 0 && xc_reference_list_module(NULL) == NULL &&
         xc_book_tree_get_node(NULL, XC_BOOK_CURRENT, &book_node) == XC_INVALID_ARGUMENT &&
         book_node.offset == 0 && book_node.path == NULL && book_node.title == NULL && !book_node.present && !book_node.has_children &&
         xc_book_entry_tree(NULL) == NULL && xc_book_entry_html(NULL) == NULL && !xc_book_entry_has_entry(NULL) &&
         xc_book_tree_module(NULL) == NULL && xc_request_take_book_tree(NULL) == NULL && xc_request_take_book_entry(NULL) == NULL &&
         xc_worker_book_tree(NULL, "Book", 0, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_worker_book_tree_path(NULL, "Book", "", &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_worker_read_book(NULL, 0, "Book", 4, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_entry_lexical_layout(NULL, &layout) == XC_INVALID_ARGUMENT && layout.css == NULL && !layout.requested && !layout.applied &&
         xc_passage_lexical_layout(NULL, &layout) == XC_INVALID_ARGUMENT &&
         xc_passage_commentary_lexical_layout(NULL, &layout) == XC_INVALID_ARGUMENT &&
         xc_chapter_lexical_layout(NULL, &layout) == XC_INVALID_ARGUMENT &&
         xc_chapter_verse_lexical_layout(NULL, 0, &layout) == XC_INVALID_ARGUMENT &&
         xc_request_double_space(NULL) == -1 &&
         xc_worker_set_double_space(NULL, 0, "Text", 1, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_worker_set_lexical_layout_enabled(NULL, 0, 1, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_reference_list_context(NULL) == NULL && xc_reference_list_expression(NULL) == NULL &&
         xc_reference_list_html(NULL) == NULL && xc_reference_list_entry_reference(NULL, 0) == NULL &&
         xc_reference_list_entry_html(NULL, 0) == NULL && xc_reference_list_limited(NULL) == 0 &&
         xc_reference_list_skipped(NULL) == 0 && xc_request_take_reference_list(NULL) == NULL &&
         xc_worker_read_reference_list(NULL, 0, "M", "John3:16", "John3:16", &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_note_module(NULL) == NULL && xc_note_reference(NULL) == NULL && xc_note_identifier(NULL) == NULL &&
         xc_note_type(NULL) == NULL && xc_note_label(NULL) == NULL &&
         xc_note_html(NULL) == NULL && xc_note_reference_list(NULL) == NULL &&
         xc_request_take_note(NULL) == NULL &&
         xc_worker_study_note(NULL, 0, "M", "John 3:16", "1", &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_catalog_declared_engine_flags(NULL, 0) == 0 &&
         xc_catalog_has_headings(NULL, 0) == 0 && xc_catalog_transliteration_available(NULL, 0) == 0 &&
         xc_worker_set_engine_options(NULL, 0, XC_ENGINE_STRONGS, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_worker_clear_engine_options(NULL, 0, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_worker_read_tab_entry(NULL, 0, "M", "John 3:16", &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_worker_read_tab_chapter(NULL, 0, "M", "John 3:16", 0, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_worker_modules(NULL, &request) == XC_INVALID_ARGUMENT && request == NULL &&
         xc_request_poll(NULL) == XC_INVALID_ARGUMENT && xc_api_version() == 1 && xh_find_reference(references, 2, "B") == 1 &&
         xh_find_reference(references, 2, "Missing") == 2 &&
         xc_session_open(NULL, &session) == XC_INVALID_ARGUMENT && session == NULL;
}
