// SPDX-License-Identifier: GPL-2.0-or-later
// Frozen actual main_do_sidebar_search indexed text preparation, master 562dfb8.
// Xiphos Developer Team GPL-2.0-or-later, kept independent after extraction.
std::string frozen_sidebar_search_text(const std::string &input,bool indexed) {
 GString *new_search=g_string_new(nullptr);
 const char *search_string=input.c_str();
 struct { int searchType; } settings{indexed?-4:-2};
	if (settings.searchType == -4) {
		g_string_append_c(new_search, '+');
		while (*search_string) {
			while (((*search_string) != ' ') && ((*search_string) != '\0'))
				g_string_append_c(new_search, *(search_string++));
			if ((*search_string) == ' ') {
				g_string_append_c(new_search, ' ');
				search_string++;
				if ((*search_string) != '\0')
					g_string_append_c(new_search, '+');
			}
		}

		// append an arbitrary space to search string end:
		// workaround for sword search failure of single +TERM.
		g_string_append_c(new_search, ' ');

		search_string = new_search->str;
	}

 std::string result(search_string);g_string_free(new_search,true);return result;
}
